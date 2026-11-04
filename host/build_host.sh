#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-only
# Build the Ombak host simulator (the whole app on this machine) into build/host/ombak_host.
# Same sources as the firmware, -ffp-contract=off like the device (no fused multiply-add).
set -e
cd "$(dirname "$0")/.."
CC="${CC:-cc}"
mkdir -p build/gen build/host
[ -f build/gen/felucca_font.h ] || python3 tools/gen_font.py build/gen/felucca_font.h >/dev/null
python3 tools/gen_dsp_tables.py build/gen/dsp_tables.h >/dev/null
python3 tools/gen_logo.py build/gen/logo.h >/dev/null
python3 tools/gen_ongkara.py build/gen/ongkara.h >/dev/null
python3 tools/gen_words.py firmware/words.tsv web/words build/gen/ann_words.h >/dev/null
U="firmware/src/dsp/bars.c firmware/src/dsp/fxbus.c firmware/src/dsp/spacevb.c firmware/src/dsp/dsp_tables.c
   firmware/src/dsp/master.c firmware/src/app/gamelan.c firmware/src/app/engine.c"
# shellcheck disable=SC2086
$CC -O2 -ffp-contract=off -std=c99 -Wall -Wextra -Wno-unused-function -Wno-unused-parameter \
    -DOMB_HOST ${OMB_HOST_DEFS:-} -Ifirmware/src -Ifirmware/src/dsp -Ibuild/gen -o ${OMB_HOST_OUT:-build/host/ombak_host} \
    host/ombak_host.c $U -lm
echo "host: build/host/ombak_host"
