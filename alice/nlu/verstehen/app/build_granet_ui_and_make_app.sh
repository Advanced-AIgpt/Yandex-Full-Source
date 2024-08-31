#!/usr/bin/env bash
# Use this file to build Verstehen App with Granet, as
# Granet UI is written on React, so needed to be built separately

SCRIPT_DIR="`dirname $0`"

# check yarn
if [[ -z "`command -v yarn`" ]]; then
    echo "Please install nodejs (preferably v10.19.0) and yarn (preferably 1.22.0)"
    echo "See https://github.com/nvm-sh/nvm and https://classic.yarnpkg.com/en/docs/install"
    exit 1
fi

pushd "$SCRIPT_DIR"

pushd "granet_ui"

echo "Step 1. Clear previous build artifacts"
rm -rf node_modules
rm -rf build
rm -rf ../verstehen_app

echo "Step 2. Build UI"
yarn install
yarn run build

if [[ $? != 0 ]]; then
    exit 1;
fi

popd

echo "Step 3. Ya.make app"
ya make -r --checkout ./

popd
