#!/usr/bin/env bash

if [[ "status" == "$1" ]]; then
    test "$(curl -s http://localhost:84/ready)" = "OK"
elif [[ "stop" == "$1" ]]; then
    sleep 10
    while true; do
        unified_agent_idle=$(curl localhost:84/check_backlog 2>/dev/null)
        if [ "$unified_agent_idle" = "IDLE" ]; then
            echo "Unified Agent is idle"
            break
        else
            echo "Unified Agent is busy, waiting..."
            sleep 1
        fi
    done
else
    ./bin/unified_agent --config ./nanny/unified_agent_config_prod.yaml 1>> /logs/unified_agent.out 2>> ./logs/unified_agent.err
fi
