"""Collect encoder, PCM effect and shared source files by component."""
from pathlib import Path

REPO = Path(__file__).resolve().parents[1]
ROOT = REPO / 'lhdc'


def encoder_sources(version):
    sources=[p for folder in (ROOT/'encoder/src'/version, ROOT/'encoder/src/api'/version,
        ROOT/'encoder/src/compat'/version/'bt') for p in folder.rglob('*.c')]
    return sorted(sources)


def ar_sources(version):
    """LHDC AR algorithm profile plus codec-specific adapters."""
    return ar_profile_sources()+[ROOT/'ar/src/compat'/version/'encoder_api.c']


def ar_profile_sources():
    return sorted(p for folder in (ROOT/'ar/src/api',ROOT/'ar/src/core',ROOT/'ar/src/effects',
        ROOT/'ar/src/compat/legacy') for p in folder.rglob('*.c')
        if p.name not in {'direction_tables.c','abir_tables.c','upmix_tables.c','parameter_defaults.c'})


def ar_shared_sources():
    return [ROOT/'ar'/p for p in ['src/pcm/pcm.c','src/pcm/pcm_backend.c','src/common/vbap_angle.c',
        'src/common/gain_smoothing.c','src/effects/hrir/direction_tables.c',
        'src/effects/convolution/abir_tables.c','src/effects/upmix/upmix_tables.c',
        'src/core/parameter_defaults.c']]
