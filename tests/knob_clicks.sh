#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-only
# The knobs turned fast over a steady sound (tests/scenarios/knobs.pen), each recording checked for clicks:
# a knob whose setting jumps in the audio path shows here
cd "$(dirname "$0")/.."
sh tests/run_scenarios.sh knobs >/dev/null || { echo "knobs.pen failed"; exit 1; }
FAIL=0
for f in build/scenarios/knobs/*.wav; do
    python3 tests/click_check.py "$f" || FAIL=1
done
exit $FAIL
