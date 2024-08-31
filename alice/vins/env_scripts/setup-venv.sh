#!/bin/bash

set -ex

TRANSPORT="skynet"
for i in "$@"
do
    case $i in
        --no-resources)
            NO_RESOURCES=1
            shift # past argument=value
            ;;
        -f|--force-resources)
            FORCE_RESOURCES=1
            shift
            ;;
        -t=*|--transport=*)
            TRANSPORT="${i#*=}"
            shift # past argument=value
            ;;
        --gpu)
            INSTALL_TF_GPU=1
            shift
            ;;
        *)
            # unknown option
            ;;
    esac
done

# compile protocol buffers
protoc -I=core/vins_core/schema/protofiles/ --python_out=core/vins_core/schema/ \
    core/vins_core/schema/protofiles/features.proto

pip install -U pip

DIRS=("core" "sdk" "api" "apps/navi" "apps/personal_assistant"
      "apps/pa_skills" "test_bot" "div2html" "tools")

PIP_PATH=`which pip`
PIP_CMD="$PIP_PATH install -i https://pypi.yandex-team.ru/simple --disable-pip-version-check"

$PIP_CMD -r test-requirements.txt

for lib in "${DIRS[@]}"
do
    $PIP_CMD -r $lib/requirements.txt
    $PIP_CMD -e $lib
done

# download resources
DOWNLOAD_RESOURCES_CMD="cit_configs/download_resources/download_resources.py --resource-base-dir $VINS_RESOURCES_PATH -t $TRANSPORT -e teamcity-agents teamcity-agents"
if [ -z $NO_RESOURCES ] ; then
    if [ -z $FORCE_RESOURCES ] ; then
        $DOWNLOAD_RESOURCES_CMD
    else
        $DOWNLOAD_RESOURCES_CMD --force
    fi
fi

if [[ ! -z $INSTALL_TF_GPU ]] ; then
    wget -6 https://proxy.sandbox.yandex-team.ru/581836801 -O tensorflow_gpu-1.8.0-cp27-cp27mu-manylinux1_x86_64.whl
    $PIP_CMD tensorflow_gpu-1.8.0-cp27-cp27mu-manylinux1_x86_64.whl
    rm tensorflow_gpu-1.8.0-cp27-cp27mu-manylinux1_x86_64.whl
fi

# build models from resources for the applications that need it - currently only PA
tools/train/compile_model_from_resources.py --app personal_assistant
