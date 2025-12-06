# http-redis-proxy

sudo apt install libcpprest-dev libhiredis-dev libmicrohttpd-dev libjsoncpp-dev libcurl4-openssl-dev python3-pip
sudo apt-get install apache2-utils

Perfect! The docker-compose.yml has been updated successfully:

- Replaced Redis with Valkey (using `valkey/valkey:alpine` image)

- Added a custom network configuration with a specific subnet (`172.25.0.0/16`) to avoid Docker's address pool exhaustion error

- All containers are now running:

  - Valkey (Redis-compatible database) on port 6379
  - l2-proxy on port 8888
  - l2-server on port 3000
  - l2-worker (running in the background)

The services are properly interconnected and should work as intended for the HTTP-Redis proxy system.

## benchmark.py

Python скрипт с асинхронным тестированием:

- Использует aiohttp для HTTP запросов
- Поддерживает параметры командной строки: URL, количество запросов, одновременность
- Выводит подробную статистику: среднее время, процентили, статус-коды
- По умолчанию тестирует [](http://0.0.0.0:8888/)<http://0.0.0.0:8888/> с 1000 запросами и 50 одновременными

## benchmark.sh

Shell скрипт с Apache Bench:

- Использует утилиту `ab` для тестирования
- Аналогичные параметры командной строки
- Сохраняет данные о времени ответа в TSV файл для дальнейшего анализа
- Проверяет наличие `ab` перед запуском

Оба скрипта готовы к использованию. Для запуска Python скрипта потребуется aiohttp (`pip install aiohttp`).


Добавлен сервис Traefik, который слушает порт 443 и проксирует HTTPS-запросы на localhost к сервису l2-proxy на порт 8888. Конфигурация проверена на валидность с помощью docker-compose config.

## Added Metrics

The following Prometheus counters with the `l2_proxy_` prefix have been added:

1. __`l2_proxy_client_requests_total`__ - Total number of client requests received
2. __`l2_proxy_redis_requests_total`__ - Total number of Redis operations performed
3. __`l2_proxy_client_request_errors_total`__ - Total number of client request errors
4. __`l2_proxy_redis_errors_total`__ - Total number of Redis operation errors

## Implementation Details

- __Prometheus Registry__: Created a global registry to manage all metrics

- __Exposer__: Added a Prometheus HTTP exposer on port 9090 at `/metrics` endpoint

- __Counter Integration__:

  - Client requests are counted in `RequestHandler::handle_request()`
  - Redis operations are counted for all Redis commands (GET, SET, RPUSH, INCR, PING)
  - Errors are tracked for both client requests (when Redis push fails) and Redis operations (when commands fail or return unexpected types)

- __Health Endpoint__: Added `/health` handler that performs Redis PING and counts it as a Redis operation

## Endpoints

- __Main Service__: `http://0.0.0.0:8888` (existing HTTP proxy)
- __Metrics__: `http://0.0.0.0:9090/metrics` (new Prometheus metrics endpoint)
- __Health Check__: `http://0.0.0.0:8888/health` (new health check endpoint)

The service has been built successfully and is ready for deployment. The metrics will help monitor client request volume, Redis operation performance, and error rates as requested.

**

## ummary of Changes

1. __Copied prometheus-cpp library__ from l2-proxy to l2-worker directory

2. __Copied CivetServer__ dependency for HTTP server functionality

3. __Updated CMakeLists.txt__ to include all necessary prometheus-cpp and CivetServer source files and include directories

4. __Modified main.cpp__ to include Prometheus metrics with:

   - Registry and counter definitions for worker-specific metrics

   - Metrics tracking:

     - `l2_worker_requests_processed_total`: Total requests processed
     - `l2_worker_redis_operations_total`: Total Redis operations
     - `l2_worker_l2_calls_total`: Total L2 server calls
     - `l2_worker_redis_errors_total`: Redis operation errors
     - `l2_worker_l2_errors_total`: L2 server call errors

   - Counter increments at appropriate points in the code

   - Prometheus Exposer running on port 9091 (different from l2-proxy's 9090)

## Key Differences from l2-proxy

- __Port__: l2-worker exposes metrics on port 9091 instead of 9090
- __Metrics__: Worker-specific metrics focused on request processing, Redis operations, and L2 server calls
- __Same technology stack__: Uses identical prometheus-cpp library and CivetServer for HTTP serving

The implementation has been successfully compiled and is ready for deployment. The metrics will be available at `http://0.0.0.0:9091/metrics` once the worker is running.

**

I've successfully added the requested functionality to `test.py`. Here's what was implemented:

## Changes Made:

1. __Added command-line options__ for specifying JSON payload size range:

   - `--min-size`: Minimum payload size in bytes (default: 200)
   - `--max-size`: Maximum payload size in bytes (default: 1,000,000)

2. __Created a function__ `generate_random_json_payload(size_bytes)` that generates random JSON payloads of approximately the specified size in bytes.

3. __Modified payload generation__:

   - Functionality test uses a fixed 500-byte payload
   - Load test requests use random payload sizes between the specified min and max values

4. __Added necessary imports__: `argparse`, `random`, `string`

## Usage:

Run the script with default sizes (200 bytes to 1 MB):

```bash
python test.py
```

Specify custom size range:

```bash
python test.py --min-size 500 --max-size 500000
```

The script now generates random JSON messages with varying sizes within the specified range for load testing, while maintaining the original functionality for basic endpoint testing.
