#include <audio_math/float_properties.h>
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include <audio_math/math.h>
#include <audio_math/float_sqrt.h>
#include "../../core/include/ar_math_constants.h"
#include "lhdc_version.h"
#include <audio_math/constants.h>
#include "../../core/include/ar_internal.h"
#include "../../core/include/ar_workspace_views.h"
#include <math.h>
#include <string.h>

#include "upmix_library.h"
#include "../../common/vbap_angle.h"

/* Calculate bytes for ten internal pointer slots in upmix workspace. */
static uint32_t LHDC_PRIVATE(upmix_workspace_directory_bytes)(void) {
    return sizeof(LHDCUpmixDirectory);
}

/* Apply IIR filtering using filter coefficients and history. */
#if defined(__aarch64__) || defined(_M_ARM64)
void LHDC_FN(process_iir_filter)(const double *input, const LHDCIirPlan *plan, int32_t frame_length,
                                 double *zi, double *zo, double *output) {
    const int32_t order = plan->order;
    double denominator_coef[10], numerator_coef[10];
    const int32_t coefficient_bytes = (order + 1) * (int32_t)sizeof(double);
    memcpy(denominator_coef, plan->denominator, coefficient_bytes);
    memcpy(numerator_coef, plan->numerator, coefficient_bytes);
    double leading_denominator = denominator_coef[0];
    if (!audio_math_is_infinite_f32((float)leading_denominator) &&
        !audio_math_is_nan_f32((float)leading_denominator) && leading_denominator != 0. &&
        leading_denominator != 1.) {
        for (int32_t i = 0; i <= order; ++i)
            numerator_coef[i] /= leading_denominator;
        for (int32_t i = 1; i <= order; ++i)
            denominator_coef[i] /= leading_denominator;
        denominator_coef[0] = 1.;
    }
    for (int32_t i = 0; i < order; ++i) {
        output[i] = zi[i];
        zo[i] = 0.;
    }
    const int32_t output_bytes = (frame_length - order) * (int32_t)sizeof(double);
    memset(output + order, 0, output_bytes);
    for (int32_t i = 0; i < frame_length; ++i) {
        int32_t available_order = order;
        if (available_order > frame_length - i - 1)
            available_order = frame_length - i - 1;
        for (int32_t j = 0; j <= available_order; ++j)
            output[i + j] += input[i] * numerator_coef[j];
        double y = output[i];
        for (int32_t j = 1; j <= available_order; ++j)
            output[i + j] -= y * denominator_coef[j];
    }
    for (int32_t i = 0; i < order; ++i)
        for (int32_t j = 0; j <= i; ++j)
            zo[j] += input[frame_length - order + i] * numerator_coef[order - i + j];
    for (int32_t i = 0; i < order; ++i)
        for (int32_t j = 0; j <= i; ++j)
            zo[j] -= output[frame_length - order + i] * denominator_coef[order - i + j];
}
#else
void LHDC_FN(process_iir_filter)(const double *input, const LHDCIirPlan *plan, int32_t frame_length,
                                 double *zi, double *zo, double *output) {
    const int32_t order = plan->order;
    const double *denominator_coef = plan->denominator;
    const double *numerator_coef = plan->numerator;
    double denominator_storage[10], numerator_storage[10];
    double leading_denominator = denominator_coef[0];
    if (!audio_math_is_infinite_f32((float)leading_denominator) &&
        !audio_math_is_nan_f32((float)leading_denominator) && leading_denominator != 0. &&
        leading_denominator != 1.) {
        memcpy(denominator_storage, denominator_coef, (order + 1) * sizeof(double));
        memcpy(numerator_storage, numerator_coef, (order + 1) * sizeof(double));
        for (int32_t i = 0; i <= order; ++i)
            numerator_storage[i] /= leading_denominator;
        for (int32_t i = 1; i <= order; ++i)
            denominator_storage[i] /= leading_denominator;
        denominator_storage[0] = 1.;
        denominator_coef = denominator_storage;
        numerator_coef = numerator_storage;
    }
    for (int32_t i = 0; i < order; ++i) {
        output[i] = zi[i];
        zo[i] = 0.;
    }
    memset(output + order, 0, (frame_length - order) * sizeof(double));
    if (order == 9) {
        int32_t i = 0;
        int32_t limit = frame_length - 9;
        for (; i < limit; ++i) {
            double in_val = input[i];
            output[i + 0] += in_val * numerator_coef[0];
            output[i + 1] += in_val * numerator_coef[1];
            output[i + 2] += in_val * numerator_coef[2];
            output[i + 3] += in_val * numerator_coef[3];
            output[i + 4] += in_val * numerator_coef[4];
            output[i + 5] += in_val * numerator_coef[5];
            output[i + 6] += in_val * numerator_coef[6];
            output[i + 7] += in_val * numerator_coef[7];
            output[i + 8] += in_val * numerator_coef[8];
            output[i + 9] += in_val * numerator_coef[9];
            double y = output[i];
            output[i + 1] -= y * denominator_coef[1];
            output[i + 2] -= y * denominator_coef[2];
            output[i + 3] -= y * denominator_coef[3];
            output[i + 4] -= y * denominator_coef[4];
            output[i + 5] -= y * denominator_coef[5];
            output[i + 6] -= y * denominator_coef[6];
            output[i + 7] -= y * denominator_coef[7];
            output[i + 8] -= y * denominator_coef[8];
            output[i + 9] -= y * denominator_coef[9];
        }
        for (; i < frame_length; ++i) {
            int32_t available_order = frame_length - i - 1;
            for (int32_t j = 0; j <= available_order; ++j)
                output[i + j] += input[i] * numerator_coef[j];
            double y = output[i];
            for (int32_t j = 1; j <= available_order; ++j)
                output[i + j] -= y * denominator_coef[j];
        }
    } else if (order == 2) {
        int32_t i = 0;
        int32_t limit = frame_length - 2;
        for (; i < limit; ++i) {
            double in_val = input[i];
            output[i + 0] += in_val * numerator_coef[0];
            output[i + 1] += in_val * numerator_coef[1];
            output[i + 2] += in_val * numerator_coef[2];
            double y = output[i];
            output[i + 1] -= y * denominator_coef[1];
            output[i + 2] -= y * denominator_coef[2];
        }
        for (; i < frame_length; ++i) {
            int32_t available_order = frame_length - i - 1;
            for (int32_t j = 0; j <= available_order; ++j)
                output[i + j] += input[i] * numerator_coef[j];
            double y = output[i];
            for (int32_t j = 1; j <= available_order; ++j)
                output[i + j] -= y * denominator_coef[j];
        }
    } else {
        for (int32_t i = 0; i < frame_length; ++i) {
            int32_t available_order = order;
            if (available_order > frame_length - i - 1)
                available_order = frame_length - i - 1;
            for (int32_t j = 0; j <= available_order; ++j)
                output[i + j] += input[i] * numerator_coef[j];
            double y = output[i];
            for (int32_t j = 1; j <= available_order; ++j)
                output[i + j] -= y * denominator_coef[j];
        }
    }
    for (int32_t i = 0; i < order; ++i)
        for (int32_t j = 0; j <= i; ++j)
            zo[j] += input[frame_length - order + i] * numerator_coef[order - i + j];
    for (int32_t i = 0; i < order; ++i)
        for (int32_t j = 0; j <= i; ++j)
            zo[j] -= output[frame_length - order + i] * denominator_coef[order - i + j];
}
#endif

