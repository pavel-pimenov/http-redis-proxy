#!/bin/bash

export OPENOBSERVE_URL=localhost
export OPENOBSERVE_LOGIN=admin@example.com
export OPENOBSERVE_PASSWORD=admin

curl -u 'admin@example.com:securepassword' \
  -H "Content-Type: application/json" \
  -d '[{
    "trace_id": "4bf92f3577b34da6a3ce929d0e0e4736",
    "span_id": "00f067aa0ba902b7",
    "parent_span_id": "",
    "name": "Test Span",
    "start_time": 1700000000000000,
    "end_time": 1700000000123456,
    "service_name": "test-service",
    "attributes": {"test": "true"}
  }]' \
  http://localhost:5080/api/default/traces/_json
  

docker compose down
docker compose build
docker compose up --remove-orphans

