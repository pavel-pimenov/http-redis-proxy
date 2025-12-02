#!/bin/bash

# Подстановка переменных в nslcd.conf
envsubst < /etc/nslcd.conf.template > /etc/nslcd.conf

# Убедимся, что nslcd может читать конфиг
chmod 600 /etc/nslcd.conf
chown nslcd:nslcd /etc/nslcd.conf

# Запуск supervisord
exec /usr/bin/supervisord -c /etc/supervisor/conf.d/supervisord.conf