/* Look up channel gains for the specified direction in the VBAP table. */
int32_t LHDC_FN(get_vbap_gain_result)(float *ptr_power_l_bank, float *ptr_power_r_bank,
                                      int32_t frame_length, float *ptr_vbap_gain_buf) {
    const float *table;
    LHDC_FN(vbap_gain_get_table_ex)(&table);
    for (int32_t i = 0; i < frame_length; ++i) {
        float left_magnitude = audio_math_sqrt_f32(ptr_power_l_bank[i]),
              right_magnitude = audio_math_sqrt_f32(ptr_power_r_bank[i]),
              magnitude_sum = left_magnitude + right_magnitude;
        float direction_sine =
            (magnitude_sum == 0.f ? 0.f : (right_magnitude - left_magnitude) / magnitude_sum) * .5f;
        int32_t rounded_angle = lhdc_ar_vbap_angle(direction_sine);
        int32_t table_index = rounded_angle < 0 ? 181 + rounded_angle : rounded_angle;
        ptr_vbap_gain_buf[i] = table[table_index];
    }
    return LHDC_CONST(STATUS_OK);
}

/* Compute compensation-filter coefficients for upmix bands. */
int32_t LHDC_FN(calculate_comp_filter_bank)(int32_t bank_index, int32_t sample_rate,
                                            int32_t sample_length, int32_t left_0_right_1,
                                            float *ptr_out_bank_data,
                                            LHDCUpmixDirectory *workspace) {
    LHDCIirPlan plan;
    if (LHDC_FN(comple_filter_bank_get_coefficient_ex)(bank_index, sample_rate, &plan) ||
        !plan.denominator || !plan.numerator)
        return LHDC_CONST(UPMIX_COEFFICIENT_ERROR);
    double *state =
        (double *)workspace->comp_filter_z_buf + ((left_0_right_1 == 0) | (bank_index << 1)) * 9;
    LHDC_FN(process_iir_filter)
    (workspace->iir_filter_in_buf, &plan, sample_length, state, state,
     workspace->iir_filter_out_buf);
    double *filtered = workspace->iir_filter_out_buf;
    for (int32_t i = 0; i < sample_length; ++i)
        ptr_out_bank_data[i] = (float)filtered[i];
    return LHDC_CONST(STATUS_OK);
}

