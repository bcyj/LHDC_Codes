/* Only the test target replaces system resolution; product builds have no fault-injection switch. */
#include <windows.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
static int phase;
/* Simulate successful or failed module lookup by test phase. */
static HMODULE WINAPI test_get_module(LPCWSTR name) {
    (void)name;
    return phase ? (HMODULE)(uintptr_t)1 : NULL;
}
/* Simulate module-load failure to verify math-backend error handling. */
static HMODULE WINAPI test_load_library(LPCWSTR name, HANDLE file, DWORD flags) {
    (void)name;
    (void)file;
    (void)flags;
    return NULL;
}
/* Return input plus one as an identifiable sine-resolution test entry. */
static double __cdecl test_sin(double value) {
    return value + 1;
}
/* Return input minus one as an identifiable cosine-resolution test entry. */
static double __cdecl test_cos(double value) {
    return value - 1;
}
/* Simulate missing symbols or return the appropriate numeric test entry by test phase. */
static FARPROC WINAPI test_get_proc(HMODULE module, LPCSTR name) {
    (void)module;
    if (phase == 1)
        return NULL;
    return (FARPROC)(name[0] == 's' ? test_sin : test_cos);
}
#define GetModuleHandleW test_get_module
#define LoadLibraryExW test_load_library
#define GetProcAddress test_get_proc
#include "../../../math/platform/windows/crt_math.c"

/* Verify math-runtime load failures, missing symbols, and successful retry results. */
int main(void) {
    for (phase = 0; phase < 2; ++phase) {
        if (audio_math_windows_math_ready() || !isnan(audio_math_windows_sin(0)) || !isnan(audio_math_windows_cos(0)))
            return 1;
    }
    if (!audio_math_windows_math_ready() || audio_math_windows_sin(2) != 3 || audio_math_windows_cos(2) != 1)
        return 2;
    puts("PASS Windows math backend failure and retry");
    return 0;
}
