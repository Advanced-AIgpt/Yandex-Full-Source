#!/bin/bash

set -e


if [[ -z "$MONGO_HOST" && -z "$VINS_MONGODB_URL" && -z "$VINS_USE_DUMMY_STORAGES" ]]; then
      echo " \$MONGO_HOST and \$VINS_MONGODB_URL not exist and \$VINS_USE_DUMMY_STORAGES not true. Nothing to do..."
      exit 100
fi

if [[ -z "$VINS_MONGODB_URL" && -z "$VINS_USE_DUMMY_STORAGES" ]]; then
    if [[ -z "$MONGO_HOST" || -z "$MONGO_DB" || -z "$MONGO_USER" || -z "$MONGO_PASSWORD" ]]; then
        echo "ERROR: \$MONGO_HOST, \$MONGO_DB, \$MONGO_USER, \$MONGO_PASSWORD are required."
        exit 100
    fi

    ENCODED_MONGO_PASSWORD=`python -c "import urllib; print urllib.quote_plus('$MONGO_PASSWORD')"`
    export VINS_MONGODB_URL="mongodb://${MONGO_USER}:${ENCODED_MONGO_PASSWORD}@${MONGO_HOST}/${MONGO_DB}"
fi


##
if [[ -z "$RUN_COMMAND" ]]; then
    echo "\$RUN_COMMAND enviroment variable is not set. Nothing to do..."
    exit 100
fi

cat <<EOF > /etc/supervisor/conf.d/vins.conf
[program:vins]
command = ${RUN_COMMAND}
autostart = true
autorestart = true
stderr_logfile=/dev/stderr
stderr_logfile_maxbytes=0
stdout_logfile=/dev/stdout
stdout_logfile_maxbytes=0
EOF


# redis
cat <<EOF > /home/vins/redis.conf
bind 127.0.0.1 ::1
port 6379
tcp-backlog 511
unixsocket /tmp/redis.sock
unixsocketperm 700
tcp-keepalive 300
daemonize no
supervised auto
logfile ""
rdbcompression yes
dir /home/vins/
maxclients 10000
maxmemory 4294967296
maxmemory-policy noeviction
EOF

export VINS_REDIS_SOCK=/tmp/redis.sock

cat <<EOF > /etc/supervisor/conf.d/redis.conf
[program:redis]
command = redis-server /home/vins/redis.conf
autostart = true
autorestart = true
stderr_logfile=/dev/stderr
stderr_logfile_maxbytes=0
stdout_logfile=/dev/stdout
stdout_logfile_maxbytes=0
EOF


## run supervisor
exec supervisord -n -c /etc/supervisor/supervisord.conf
