#!/usr/bin/env bash
# ./venv/bin/python manage.py migrate --noinput
# supervisord -n
/app/venv/bin/uwsgi --ini /app/logviewer/uwsgi.ini &
/usr/sbin/nginx
