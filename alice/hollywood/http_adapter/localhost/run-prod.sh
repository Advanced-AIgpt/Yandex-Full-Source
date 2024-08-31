#!/bin/bash -e

cd "$(dirname "$0")"

if [ -d $ARCADIA_DIR/.svn ]; then
    echo "SVN detected, will use --checkout"
    BUILD_OPTIONS="--checkout"
else
    echo "ARC detected, will not use --checkout"
    BUILD_OPTIONS=""
fi

ya make $BUILD_OPTIONS -j0 ../../../../apphost/conf
ya make $BUILD_OPTIONS ../../../../apphost/tools/app_host_launcher

export TVM_SECRET=$(ya vault get version sec-01cnbk6vvm6mfrhdyzamjhm4cm --only-value TVM2_SECRET)

rm -rf local_apphost_dir && ../../../../apphost/tools/app_host_launcher/app_host_launcher setup --nora -P 40000 --tvm-id 2000860 -y -p local_apphost_dir/ arcadia -v ALICE --target-ctype test

