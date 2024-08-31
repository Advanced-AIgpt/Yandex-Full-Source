#!/bin/bash

# Use -F <test_name_pattern> arg to generate run requests for a subset of all tests
ya make -r --checkout -A --test-tag ya:manual --test-tag ya:alice_run_request_generator -DIT2_GENERATOR \
    --test-env STUBBER_SERVER_MODE=USE_UPSTREAM_UPDATE_ALL "$@"
