#include "lhdc_memory.h"

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include "../include/encoder_internal.h"
#include "../../../../common/include/dsp/fft_float32.h"
#include <audio_math/bits.h>
#if defined(_KERNEL_MODE) && defined(_MSC_VER) && !defined(__clang__) &&                           \
    (defined(_M_IX86) || defined(_M_X64))
#include <audio_math/float_trig.h>
#endif
#include "../include/codec/codec_internal.h"
#include <math.h>
#include <string.h>

/* Adapt version state to a read-only plan view and invoke the common KISS FFT executor. */
void v4f_kf_work(kiss_fft_cpx *Fout, const kiss_fft_cpx *f, size_t fstride, int32_t in_stride,
                 int32_t *factors, Pkiss_fft_state_v4 st) {
    const LhdcKissFftPlan plan = {st->nfft, st->inverse, st->twiddles, 1, NULL};
    lhdc_kiss_fft_work(Fout, f, fstride, in_stride, factors, &plan);
}

/* Calculate dynamic configuration size and initialize trailing twiddles; support queries, caller memory, and heap allocation. */
kiss_fft_cfg_v4 v4f_kiss_fft_alloc(int32_t nfft, int32_t inverse_fft, void *mem, size_t *lenmem) {
    if (nfft < 1 || (size_t)nfft > (SIZE_MAX - sizeof(kiss_fft_state_v4)) / sizeof(kiss_fft_cpx)) {
        if (lenmem)
            *lenmem = 0;
        return NULL;
    }
    size_t memneeded = sizeof(kiss_fft_state_v4) + (size_t)nfft * sizeof(kiss_fft_cpx);
    kiss_fft_cfg_v4 st;
    if (!lenmem)
        st = lhdc_memory_alloc(NULL, memneeded);
    else {
        st = mem && *lenmem >= memneeded ? mem : NULL;
        *lenmem = memneeded;
    }
    if (!st)
        return NULL;
    st->nfft = nfft;
    st->inverse = inverse_fft;
    lhdc_kiss_fft_twiddles(nfft, inverse_fft, st->twiddles);
    const int32_t threshold = (int32_t)audio_math_sqrt_floor_u32((uint32_t)nfft);
    lhdc_kiss_fft_factor(nfft, threshold, st->factors);
    return st;
}

/* Execute strided complex FFT; in-place transforms use separate output scratch. */
void v4f_kiss_fft_stride(kiss_fft_cfg_v4 st, const kiss_fft_cpx *fin, kiss_fft_cpx *fout,
                         int32_t in_stride) {
    if (fin != fout) {
        v4f_kf_work(fout, fin, 1, in_stride, st->factors, st);
        return;
    }
    kiss_fft_cpx *tmpbuf = lhdc_memory_alloc(NULL, (size_t)st->nfft * sizeof *tmpbuf);
    if (!tmpbuf)
        return;
    v4f_kf_work(tmpbuf, fin, 1, in_stride, st->factors, st);
    memcpy(fout, tmpbuf, (size_t)st->nfft * sizeof *tmpbuf);
    lhdc_memory_free(tmpbuf);
}

/* Execute forward/inverse FFT on contiguous complex input. */
void v4f_kiss_fft(kiss_fft_cfg_v4 cfg, const kiss_fft_cpx *fin, kiss_fft_cpx *fout) {
    v4f_kiss_fft_stride(cfg, fin, fout, 1);
}

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include "../include/encoder_internal.h"
#include <audio_math/constants.h>
#include "../include/codec/codec_internal.h"
#include <math.h>

/* Allocate one real-FFT block containing complex subtransform, scratch, and super-twiddles. */
kiss_fftr_cfg v4f_kiss_fftr_alloc(int32_t nfft, int32_t inverse_fft, void *mem, size_t *lenmem) {
    if (nfft < 2 || (nfft & 1) ||
        (size_t)nfft > (SIZE_MAX - sizeof(kiss_fftr_state) - sizeof(kiss_fft_state_v4)) /
                           (3 * sizeof(kiss_fft_cpx))) {
        if (lenmem)
            *lenmem = 0;
        return NULL;
    }
    int32_t ncfft = nfft / 2;
    size_t subsize;
    v4f_kiss_fft_alloc(ncfft, inverse_fft, NULL, &subsize);
    size_t memneeded = sizeof(kiss_fftr_state) + subsize +
                       ((size_t)ncfft * 2 + (size_t)ncfft / 2) * sizeof(kiss_fft_cpx);
    if (!lenmem)
        mem = lhdc_memory_alloc(NULL, memneeded);
    else {
        size_t available = *lenmem;
        *lenmem = memneeded;
        if (available < memneeded)
            return NULL;
    }
    kiss_fftr_cfg st = mem;
    if (!st)
        return NULL;
    st->substate = (kiss_fft_cfg_v4)(st + 1);
    st->tmpbuf = (kiss_fft_cpx *)((uint8_t *)st->substate + subsize);
    st->super_twiddles = st->tmpbuf + ncfft;
    st->sample_pairs = st->super_twiddles + ncfft / 2;
    v4f_kiss_fft_alloc(ncfft, inverse_fft, st->substate, &subsize);
    if (!lhdc_kiss_fftr_copy_twiddles(nfft, inverse_fft, st->super_twiddles)) {
        for (int32_t i = 0; i < ncfft / 2; ++i) {
            double phase = ((double)(i + 1) / (double)ncfft + .5) * -AUDIO_MATH_PI_F64;
            if (inverse_fft)
                phase = -phase;
#if defined(_KERNEL_MODE) && defined(_MSC_VER) && !defined(__clang__) &&                           \
    (defined(_M_IX86) || defined(_M_X64))
            /* Preserve double phase and float store boundaries rather than substituting the complex FFT's different phase formula. */
            audio_math_sincos_bounded_f64_to_f32_soft(phase, &st->super_twiddles[i].i,
                                                &st->super_twiddles[i].r);
#else
            double sine, cosine;
            audio_math_sincos(phase, &sine, &cosine);
            st->super_twiddles[i] = (kiss_fft_cpx){(float)cosine, (float)sine};
#endif
        }
    }
    return st;
}

