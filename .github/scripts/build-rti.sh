#!/bin/bash
pushd core/federated/RTI
mkdir build
cd build
if [[ "$OSTYPE" == "darwin"* ]]; then
    export OPENSSL_ROOT_DIR="/usr/local/opt/openssl"
fi
cmake "$@" ../
make
popd
