#!/bin/bash

ya make -r -A --test-tag ya:manual --test-tag ya:alice_run_request_generator -DIT2_GENERATOR \
    --test-env STUBBER_SERVER_MODE=USE_UPSTREAM_UPDATE_ALL "$@"