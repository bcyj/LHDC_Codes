"""Collect sources from the independent audio_codec_common repository."""
import os
from pathlib import Path

REPO=Path(__file__).resolve().parents[1]
MODULE=REPO/'lhdc'
COMMON_REPO=Path(os.environ.get('AUDIO_COMMON_ROOT', REPO.parent/'audio_codec_common')).resolve()


def math_sources(windows=False):
    result=[COMMON_REPO/'math/src'/name for name in ['fma_soft.c','float_exp.c','float_sine.c',
        'fft_float32.c','fft_float32_tables.c','fft_sse.c']]
    if windows:
        result += sorted((COMMON_REPO/'math/platform/windows').glob('*.c'))
    return result


def maintained_source(relative):
    if relative.startswith('math/') and (COMMON_REPO/relative).is_file():
        return COMMON_REPO/relative
    p=MODULE/relative
    if p.is_file():
        return p
    if relative.startswith('ar/'):
        p=REPO/relative
        if p.is_file():
            return p
    if relative=='common/src/dsp/fft_float32_tables.c':
        return COMMON_REPO/'math/src/fft_float32_tables.c'
    if relative in ['common/src/dsp/fma_soft.c','common/src/dsp/float_exp.c','common/src/dsp/float_sine.c']:
        return COMMON_REPO/'math/src'/Path(relative).name
    raise FileNotFoundError('Maintained source not found: '+str(p))
