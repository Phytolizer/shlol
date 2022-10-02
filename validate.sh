#!/bin/sh

PRESET=$1
OUTDIR=out/build/$PRESET

if [ ! "$PRESET" ]; then
    echo "Usage: $0 <preset>"
    exit 1
fi

cmake --build "$OUTDIR"
tests/validate "$OUTDIR"/shlol
