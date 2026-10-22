set(objects)
foreach(c encoder ar decoder)
  string(TOUPPER "${c}" upper)
  if(LHDC_BUILD_${upper})
    if(c STREQUAL "decoder")
      list(APPEND objects $<TARGET_OBJECTS:lhdc_decoder_v4_objects> $<TARGET_OBJECTS:lhdc_decoder_v5_objects>)
    else()
      list(APPEND objects $<TARGET_OBJECTS:lhdc_${c}_v4_objects> $<TARGET_OBJECTS:lhdc_${c}_v5_objects>)
    endif()
  endif()
endforeach()
if(LHDC_BUILD_AR)
  list(APPEND objects $<TARGET_OBJECTS:lhdc_ar_common_objects>)
endif()
if(LHDC_BUILD_ENCODER OR LHDC_BUILD_AR OR LHDC_BUILD_DECODER)
  list(APPEND objects $<TARGET_OBJECTS:lhdc_dsp_objects>)
endif()
# Include every object: unresolved references cannot hide in unused archive members.
add_executable(lhdc_component_smoke component_smoke.c ${objects})
target_include_directories(lhdc_component_smoke PRIVATE
  "${LHDC_ROOT}/encoder/include" "${LHDC_ROOT}/ar/include" "${LHDC_ROOT}/decoder/include"
  "${LHDC_ROOT}/../ar/include")
target_compile_definitions(lhdc_component_smoke PRIVATE
  LHDC_TEST_DECODER=$<BOOL:${LHDC_BUILD_DECODER}>
  LHDC_TEST_ENCODER_WITH_AR=$<BOOL:${LHDC_ENCODER_WITH_AR}>
  LHDC_TEST_ENCODER=$<BOOL:${LHDC_BUILD_ENCODER}> LHDC_TEST_AR=$<BOOL:${LHDC_BUILD_AR}>)
lhdc_compile_settings(lhdc_component_smoke)
target_link_libraries(lhdc_component_smoke PRIVATE lhdc_platform lhdc_common)
add_test(NAME lhdc_component_smoke COMMAND lhdc_component_smoke)

# Also exercise the actual public targets and their transitive dependencies.
add_executable(lhdc_component_consumer component_smoke.c)
target_compile_definitions(lhdc_component_consumer PRIVATE
  LHDC_TEST_DECODER=$<BOOL:${LHDC_BUILD_DECODER}>
  LHDC_TEST_ENCODER_WITH_AR=$<BOOL:${LHDC_ENCODER_WITH_AR}>
  LHDC_TEST_ENCODER=$<BOOL:${LHDC_BUILD_ENCODER}> LHDC_TEST_AR=$<BOOL:${LHDC_BUILD_AR}>)
lhdc_compile_settings(lhdc_component_consumer)
if(LHDC_BUILD_ENCODER)
  target_link_libraries(lhdc_component_consumer PRIVATE lhdc::encoder)
endif()
if(LHDC_BUILD_AR)
  target_link_libraries(lhdc_component_consumer PRIVATE lhdc::ar)
endif()
add_test(NAME lhdc_component_consumer COMMAND lhdc_component_consumer)

if(LHDC_BUILD_DECODER)
  target_link_libraries(lhdc_component_consumer PRIVATE lhdc::decoder)
endif()

if(LHDC_BUILD_ENCODER AND LHDC_BUILD_DECODER AND LHDC_BUILD_AR)
  find_package(Threads REQUIRED)
  add_executable(lhdc_instance_isolation instance_isolation.c)
  lhdc_compile_settings(lhdc_instance_isolation)
  target_link_libraries(lhdc_instance_isolation PRIVATE lhdc::encoder lhdc::decoder lhdc::ar Threads::Threads)
  add_test(NAME lhdc_instance_isolation COMMAND lhdc_instance_isolation)
endif()

if(LHDC_BUILD_ENCODER AND LHDC_BUILD_DECODER)
  add_executable(lhdc_llac_quantization llac_quantization.c)
  lhdc_compile_settings(lhdc_llac_quantization)
  target_link_libraries(lhdc_llac_quantization PRIVATE lhdc::encoder lhdc::decoder)
  add_test(NAME lhdc_llac_quantization COMMAND lhdc_llac_quantization)
endif()

if(LHDC_BUILD_ENCODER)
  add_executable(lhdc_encoder_errors encoder_errors.c)
  target_compile_definitions(lhdc_encoder_errors PRIVATE LHDC_CODE_VERSION=4 LHDC_COMBINED_NAMESPACE=1)
  lhdc_compile_settings(lhdc_encoder_errors)
  target_link_libraries(lhdc_encoder_errors PRIVATE lhdc::encoder)
  add_test(NAME lhdc_encoder_errors COMMAND lhdc_encoder_errors)
  add_executable(lhdc_host_memory host_memory.c)
  lhdc_compile_settings(lhdc_host_memory)
  target_link_libraries(lhdc_host_memory PRIVATE lhdc::encoder)
  target_compile_definitions(lhdc_host_memory PRIVATE
    LHDC_TEST_DECODER=$<BOOL:${LHDC_BUILD_DECODER}>
    LHDC_TEST_ENCODER_WITH_AR=$<BOOL:${LHDC_ENCODER_WITH_AR}>)
  if(LHDC_BUILD_DECODER)
    target_link_libraries(lhdc_host_memory PRIVATE lhdc::decoder)
  endif()
  add_test(NAME lhdc_host_memory COMMAND lhdc_host_memory)
endif()