/* Upmix algorithm: calculate workspace size. */
int32_t LHDC_FN(upmix_lib_get_request_workspace_size)(uint32_t *rtn_size) {
    *rtn_size = 0x4aa0 + 10 * sizeof(void *);
    return LHDC_CONST(STATUS_OK);
}

/* Estimate band power and image gains, then separate direct and ambient sound. */
static void LHDC_PRIVATE(separate_upmix_bands)(LHDCUpmixDirectory *workspace, float phase_l,
                                               float phase_r, float power_alpha,
                                               int32_t frame_length) {
    float *power = workspace->temp_buf_area_2, *data = workspace->bank_result;
    float *prev = workspace->pre_power_value_buf, *vbap = workspace->temp_buf_area_1;
    const int32_t offsets[4] = {0, 512, 768, 1536};
    for (int32_t side = 0; side < 2; ++side) {
        float *left_band = data + 512 + side * 512, *right_band = left_band + 256;
        float max = -1.f;
        for (int32_t i = 0; i < frame_length; ++i) {
            float first_component = audio_math_abs_f32(left_band[i]),
                  second_component = audio_math_abs_f32(right_band[i]);
            if (!(first_component >= second_component))
                first_component = second_component;
            if (max < first_component)
                max = first_component;
        }
        if (audio_math_abs_f32(max) < LHDC_AR_GAIN_ZERO_THRESHOLD_F32)
            max = 0.f;
        LHDC_PRIVATE(smooth_normalized_power)
        (left_band, power_alpha, max, frame_length, power + side * 512, prev + side);
        LHDC_PRIVATE(smooth_normalized_power)
        (right_band, power_alpha, max, frame_length, power + side * 512 + 256, prev + side + 2);
    }
    LHDC_FN(get_vbap_gain_result)(power, power + 512, frame_length, vbap);
    LHDC_FN(get_vbap_gain_result)(power + 256, power + 768, frame_length, vbap + 256);
    for (int32_t bank = 0; bank < 2; ++bank) {
        float *left_band = power + bank * 256, *right_band = left_band + 512;
        for (int32_t i = 0; i < frame_length; ++i) {
            float sum = left_band[i] + right_band[i];
            if (audio_math_abs_f32(sum) < LHDC_AR_GAIN_ZERO_THRESHOLD_F32) {
                left_band[i] = 0.f;
                right_band[i] = 0.f;
            } else {
                right_band[i] = audio_math_sqrt_f32(right_band[i] / sum);
                left_band[i] = audio_math_sqrt_f32(left_band[i] / sum);
            }
        }
    }
    for (int32_t bank = 0; bank < 2; ++bank) {
        float *direct = power + bank * 256, *ambient = direct + 512,
              *left_band = data + offsets[bank + 1], *right_band = left_band + 512;
        for (int32_t i = 0; i < frame_length; ++i) {
            float gl = direct[i], gr = ambient[i];
            if (gl * phase_r - gr * phase_l == 0.f) {
                direct[i] = 0.f;
                ambient[i] = 0.f;
            } else {
                direct[i] = (left_band[i] + right_band[i]) / (gl + gr);
                ambient[i] = (gl * right_band[i] - gr * left_band[i]) / -(gl + gr);
            }
        }
    }
}

