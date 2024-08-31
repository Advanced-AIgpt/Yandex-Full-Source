#!/bin/bash

#bot @NaviDevTrBot
export VINS_DEFAULT_LANG='tr'

BOT_BIN="$( cd "$(dirname "$0")" ; pwd -P )/telegram_bot/telegram_bot"
$BOT_BIN --telegram_token="245883888:AAGJQ0qfqJxUJJE8zJmr-VKBVxXMzK1bpeY"
