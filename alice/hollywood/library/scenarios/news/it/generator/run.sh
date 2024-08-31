#!/bin/bash

ya make -r -A --test-tag ya:manual --yt-store --test-tag ya:alice_run_request_generator -DIT2_GENERATOR \
    --test-env STUBBER_SERVER_MODE=USE_UPSTREAM_UPDATE_ALL "$@"

# More options for STUBBER_SERVER_MODE env var:
# USE_UPSTREAM_UPDATE_ALL
# USE_STUBS_UPDATE_NONE
