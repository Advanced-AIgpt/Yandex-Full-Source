#!/bin/bash

OPTIND=1

query=""
client="android" # "ios", "stroka"
os="Android"
exp=""

while getopts "h?q:c:e:" opt; do
    case "$opt" in
        h|\?)
            echo "./$0 -q <query> [-c <client>] [-e <experiment>]*"
            exit 0
            ;;
        q)
            query="$OPTARG"
            ;;
        c)
            client="$OPTARG"
            ;;
        e)
            if [ ! -z $exp ]; then
                exp="${exp}, "
            fi
            exp="${exp}'${OPTARG}'"
            ;;
    esac
done

if [ -z "$query" ]; then
    echo "query not set"
    exit 1
fi

case "$client" in
    "android")
        client="ru.yandex.searchplugin"
        os="Android"
        ;;
    "ios")
        client="ru.yandex.mobile"
        os="iPhone"
        ;;
    "stroka")
        client="winsearchbar"
        os="Windows"
        ;;
    "quasar")
        client="ru.yandex.quasar"
        os="Android"
        ;;
    *)
        echo "unknown client '$client'"
        exit 1
esac

request='
{
    "form": {
        "name": "personal_assistant.scenarios.search",
        "slots": [
            {
                "name": "query",
                "optional": false,
                "type": "string",
                "value": "'${query}'"
            }
        ]
    },
    "meta": {
        "experiments": ['${exp}'],
        "user_agent": "Mozilla/5.0 ('${os}'; CPU OS 7_0 like Mac OS X) AppleWebKit/537.51.1 (KHTML, like Gecko) Version/7.0 Mobile/11A465 Safari/9537.53",
        "client_id": "'${client}'/7.30 (LInux; '${os}' 5.1.1;)",
        "epoch": 1486036540,
        "location": {
            "lat": 55.665589,
            "lon": 37.561917
        },
        "tz": "Europe/Moscow",
        "uid": null,
        "uuid": "276756316",
        "utterance": "'${query}'"
    }
}'

host='bass-prod.n.yandex-team.ru'
#host='bass-testing.n.yandex-team.ru'
host='fuzzy.search.yandex.net:12345'
#host='localhost:12345'

curl -s -d "$request" "http://$host/vins" | jq .

