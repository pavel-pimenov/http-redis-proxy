#!/bin/bash

export OPENOBSERVE_URL=localhost
export OPENOBSERVE_LOGIN=admin@example.com
export OPENOBSERVE_PASSWORD=admin

docker compose down
docker compose build
docker compose up --remove-orphans

