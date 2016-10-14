#!/bin/sh

SCRIPT_DIR="$( cd "$( dirname "$0" )" && pwd )";
SCRIPT_DIR="$( readlink -f $SCRIPT_DIR )";
cd "$SCRIPT_DIR";

export PROJECT_INSTALL_DIR=$(cd ../.. && pwd);

chmod +x $(find ../../ -name "*.sh")
$PROJECT_INSTALL_DIR/atframe/atproxy/bin/stop.sh ;

$PROJECT_INSTALL_DIR/atframe/atgateway/bin/stop-1.sh ;
$PROJECT_INSTALL_DIR/atframe/atgateway/bin/stop-2.sh ;
$PROJECT_INSTALL_DIR/atframe/atgateway/bin/stop-99.sh ;

$PROJECT_INSTALL_DIR/echosvr/bin/stop.sh ;
$PROJECT_INSTALL_DIR/gamesvr/bin/stop-1.sh ;
$PROJECT_INSTALL_DIR/loginsvr/bin/stop-1.sh ;