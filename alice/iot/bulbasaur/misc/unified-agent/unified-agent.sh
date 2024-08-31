#!/usr/bin/env bash

if [ "${ENV_TYPE}" == "PRODUCTION" ]; then
  exec /usr/bin/unified_agent -c /usr/local/etc/unified-agent.production.yaml
else
  exec /usr/bin/unified_agent -c /usr/local/etc/unified-agent.beta.yaml
fi