/* Upmix stereo to four or six channels using phase, smoothing, and sample rate; update instance history. */
int32_t LHDC_FN(upmix_lib_process_ex)(const float *ptr_data_in_l, const float *ptr_data_in_r,
                                      float phase_l, float phase_r, float power_alpha,
                                      float gain_filter_bank, int32_t frame_length,
                                      int32_t upmix_type, int32_t sample_rate,
                                      float *ptr_data_out_fl, float *ptr_data_out_fr,
                                      float *ptr_data_out_c, float *ptr_data_out_lfe,
                                      float *ptr_data_out_rl, float *ptr_data_out_rr,
                                      LHDCUpmixDirectory *workspace) {
    if (!ptr_data_in_l || !ptr_data_in_r)
        return LHDC_CONST(UPMIX_NULL_INPUT);
    if (!ptr_data_out_fl || !ptr_data_out_fr || !ptr_data_out_c || !ptr_data_out_lfe ||
        (upmix_type == 1 && (!ptr_data_out_rl || !ptr_data_out_rr)))
        return LHDC_CONST(UPMIX_NULL_OUTPUT);
    double *input = workspace->iir_filter_in_buf, *output = workspace->iir_filter_out_buf,
           *shelf = workspace->shlv_filter_z_buf;
    float *vbap = workspace->temp_buf_area_1, *power = workspace->temp_buf_area_2,
          *data = workspace->bank_result;
    LHDCIirPlan front_plan = {0}, rear_plan = {0};
    if (upmix_type == 1 &&
        (LHDC_FN(shelving_filter_get_coefficient_ex)(sample_rate, 0, &front_plan) == -1 ||
         LHDC_FN(shelving_filter_get_coefficient_ex)(sample_rate, 1, &rear_plan) == -1))
        return LHDC_CONST(UPMIX_SHELVING_FILTER_ERROR);
    int32_t offsets[4] = {0, 512, 768, 1536};
    int32_t right_offsets[4] = {256, 1024, 1280, 1792};
    for (int32_t side = 0; side < 2; ++side) {
        const float *src = side ? ptr_data_in_r : ptr_data_in_l;
        for (int32_t i = 0; i < frame_length; ++i)
            input[i] = src[i];
        int32_t error = 0;
        for (int32_t bank = 0; bank < 4; ++bank)
            error |= LHDC_FN(calculate_comp_filter_bank)(
                bank, sample_rate, frame_length, side,
                data + (side ? right_offsets[bank] : offsets[bank]), workspace);
        if (error)
            return LHDC_CONST(UPMIX_FILTER_BANK_ERROR);
    }
    LHDC_PRIVATE(separate_upmix_bands)(workspace, phase_l, phase_r, power_alpha, frame_length);
    for (int32_t i = 0; i < frame_length; ++i) {
        float first_component = power[512 + i], second_component = power[768 + i];
        ptr_data_out_fl[i] =
            data[1536 + i] + (first_component * phase_l + second_component * phase_l);
        ptr_data_out_fr[i] =
            (first_component * phase_r + second_component * phase_r) + data[1792 + i];
    }
    if (upmix_type == 0) {
        if (ptr_data_out_rl)
            memset(ptr_data_out_rl, 0, frame_length * 4);
        if (ptr_data_out_rr)
            memset(ptr_data_out_rr, 0, frame_length * 4);
    } else if (upmix_type == 1) {
        float *front[2] = {ptr_data_out_fl, ptr_data_out_fr},
              *rear[2] = {ptr_data_out_rl, ptr_data_out_rr};
        for (int32_t side = 0; side < 2; ++side) {
            for (int32_t i = 0; i < frame_length; ++i)
                input[i] = front[side][i];
            LHDC_FN(process_iir_filter)
            (input, &front_plan, frame_length, shelf + 2 * side, shelf + 2 * side, output);
            for (int32_t i = 0; i < frame_length; ++i)
                front[side][i] = (float)output[i];
            LHDC_FN(process_iir_filter)
            (input, &rear_plan, frame_length, shelf + 4 + 2 * side, shelf + 4 + 2 * side, output);
            for (int32_t i = 0; i < frame_length; ++i)
                rear[side][i] = (float)output[i];
        }
    }
    for (int32_t i = 0; i < frame_length; ++i) {
        ptr_data_out_c[i] =
            (power[i] * vbap[i] + power[256 + i] * vbap[256 + i]) * gain_filter_bank;
        ptr_data_out_fl[i] *= gain_filter_bank;
        ptr_data_out_fr[i] *= gain_filter_bank;
        ptr_data_out_lfe[i] = (data[i] + data[256 + i]) * .5f * gain_filter_bank;
        if (ptr_data_out_rl)
            ptr_data_out_rl[i] *= gain_filter_bank;
        if (ptr_data_out_rr)
            ptr_data_out_rr[i] *= gain_filter_bank;
    }
    return LHDC_FN(check_protection_area)(workspace);
}

