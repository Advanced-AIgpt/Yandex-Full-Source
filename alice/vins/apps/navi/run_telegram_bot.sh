#!/bin/bash

#bot @NaviDevBot
export VINS_DEFAULT_LANG='ru'

BOT_BIN="$( cd "$(dirname "$0")" ; pwd -P )/telegram_bot/telegram_bot"
$BOT_BIN --telegram_token="294987032:AAFhAzsr5RrQW9D6Ady2LXDBio4kFm6wkaU"
