#!/bin/sh

#SCRIPT_DIR="$( cd "$( dirname "$0" )" && pwd )"
SCRIPT_DIR=$HOME/git-repo/sniper-detloc

export GRAPHITE_ROOT=$SCRIPT_DIR
echo "Set sniper root to ${GRAPHITE_ROOT}"

export BENCHMARKS_ROOT=$(pwd)
echo "Benchmark root: ${BENCHMARKS_ROOT}"