/* Upmix algorithm: perform channel upmixing. */
int32_t LHDC_FN(upmix_lib_process)(PST_LHDC_UPMIX state, int32_t frame_length) {
    LHDCUpmixDirectory *workspace = (void *)state->pucLibWorkspace;
    struct upmix_process_parameter *parameters = workspace->process_parameter;
    return LHDC_FN(upmix_lib_process_ex)(
        state->pfPcm[0], state->pfPcm[1], 1.f, -1.f, parameters->power_alpha,
        parameters->gain_filter_bank, frame_length, 0, state->nSampleRate, state->pfPcmOutput[0],
        state->pfPcmOutput[1], state->pfPcmOutput[2], state->pfPcmOutput[3], NULL, NULL, workspace);
}

/* Verify upmix workspace guard regions. */
int32_t LHDC_FN(check_protection_area)(LHDCUpmixDirectory *workspace) {
    uint8_t *workspace_slots = workspace->protect_area;
    for (int32_t i = 0; i < 8; ++i)
        if (workspace_slots[i] != 6)
            return LHDC_CONST(UPMIX_GUARD_CORRUPTED);
    return LHDC_CONST(STATUS_OK);
}

/* Upmix algorithm: clear history buffers and filter state. */
int32_t LHDC_FN(upmix_lib_reset_cache_buffer)(uint8_t *addr_mem) {
    if (!addr_mem)
        return LHDC_CONST(UPMIX_NULL_WORKSPACE);
    LHDCUpmixStorage *storage = &((LHDCUpmixWorkspace *)addr_mem)->storage;
    memset(storage->comp_filter_z_buf, 0, sizeof(storage->comp_filter_z_buf));
    memset(storage->shlv_filter_z_buf, 0, sizeof(storage->shlv_filter_z_buf));
    memset(storage->pre_power_value_buf, 0, sizeof(storage->pre_power_value_buf));
    memset(storage->protect_area, 6, sizeof(storage->protect_area));
    return LHDC_CONST(STATUS_OK);
}

/* Upmix algorithm: restore default settings. */
int32_t LHDC_FN(upmix_lib_reset_setting)(uint8_t *addr_mem) {
    if (!addr_mem)
        return LHDC_CONST(UPMIX_NULL_WORKSPACE);
    uint64_t init = UINT64_C(0x3f19999a3f7d70a4);
    memcpy(((LHDCUpmixDirectory *)addr_mem)->process_parameter, &init, sizeof(init));
    return LHDC_CONST(STATUS_OK);
}

/* Upmix algorithm: initialize configuration and runtime state. */
int32_t LHDC_FN(upmix_lib_init)(uint8_t *addr_mem, uint32_t mem_size) {
    if (!addr_mem)
        return LHDC_CONST(UPMIX_NULL_WORKSPACE);
    uint32_t direction_table = LHDC_PRIVATE(upmix_workspace_directory_bytes)();
    int32_t init_result = LHDC_CONST(UPMIX_INVALID_WORKSPACE_SIZE);
    if (mem_size == direction_table + 0x4aa0) {
        memset(addr_mem, 0, mem_size);
        LHDCUpmixWorkspace *workspace = (void *)addr_mem;
        workspace->directory.process_parameter = &workspace->storage.process_parameter;
        workspace->directory.iir_filter_in_buf = workspace->storage.iir_filter_in_buf;
        workspace->directory.iir_filter_out_buf = workspace->storage.iir_filter_out_buf;
        workspace->directory.temp_buf_area_1 = workspace->storage.temp_buf_area_1;
        workspace->directory.temp_buf_area_2 = workspace->storage.temp_buf_area_2;
        workspace->directory.comp_filter_z_buf = workspace->storage.comp_filter_z_buf;
        workspace->directory.shlv_filter_z_buf = workspace->storage.shlv_filter_z_buf;
        workspace->directory.bank_result = workspace->storage.bank_result;
        workspace->directory.pre_power_value_buf = workspace->storage.pre_power_value_buf;
        workspace->directory.protect_area = workspace->storage.protect_area;
        init_result = LHDC_CONST(STATUS_OK);
    }
    LHDC_FN(upmix_lib_reset_cache_buffer)(addr_mem);
    LHDC_FN(upmix_lib_reset_setting)(addr_mem);
    return init_result;
}
