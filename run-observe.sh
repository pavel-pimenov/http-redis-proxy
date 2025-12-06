ZO_ROOT_USER_EMAIL="admin@example.com" \
ZO_ROOT_USER_PASSWORD="admin" \
./openobserve

curl -u 'admin@example.com:admin' \
  -H "Content-Type: application/json" \
  -d '[{
    "trace_id": "0123456789abcdef0123456789abcdef",
    "span_id": "0123456789abcdef",
    "name": "Manual Test MSK",
    "start_time": 1765032202000000,
    "end_time":   1765032202100000,
    "service_name": "manual-test",
    "attributes": {}
  }]' \
  http://localhost:5080/api/default/traces/_json