foreach(v v4 v5)
  lhdc_sources(sources encoder ${v})
  if(NOT LHDC_ENCODER_WITH_AR)
    list(APPEND sources "${CMAKE_CURRENT_SOURCE_DIR}/src/compat/${v}/ar_disabled.c")
  endif()
  add_library(lhdc_encoder_${v}_objects OBJECT ${sources})
  lhdc_version_objects(lhdc_encoder_${v}_objects ${v})
  target_compile_definitions(lhdc_encoder_${v}_objects PRIVATE
    LHDC_ENCODER_WITH_AR=$<BOOL:${LHDC_ENCODER_WITH_AR}>)
  if(v STREQUAL "v4")
    target_compile_definitions(lhdc_encoder_${v}_objects PRIVATE
      LHDC_ENCODER_HIGH_PERFORMANCE_SSE=$<BOOL:${LHDC_ENCODER_HIGH_PERFORMANCE_SSE}>
      LLAC_QUANT_FAST_RECIPROCAL=$<BOOL:${LHDC_ENCODER_HIGH_PERFORMANCE_SSE}>
      LHDC_LLAC_FAST_RECIPROCAL=$<BOOL:${LHDC_ENCODER_HIGH_PERFORMANCE_SSE}>)
  endif()
  if(v STREQUAL "v5")
    target_compile_definitions(lhdc_encoder_${v}_objects PRIVATE
      LHDC_V5_ENCODER_FIXED_FFT=$<BOOL:${LHDC_V5_ENCODER_FIXED_FFT}>)
  endif()
  if(NOT LHDC_ENCODER_WITH_AR)
    set(disabled "${CMAKE_CURRENT_SOURCE_DIR}/src/compat/${v}/ar_disabled.h")
    if(MSVC)
      target_compile_options(lhdc_encoder_${v}_objects PRIVATE "/FI${disabled}")
    else()
      target_compile_options(lhdc_encoder_${v}_objects PRIVATE "SHELL:-include \"${disabled}\"")
    endif()
  endif()
  target_include_directories(lhdc_encoder_${v}_objects PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/include/lhdc/encoder"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/${v}/include")
  if(ANDROID)
    lhdc_android_encoder_sources(${sources})
  endif()
endforeach()
add_library(lhdc_encoder STATIC $<TARGET_OBJECTS:lhdc_encoder_v4_objects>
  $<TARGET_OBJECTS:lhdc_encoder_v5_objects>)
add_library(lhdc::encoder ALIAS lhdc_encoder)
target_include_directories(lhdc_encoder PUBLIC "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>")
target_link_libraries(lhdc_encoder PUBLIC lhdc_platform lhdc_dsp lhdc_common)
if(LHDC_ENCODER_WITH_AR)
  target_link_libraries(lhdc_encoder PUBLIC lhdc_ar)
endif()

if(LHDC_BUILD_TESTS)
  add_subdirectory(tests)
endif()
