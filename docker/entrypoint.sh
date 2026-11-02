#!/bin/sh
set -e
SCENARIO="${TIVIDY_SCENARIO:-acme-software.txt}"
mkdir -p /data
if [ ! -f "/data/$SCENARIO" ]; then
  cp /app/scenarios-default/acme-software.txt "/data/$SCENARIO"
  echo "Created /data/$SCENARIO from the demo company"
fi
for port in 8080 8081 8082; do
  socat "TCP-LISTEN:$port,fork,reuseaddr" "TCP:127.0.0.1:$((port + 10000))" &
done
echo "TIVIDY in Docker: open http://localhost:8080, http://localhost:8081 and http://localhost:8082"
exec /app/bin/TIVIDY_Server --ports 18080,18081,18082 --web /app/web --scenarios /data --scenario "/data/$SCENARIO"
