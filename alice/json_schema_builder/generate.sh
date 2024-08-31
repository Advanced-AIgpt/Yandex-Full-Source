#!/bin/bash -ex

cd "$(dirname "$0")"

ya make tool
ya svn up ../../portal/morda-schema

./tool/tool generate-cpp --namespace NAlice::NJsonSchemaBuilder::NDiv2 --clean-directory --generate-log-id-filler ../../portal/morda-schema/div/2 div2
./tool/tool generate-cpp --namespace NAlice::NJsonSchemaBuilder::NDiv1 --clean-directory ../../portal/morda-schema/div/1 div1

ya make -t div1 div2
