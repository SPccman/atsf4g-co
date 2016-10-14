#!/bin/sh

SCRIPT_DIR="$( cd "$( dirname "$0" )" && pwd )";
SCRIPT_DIR="$( readlink -f $SCRIPT_DIR )";
cd "$SCRIPT_DIR";

export PROJECT_INSTALL_DIR=$(cd ../.. && pwd);

chmod +x ./stop_all.sh;
./stop_all.sh;

echo "wait 3s for server to stop";
sleep 3;

$PROJECT_INSTALL_DIR/atframe/atproxy/bin/start.sh ;

$PROJECT_INSTALL_DIR/atframe/atgateway/bin/start-1.sh ;
$PROJECT_INSTALL_DIR/atframe/atgateway/bin/start-2.sh ;
$PROJECT_INSTALL_DIR/atframe/atgateway/bin/start-99.sh ;

$PROJECT_INSTALL_DIR/echosvr/bin/start.sh ;
$PROJECT_INSTALL_DIR/gamesvr/bin/start-1.sh ;
$PROJECT_INSTALL_DIR/loginsvr/bin/start-1.sh ;