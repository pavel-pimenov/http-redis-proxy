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
