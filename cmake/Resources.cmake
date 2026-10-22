include_guard(GLOBAL)
option(LHDC_DRIVER_RESOURCES "Select iterative FFT, explicit host allocation and no logging (not a complete kernel port)" OFF)
option(LHDC_KISS_FFT_ITERATIVE_STATIC "Use iterative float KISS FFT with static tables" ${LHDC_DRIVER_RESOURCES})
option(LHDC_EXTERNAL_ALLOCATOR_ONLY "Require explicit host allocator; disable default CRT heap" ${LHDC_DRIVER_RESOURCES})
option(LHDC_ENABLE_LOGGING "Enable diagnostic logging and argument evaluation" OFF)
if(LHDC_DRIVER_RESOURCES AND
   (NOT LHDC_KISS_FFT_ITERATIVE_STATIC OR NOT LHDC_EXTERNAL_ALLOCATOR_ONLY OR LHDC_ENABLE_LOGGING))
  message(FATAL_ERROR "LHDC_DRIVER_RESOURCES requires iterative FFT ON, external allocator ON and logging OFF. Use a fresh build directory or set all three explicitly.")
endif()

# Apply to product objects, not to the intentionally recursive test oracle.
function(lhdc_resource_settings target)
  if(LHDC_DRIVER_RESOURCES)
    target_compile_definitions(${target} PRIVATE LHDC_DRIVER_RESOURCES=1
      LHDC_KISS_FFT_ITERATIVE_STATIC=1 LHDC_EXTERNAL_ALLOCATOR_ONLY=1 LHDC_ENABLE_LOGGING=0)
  endif()
endfunction()
