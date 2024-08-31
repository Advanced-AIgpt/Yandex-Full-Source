#!/bin/sh -e

cd /home/app || exit 125
supervisord -c /home/app/supervisord.conf
