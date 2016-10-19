#!/usr/bin/env bash
<%!
    import common.project_utils as project
%>
SCRIPT_DIR="$( cd "$( dirname "$0" )" && pwd )";
SCRIPT_DIR="$( readlink -f $SCRIPT_DIR )";
cd "$SCRIPT_DIR";

SERVER_NAME="${project.get_server_name()}";
SERVERD_NAME="$SERVER_NAME";
SERVER_FULL_NAME="${project.get_server_full_name()}";
SERVER_BUS_ID=${hex(project.get_server_id())};
export PROJECT_INSTALL_DIR=$(cd ${project_install_prefix} && pwd);

source "$PROJECT_INSTALL_DIR/script/helper/common/common.sh";

if [ ! -e "$SERVERD_NAME" ]; then
    SERVERD_NAME="${project.get_server_name()}d";
fi

if [ ! -e "$SERVERD_NAME" ]; then
    ErrorMsg "Executable $SERVER_NAME or $SERVERD_NAME not found, run $@ failed";
    exit 1;
fi
SERVER_PID_FILE_NAME="$SERVER_FULL_NAME.pid";

export LD_LIBRARY_PATH=$PROJECT_INSTALL_DIR/lib:$PROJECT_INSTALL_DIR/tools/shared:$LD_LIBRARY_PATH ;

PROFILE_DIR="$PROJECT_INSTALL_DIR/profile";
export GCOV_PREFIX="$PROFILE_DIR/gcov";
export GCOV_PREFIX_STRIP=16 ;
mkdir -p "$GCOV_PREFIX";

export MALLOC_CONF="stats_print:false,tcache:false";
export LD_PRELOAD=$PROJECT_INSTALL_DIR/tools/shared/libjemalloc.so;
