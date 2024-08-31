#!/usr/bin/env bash

set -ex

ya make $ARCADIA/geobase/data/tree+ling $ARCADIA/geobase/data/tzdata
mkdir -p ~/.geobase/tzdata
tar -xvf $ARCADIA/geobase/data/tzdata/tzdata.tar.gz -C ~/.geobase/tzdata
