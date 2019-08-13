#!/bin/sh

HOMEDIR=/home/agung
#SCRIPT_DIR="$( cd "$( dirname "$0" )" && pwd )"
SCRIPT_DIR=$HOMEDIR/git-repo/sniper-detloc

export GRAPHITE_ROOT=$SCRIPT_DIR
echo "Set sniper root to ${GRAPHITE_ROOT}"

export BENCHMARKS_ROOT=$SCRIPT_DIR/benchmarks
echo "Benchmark root: ${BENCHMARKS_ROOT}"


