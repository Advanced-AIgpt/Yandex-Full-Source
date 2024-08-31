#!/bin/bash

# set -x

export TVM_SECRET=fake
# export TVM_SECRET=$(ya vault get version ver-01ehcmxj6b1774y000nn9v73v4 --only-value client_secret)

rm -rf matrix.evlog matrix.rtlog service_tickets
./matrix run -c config.json
