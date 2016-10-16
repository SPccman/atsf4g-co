#!/bin/sh

SCRIPT_DIR="$( cd "$( dirname "$0" )" && pwd )";
SCRIPT_DIR="$( readlink -f $SCRIPT_DIR )";
cd "$SCRIPT_DIR";

export PROJECT_INSTALL_DIR=$(cd ../../.. && pwd);


export LD_LIBRARY_PATH=$PROJECT_INSTALL_DIR/lib:$PROJECT_INSTALL_DIR/tools/shared:$LD_LIBRARY_PATH ;

PROFILE_DIR="$PROJECT_INSTALL_DIR/profile";
export GCOV_PREFIX="$PROFILE_DIR/gcov";
export GCOV_PREFIX_STRIP=16 ;
mkdir -p "$GCOV_PREFIX";

export MALLOC_CONF="stats_print:false,tcache:false";
export LD_PRELOAD=$PROJECT_INSTALL_DIR/tools/shared/libjemalloc.so;

PID_FILE=atproxy.pid;

if [ -e $PID_FILE ]; then
    PROC_PATH="/proc/$(cat $PID_FILE)";
    if [ -e "$PROC_PATH" ]; then
        ./atproxy -id 0x00000001 -c ../etc/atproxy.conf -p $PID_FILE stop
    fi
fi
