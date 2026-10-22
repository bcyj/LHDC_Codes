"""Build Android SOs through the single maintained ndk-build entry point."""
import argparse
import os
import hashlib
import json
from pathlib import Path
import shutil
import subprocess
from generate_android_sources import generate, REPO, ROOT


def build(ndk, output, abis, jobs, v5_fixed_fft=False, external_allocator_only=False,
          iterative_fft=False, driver_resources=False, logging=False, bundle=True):
    ndk = ndk.resolve()
    generate(check=True)
    output = output.resolve()
    output.mkdir(parents=True, exist_ok=True)
    android = ROOT / 'platform/android'
    command = [str(ndk / ('ndk-build.cmd' if os.name == 'nt' else 'ndk-build')), '-j' + str(jobs),
        'NDK_PROJECT_PATH=null', 'APP_BUILD_SCRIPT=' + str(android / 'Android.mk'),
        'NDK_APPLICATION_MK=' + str(android / 'Application.mk'),
        'NDK_OUT=' + str(output / 'obj'), 'NDK_LIBS_OUT=' + str(output / 'libs'),
        'APP_ABI=' + ' '.join('armeabi-v7a' if b == 32 else 'arm64-v8a' for b in abis),
        'LHDC_V5_ENCODER_FIXED_FFT=' + str(int(v5_fixed_fft)),
        'LHDC_EXTERNAL_ALLOCATOR_ONLY=' + str(int(external_allocator_only)),
        'LHDC_KISS_FFT_ITERATIVE_STATIC=' + str(int(iterative_fft)),
        'LHDC_DRIVER_RESOURCES=' + str(int(driver_resources)),
        'LHDC_ENABLE_LOGGING=' + str(int(logging)),
        'LHDC_ANDROID_BUILD_BUNDLE=' + str(int(bundle))]
    result = subprocess.run(command, cwd=REPO, capture_output=True)
    (output / 'ndk-build.log').write_bytes(result.stdout + result.stderr)
    if result.returncode:
        raise RuntimeError((result.stdout + result.stderr).decode('utf8', errors='replace')[-8000:])
    binaries = {}
    for bits in abis:
        destination = output / str(bits)
        destination.mkdir(exist_ok=True)
        source = output / 'libs' / ('armeabi-v7a' if bits == 32 else 'arm64-v8a')
        for p in source.glob('*.so'):
            target = destination / p.name
            shutil.copy2(p, target)
            binaries[target.relative_to(output).as_posix()] = hashlib.sha256(target.read_bytes()).hexdigest()
        print(bits, 'NDK shared libraries built', flush=True)
    (output / 'build_receipt.json').write_text(json.dumps(dict(
        command=command, exit_code=result.returncode, binary_hashes=binaries,
        decoder_api='instance', bugfix_policy='unconditional'), indent=2) + '\n', encoding='utf8')


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--ndk', type=Path, default=Path(os.environ['ANDROID_NDK_HOME']) if os.environ.get('ANDROID_NDK_HOME') else None)
    parser.add_argument('--out', type=Path, required=True)
    parser.add_argument('--abi', choices=['32', '64', 'both'], default='both')
    parser.add_argument('--jobs', type=int, default=4)
    parser.add_argument('--v5-fixed-fft', action='store_true')
    parser.add_argument('--external-allocator-only', action='store_true')
    parser.add_argument('--iterative-fft', action='store_true')
    parser.add_argument('--driver-resources', action='store_true')
    parser.add_argument('--logging', action='store_true')
    parser.add_argument('--no-bundle', action='store_true')
    args = parser.parse_args()
    if args.ndk is None:
        parser.error('Specify --ndk or set ANDROID_NDK_HOME')
    build(args.ndk, args.out, [32, 64] if args.abi == 'both' else [int(args.abi)], args.jobs,
          args.v5_fixed_fft, args.external_allocator_only, args.iterative_fft,
          args.driver_resources, args.logging, not args.no_bundle)
