#!/bin/bash

# script to run Amelie Dev bot
# go to https://t.me/{bot}?start={code} when loggin in


ARCADIA_ROOT="${ARCADIA_ROOT:-.}"

if [[ -z $AMELIE_TGBOT_TOKEN ]]; then
    AMELIE_TGBOT_TOKEN=$(ya vault get version ver-01g48tpnkcvk5v55fzwwy4n0c6 -o "telegram.dev.token")
    echo "Using default dev bot https://t.me/AmelieLocalDevBot. Set AMELIE_TGBOT_TOKEN evn to use custom bot"
fi

mkdir -p tvmcache

ya make -r "${ARCADIA_ROOT}/alice/amelie/cmd/amelie"

MONGO_DB_PASSWORD=$(ya vault get version ver-01f7brjg5p1p96r9xjpy87yw1f -o "mongo.password") \
PASSPORT_CLIENT_ID=$(ya vault get version ver-01f7brjg5p1p96r9xjpy87yw1f -o "passport.alicebot.client_id") \
PASSPORT_CLIENT_SECRET=$(ya vault get version ver-01f7brjg5p1p96r9xjpy87yw1f -o "passport.alicebot.client_secret") \
STAFF_TOKEN=$(ya vault get version ver-01f7brjg5p1p96r9xjpy87yw1f -o "staff.oauth_token") \
TELEGRAM_TOKEN=$AMELIE_TGBOT_TOKEN \
TVM_SECRET=$(ya vault get version ver-01f4c978yrrmd7dw8ws0jwa9xg -o "client_secret") \
"${ARCADIA_ROOT}/alice/amelie/cmd/amelie/amelie" -c "${ARCADIA_ROOT}/alice/amelie/configs/dev.yaml"