/* Transform real time samples to a half-spectrum including DC and Nyquist. */
int32_t v4f_kiss_fftr(kiss_fftr_cfg st, const float *timedata, kiss_fft_cpx *freqdata) {
    if (!st || !st->substate || !timedata || !freqdata || st->substate->inverse)
        return LHDC_V4_STATUS_ERROR;
    int32_t ncfft = st->substate->nfft;
    for (int32_t k = 0; k < ncfft; ++k)
        st->sample_pairs[k] = (kiss_fft_cpx){timedata[2 * k], timedata[2 * k + 1]};
    v4f_kiss_fft(st->substate, st->sample_pairs, st->tmpbuf);
    kiss_fft_cpx tdc = st->tmpbuf[0];
    freqdata[0] = (kiss_fft_cpx){tdc.r + tdc.i, 0.0f};
    freqdata[ncfft] = (kiss_fft_cpx){tdc.r - tdc.i, 0.0f};
    for (int32_t k = 1; k <= ncfft / 2; ++k) {
        kiss_fft_cpx fpk = st->tmpbuf[k], fpnk = st->tmpbuf[ncfft - k];
        kiss_fft_cpx f1k = {fpk.r + fpnk.r, fpk.i - fpnk.i};
        kiss_fft_cpx f2k = {fpk.r - fpnk.r, fpk.i + fpnk.i};
        kiss_fft_cpx coef = st->super_twiddles[k - 1];
        kiss_fft_cpx tw = {f2k.r * coef.r - f2k.i * coef.i, f2k.i * coef.r + f2k.r * coef.i};
        freqdata[k] = (kiss_fft_cpx){(f1k.r + tw.r) * .5f, (f1k.i + tw.i) * .5f};
        freqdata[ncfft - k] = (kiss_fft_cpx){(f1k.r - tw.r) * .5f, (tw.i - f1k.i) * .5f};
    }
    return LHDC_V4_STATUS_OK;
}


/* Merge conjugate half-spectra and execute unnormalized complex inverse FFT. */
int32_t v4f_kiss_fftri(kiss_fftr_cfg st, const kiss_fft_cpx *freqdata, float *timedata) {
    if (!st || !st->substate || !freqdata || !timedata || !st->substate->inverse)
        return LHDC_V4_STATUS_ERROR;
    int32_t ncfft = st->substate->nfft;
    st->tmpbuf[0] =
        (kiss_fft_cpx){freqdata[0].r + freqdata[ncfft].r, freqdata[0].r - freqdata[ncfft].r};
    for (int32_t k = 1; k <= ncfft / 2; ++k) {
        kiss_fft_cpx fk = freqdata[k], fnkc = freqdata[ncfft - k];
        kiss_fft_cpx fek = {fk.r + fnkc.r, fk.i - fnkc.i};
        kiss_fft_cpx diff = {fk.r - fnkc.r, fk.i + fnkc.i};
        kiss_fft_cpx coef = st->super_twiddles[k - 1];
        kiss_fft_cpx fok = {diff.r * coef.r - diff.i * coef.i, diff.i * coef.r + diff.r * coef.i};
        st->tmpbuf[k] = (kiss_fft_cpx){fek.r + fok.r, fek.i + fok.i};
        st->tmpbuf[ncfft - k] = (kiss_fft_cpx){fek.r - fok.r, -(fek.i - fok.i)};
    }
    v4f_kiss_fft(st->substate, st->tmpbuf, st->sample_pairs);
    for (int32_t k = 0; k < ncfft; ++k) {
        timedata[2 * k] = st->sample_pairs[k].r;
        timedata[2 * k + 1] = st->sample_pairs[k].i;
    }
    return LHDC_V4_STATUS_OK;
}
