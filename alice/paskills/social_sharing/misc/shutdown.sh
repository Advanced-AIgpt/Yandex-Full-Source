#!/bin/sh -e

PORT=${QLOUD_HTTP_PORT:-8080}
echo "Executing graceful shutdown script for port $PORT"
curl -X POST "http://localhost:$PORT/actuator/shutdown" -H 'Content-Type: application/json'
