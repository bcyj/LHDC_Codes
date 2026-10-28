LHDC_ANDROID_DIR := $(call my-dir)
LHDC_PROJECT_ROOT := $(abspath $(LHDC_ANDROID_DIR)/../../..)
AUDIO_COMMON_ROOT ?= $(abspath $(LHDC_PROJECT_ROOT)/../audio_codec_common)
LOCAL_PATH := $(LHDC_PROJECT_ROOT)
include $(LHDC_ANDROID_DIR)/sources.mk

LHDC_ENABLE_LOGGING ?= 0
LHDC_V5_ENCODER_FIXED_FFT ?= 0
LHDC_EXTERNAL_ALLOCATOR_ONLY ?= 0
LHDC_KISS_FFT_ITERATIVE_STATIC ?= 0
LHDC_DRIVER_RESOURCES ?= 0
LHDC_ANDROID_BUILD_BUNDLE ?= 0
ifeq ($(LHDC_DRIVER_RESOURCES),1)
  LHDC_EXTERNAL_ALLOCATOR_ONLY := 1
  LHDC_KISS_FFT_ITERATIVE_STATIC := 1
  LHDC_ENABLE_LOGGING := 0
endif

LHDC_ANDROID_CFLAGS := -std=c11 -O2 -ffp-contract=off -fno-fast-math \
  -Wunused-variable -Wunused-function -Wunused-const-variable -Wparentheses -Wuninitialized -D_GNU_SOURCE=1 \
  -DLHDC_ENABLE_LOGGING=$(LHDC_ENABLE_LOGGING) \
  -DLHDC_EXTERNAL_ALLOCATOR_ONLY=$(LHDC_EXTERNAL_ALLOCATOR_ONLY) \
  -DLHDC_KISS_FFT_ITERATIVE_STATIC=$(LHDC_KISS_FFT_ITERATIVE_STATIC) \
  -DAUDIO_MATH_FFT_ITERATIVE_STATIC=$(LHDC_KISS_FFT_ITERATIVE_STATIC) \
  -DLHDC_V5_ENCODER_FIXED_FFT=$(LHDC_V5_ENCODER_FIXED_FFT) \
  -DLHDC_DRIVER_RESOURCES=$(LHDC_DRIVER_RESOURCES)
LHDC_ANDROID_INCLUDES := $(AUDIO_COMMON_ROOT)/math/include $(AUDIO_COMMON_ROOT)/core/include \
  $(addprefix $(LOCAL_PATH)/,ar/include \
  lhdc/common/include lhdc/platform/include lhdc/ar/include lhdc/ar/src/core/include \
  lhdc/encoder/include/lhdc/encoder lhdc/decoder/include)

# Arguments: module, inventory key, version (empty for shared data), ARM mode.
define lhdc_android_static
include $$(CLEAR_VARS)
LOCAL_MODULE := $(1)
LOCAL_SRC_FILES := $$(LHDC_SOURCES_$(2))
LOCAL_C_INCLUDES := $$(LHDC_ANDROID_INCLUDES)
LOCAL_CFLAGS := $$(LHDC_ANDROID_CFLAGS)
LOCAL_ARM_MODE := $(if $(4),$(4),thumb)
ifeq ($$(TARGET_ARCH_ABI),armeabi-v7a)
  LOCAL_CFLAGS += -mfpu=$(if $(filter arm,$(4)),neon,vfpv3-d16) -mfloat-abi=softfp
endif
ifneq ($(3),)
  LOCAL_CFLAGS += -DLHDC_CODE_VERSION=$(3) -DLHDC_COMBINED_NAMESPACE=1 \
    -DLHDC_BT_USE_AOSP_REFERENCE=0 \
    -include $$(LOCAL_PATH)/lhdc/common/include/lhdc_data_namespace.h
  LOCAL_C_INCLUDES += $$(LOCAL_PATH)/lhdc/encoder/src/v$(3)/include \
    $$(LOCAL_PATH)/lhdc/decoder/src/v$(3)/include \
    $$(LOCAL_PATH)/lhdc/decoder/src/api/v$(3)/include
endif
include $$(BUILD_STATIC_LIBRARY)
endef

$(eval $(call lhdc_android_static,lhdc_ndk_math,MATH))
$(eval $(call lhdc_android_static,lhdc_ndk_common,COMMON))
$(eval $(call lhdc_android_static,lhdc_ndk_dsp,DSP))
$(eval $(call lhdc_android_static,lhdc_ndk_ar_data,AR_SHARED))
$(eval $(call lhdc_android_static,lhdc_ndk_pcm,PCM))
$(foreach v,4 5,$(eval $(call lhdc_android_static,lhdc_ndk_encoder_v$(v),ENCODER_V$(v),$(v))))
$(foreach v,4 5,$(eval $(call lhdc_android_static,lhdc_ndk_ar_v$(v),AR_V$(v),$(v))))
$(foreach v,4 5,$(eval $(call lhdc_android_static,lhdc_ndk_bt_v$(v),BT_V$(v),$(v),arm)))
$(foreach v,4 5,$(eval $(call lhdc_android_static,lhdc_ndk_decoder_v$(v),DECODER_V$(v),$(v))))

LHDC_ANDROID_COMMON_LIBS := lhdc_ndk_ar_data lhdc_ndk_dsp lhdc_ndk_common lhdc_ndk_math

# Arguments: SO name, whole archives, ordinary archives, shared dependencies.
define lhdc_android_shared
include $$(CLEAR_VARS)
LOCAL_MODULE := $(1)
LOCAL_WHOLE_STATIC_LIBRARIES := $(2)
LOCAL_STATIC_LIBRARIES := $(3)
LOCAL_SHARED_LIBRARIES := $(4)
LOCAL_LDLIBS := -lm -llog
LOCAL_LDFLAGS := -Wl,-Bsymbolic -Wl,--no-undefined
include $$(BUILD_SHARED_LIBRARY)
endef

$(eval $(call lhdc_android_shared,lhdc,lhdc_ndk_encoder_v4 lhdc_ndk_ar_v4,$(LHDC_ANDROID_COMMON_LIBS)))
$(eval $(call lhdc_android_shared,lhdcv5,lhdc_ndk_encoder_v5 lhdc_ndk_ar_v5,$(LHDC_ANDROID_COMMON_LIBS)))
$(eval $(call lhdc_android_shared,lhdcBT_enc,lhdc_ndk_bt_v4,,lhdc))
$(eval $(call lhdc_android_shared,lhdcv5BT_enc,lhdc_ndk_bt_v5,,lhdcv5))
$(eval $(call lhdc_android_shared,lhdcdec,lhdc_ndk_decoder_v4,$(LHDC_ANDROID_COMMON_LIBS)))
$(eval $(call lhdc_android_shared,lhdcv5dec,lhdc_ndk_decoder_v5,$(LHDC_ANDROID_COMMON_LIBS)))

ifeq ($(LHDC_ANDROID_BUILD_BUNDLE),1)
  $(eval $(call lhdc_android_shared,lhdc_all,lhdc_ndk_encoder_v4 lhdc_ndk_encoder_v5 \
    lhdc_ndk_ar_v4 lhdc_ndk_ar_v5 lhdc_ndk_bt_v4 lhdc_ndk_bt_v5 lhdc_ndk_pcm \
    lhdc_ndk_decoder_v4 lhdc_ndk_decoder_v5,$(LHDC_ANDROID_COMMON_LIBS)))
endif
