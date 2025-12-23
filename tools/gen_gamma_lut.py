#!/usr/bin/env python3
"""
Generate a 256-entry gamma lookup table for FastLED-style 8-bit values.

Usage examples:
  python tools/gen_gamma_lut.py
  python tools/gen_gamma_lut.py --gamma 2.8
  python tools/gen_gamma_lut.py --name gamma8 --progmem
  python tools/gen_gamma_lut.py --format csv

Notes:
- Output values are clamped to [0..255].
- The table maps input v (0..255) -> round(((v/255)^gamma) * 255).
"""

from __future__ import annotations

import argparse
import math
import sys
from typing import List


def build_gamma_lut(gamma: float) -> List[int]:
    if gamma <= 0.0:
        raise ValueError('gamma must be > 0')
    out: List[int] = []
    for i in range(256):
        x = i / 255.0
        y = math.pow(x, gamma)
        v = int(y * 255.0 + 0.5)
        if v < 0:
            v = 0
        elif v > 255:
            v = 255
        out.append(v)
    return out


def emit_c_array(values: List[int], name: str, progmem: bool, include_helper: bool) -> str:
    # Use "static" so the output is safe to paste into a header file.
    # With PROGMEM, the table stays in flash (not RAM).
    qual = 'static const uint8_t '
    if progmem:
        qual = 'static const uint8_t PROGMEM '
    lines = [f'{qual}{name}[256] = {{']
    for row in range(0, 256, 16):
        chunk = values[row : row + 16]
        lines.append('    ' + ', '.join(f'{v:3d}' for v in chunk) + ',')
    lines.append('};')
    if include_helper:
        lines.append('')
        lines.append(f'static inline uint8_t {name}_read(uint8_t v) {{')
        if progmem:
            lines.append(f'    return pgm_read_byte(&{name}[v]);')
        else:
            lines.append(f'    return {name}[v];')
        lines.append('}')
    return '\n'.join(lines)


def emit_csv(values: List[int]) -> str:
    return ','.join(str(v) for v in values)


def main(argv: List[str]) -> int:
    p = argparse.ArgumentParser()
    p.add_argument('--gamma', type=float, default=2.2, help='Gamma value (default: 2.2)')
    p.add_argument('--name', type=str, default='gamma8', help='C array name (default: gamma8)')
    p.add_argument('--progmem', action='store_true', help='Emit PROGMEM-qualified table (Arduino)')
    p.add_argument(
        '--helper',
        action='store_true',
        help='Also emit a small helper function to read a value (uses pgm_read_byte for PROGMEM).',
    )
    p.add_argument(
        '--format',
        choices=['c', 'csv'],
        default='c',
        help='Output format (default: c)',
    )
    args = p.parse_args(argv)

    values = build_gamma_lut(args.gamma)
    if args.format == 'csv':
        sys.stdout.write(emit_csv(values) + '\n')
    else:
        sys.stdout.write(emit_c_array(values, args.name, args.progmem, args.helper) + '\n')
    return 0


if __name__ == '__main__':
    raise SystemExit(main(sys.argv[1:]))


