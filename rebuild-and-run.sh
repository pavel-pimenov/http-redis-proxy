#!/bin/bash

export OPENOBSERVE_URL=localhost
export OPENOBSERVE_LOGIN=admin@example.com
export OPENOBSERVE_PASSWORD=admin

NOW_US=$(($(date +%s) * 1000000 + $(date +%N) / 1000))
curl -u 'admin@example.com:admin' \
  -H "Content-Type: application/json" \
  -d "[{
    \"trace_id\": \"$(openssl rand -hex 16)\",
    \"span_id\": \"$(openssl rand -hex 8)\",
    \"name\": \"Manual Test\",
    \"start_time\": $((NOW_US - 10000)),
    \"end_time\": $NOW_US,
    \"service_name\": \"manual\",
    \"attributes\": {}
  }]" \
  http://localhost:5080/api/default/traces/_json
  
docker compose down
docker compose build
docker compose up --remove-orphans

