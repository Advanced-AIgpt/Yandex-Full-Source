#!/bin/bash

pbjs \
 -t static-module \
 -w commonjs \
 --path ../../ \
 -o ./src/protos/index.js \
 --keep-case \
 alice/protos/api/renderer/api.proto \
 alice/megamind/protos/scenarios/response.proto \
 alice/megamind/protos/scenarios/combinator_request.proto \
 alice/megamind/protos/scenarios/request.proto &&
pbts -o src/protos/index.d.ts src/protos/index.js

protoc \
 --plugin=./node_modules/.bin/protoc-gen-ts_proto \
 --ts_proto_out=./src/protos \
 --proto_path=../../ \
 --ts_proto_opt=outputClientImpl=false \
 --ts_proto_opt=outputPartialMethods=false \
 --ts_proto_opt=useOptionals=true \
 alice/megamind/protos/common/frame.proto
# uncomment on migration from pbjs to  ts-proto for every proto
# alice/protos/api/renderer/api.proto \
# alice/megamind/protos/scenarios/response.proto \
# alice/megamind/protos/scenarios/combinator_request.proto \
# alice/megamind/protos/scenarios/request.proto
