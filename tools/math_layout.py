"""Collect shared math sources and resolve component-relative source paths."""
from pathlib import Path

REPO=Path(__file__).resolve().parents[1]
MODULE=REPO/'lhdc'


def math_sources(windows=False):
    result=[REPO/'math/src'/name for name in ['fma_soft.c','float_exp.c','float_sine.c',
        'fft_float32.c','fft_float32_tables.c']]
    if windows:
        result += sorted((REPO/'math/platform/windows').glob('*.c'))
    return result


def maintained_source(relative):
    if relative.startswith('math/') and (REPO/relative).is_file():
        return REPO/relative
    p=MODULE/relative
    if p.is_file():
        return p
    if relative.startswith('ar/'):
        p=REPO/relative
        if p.is_file():
            return p
    if relative=='common/src/dsp/fft_float32_tables.c':
        return REPO/'math/src/fft_float32_tables.c'
    if relative in ['common/src/dsp/fma_soft.c','common/src/dsp/float_exp.c','common/src/dsp/float_sine.c']:
        return REPO/'math/src'/Path(relative).name
    raise FileNotFoundError('Maintained source not found: '+str(p))
