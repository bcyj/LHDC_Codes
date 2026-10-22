foreach(v v4 v5)
  lhdc_sources(sources ar ${v})
  add_library(lhdc_ar_${v}_objects OBJECT ${sources})
  lhdc_version_objects(lhdc_ar_${v}_objects ${v})
endforeach()
add_library(lhdc_ar_common_objects OBJECT src/pcm/pcm.c src/pcm/pcm_backend.c
  src/common/vbap_angle.c src/common/gain_smoothing.c src/effects/hrir/direction_tables.c
  src/effects/convolution/abir_tables.c src/effects/upmix/upmix_tables.c src/core/parameter_defaults.c)
lhdc_compile_settings(lhdc_ar_common_objects)
set_target_properties(lhdc_ar_common_objects PROPERTIES POSITION_INDEPENDENT_CODE ON)
target_include_directories(lhdc_ar_common_objects PRIVATE include "${LHDC_ROOT}/../ar/include")
add_library(lhdc_ar STATIC $<TARGET_OBJECTS:lhdc_ar_v4_objects>
  $<TARGET_OBJECTS:lhdc_ar_v5_objects> $<TARGET_OBJECTS:lhdc_ar_common_objects>)
add_library(lhdc::ar ALIAS lhdc_ar)
target_include_directories(lhdc_ar PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include"
  "${LHDC_ROOT}/../ar/include")
target_link_libraries(lhdc_ar PUBLIC audio_math::math lhdc_platform lhdc_dsp)
if(LHDC_BUILD_TESTS)
  add_executable(lhdc_ar_public_api tests/public_api.c)
  lhdc_compile_settings(lhdc_ar_public_api)
  target_link_libraries(lhdc_ar_public_api PRIVATE lhdc::ar)
  add_test(NAME lhdc_ar_public_api COMMAND lhdc_ar_public_api)
  add_subdirectory(tests)
endif()
