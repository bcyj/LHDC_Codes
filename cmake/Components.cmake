if(ANDROID)
  include("${LHDC_ROOT}/platform/android/Android.cmake")
endif()

# Collect component sources from their implementation and API directories.
function(lhdc_sources out component version)
  if(component STREQUAL "encoder")
    file(GLOB_RECURSE result CONFIGURE_DEPENDS
      "${LHDC_ROOT}/encoder/src/${version}/*.c"
      "${LHDC_ROOT}/encoder/src/api/${version}/*.c"
      "${LHDC_ROOT}/encoder/src/compat/${version}/bt/*.c")
  elseif(component STREQUAL "ar")
    file(GLOB_RECURSE result CONFIGURE_DEPENDS "${LHDC_ROOT}/ar/src/api/*.c"
      "${LHDC_ROOT}/ar/src/core/*.c" "${LHDC_ROOT}/ar/src/effects/*.c"
      "${LHDC_ROOT}/ar/src/compat/legacy/*.c" "${LHDC_ROOT}/ar/src/compat/${version}/*.c")
    list(REMOVE_ITEM result "${LHDC_ROOT}/ar/src/core/parameter_defaults.c"
      "${LHDC_ROOT}/ar/src/effects/hrir/direction_tables.c"
      "${LHDC_ROOT}/ar/src/effects/convolution/abir_tables.c"
      "${LHDC_ROOT}/ar/src/effects/upmix/upmix_tables.c")
  else()
    message(FATAL_ERROR "Unknown LHDC component: ${component}")
  endif()
  list(REMOVE_DUPLICATES result)
  set(${out} "${result}" PARENT_SCOPE)
endfunction()

function(lhdc_compile_settings target)
  target_link_libraries(${target} PRIVATE audio_math::math)
  target_compile_definitions(${target} PRIVATE LHDC_ENABLE_LOGGING=$<BOOL:${LHDC_ENABLE_LOGGING}>)
  target_compile_features(${target} PRIVATE c_std_11)
  if(MSVC)
    target_compile_options(${target} PRIVATE /O2 /fp:strict /utf-8)
    if(CMAKE_SIZEOF_VOID_P EQUAL 4)
      target_compile_options(${target} PRIVATE /arch:SSE2)
    endif()
    target_compile_definitions(${target} PRIVATE _CRT_SECURE_NO_WARNINGS _USE_MATH_DEFINES)
  else()
    target_compile_options(${target} PRIVATE -O2 -ffp-contract=off -fno-fast-math
      -Wunused-variable -Wunused-function -Wunused-const-variable -Wparentheses -Wuninitialized)
    if(WIN32)
      target_compile_options(${target} PRIVATE -fno-strict-aliasing)
      target_compile_definitions(${target} PRIVATE __USE_MINGW_ANSI_STDIO=1)
    endif()
  endif()
endfunction()

function(lhdc_version_objects target version)
  lhdc_compile_settings(${target})
  lhdc_resource_settings(${target})
  set_target_properties(${target} PROPERTIES POSITION_INDEPENDENT_CODE ON)
  string(SUBSTRING "${version}" 1 1 number)
  target_compile_definitions(${target} PRIVATE LHDC_CODE_VERSION=${number}
    LHDC_COMBINED_NAMESPACE=1 LHDC_BT_USE_AOSP_REFERENCE=0)
  if(version STREQUAL "v4" AND NOT LHDC_V4_SHIFT_COUNT_BITS STREQUAL "AUTO")
    target_compile_definitions(${target} PRIVATE LHDC_V4_SHIFT_COUNT_BITS=${LHDC_V4_SHIFT_COUNT_BITS})
  endif()
  if(ANDROID)
    lhdc_android_compile_settings(${target})
  endif()
  target_include_directories(${target} PRIVATE "${LHDC_ROOT}/common/include"
    "${LHDC_ROOT}/platform/include" "${LHDC_ROOT}/ar/src/core/include"
    "${LHDC_ROOT}/ar/include" "${LHDC_ROOT}/../ar/include")
  set(namespace "${LHDC_ROOT}/common/include/lhdc_data_namespace.h")
  if(MSVC)
    target_compile_options(${target} PRIVATE "/FI${namespace}")
  else()
    target_compile_options(${target} PRIVATE "SHELL:-include \"${namespace}\"")
  endif()
endfunction()
