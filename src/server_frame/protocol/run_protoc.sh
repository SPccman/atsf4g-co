#!/bin/sh

cd "$(dirname "$0")";
protoc -I client --cpp_out=client $(ls client/*.proto);

protoc -I config --cpp_out=config $(ls config/*.proto);

protoc -I client -I server --cpp_out=server $(ls server/*.proto);