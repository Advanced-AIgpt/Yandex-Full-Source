#!/bin/bash

OPTIND=1

target=""
target_t="string" # "default_app"
target_type="" # "app", "site", "", "default_app"
client="android" # "ios", "stroka"
os="Android"

while getopts "h?t:T:c:" opt; do
    case "$opt" in
        h|\?)
            echo "./$0 -t <target> [-T <target_type>] [-c <client>]"
            exit 0
            ;;
        t)
            target="$OPTARG"
            ;;
        T)
            target_type="$OPTARG"
            ;;
        c)
            client="$OPTARG"
            ;;
    esac
done

if [ -z "$target" ]; then
    echo "target not set"
    exit 1
fi

if [ "$target_type" == "default_app" ]; then
    target_t="default_app"
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
    *)
        echo "unknown client '$client'"
        exit 1
esac

request='
{
    "form": {
        "name": "personal_assistant.scenarios.open_site_or_app",
        "slots": [
            {
                "name": "target",
                "optional": false,
                "type": "'${target_t}'",
                "value": "'${target}'"
            },
            {
                "name": "target_type",
                "optional": true,
                "type": "site_or_app",
                "value": "'${target_type}'"
            }
        ]
    },
    "meta": {
        "experiments": [],
        "user_agent": "Mozilla/5.0 ('${os}'; CPU OS 7_0 like Mac OS X) AppleWebKit/537.51.1 (KHTML, like Gecko) Version/7.0 Mobile/11A465 Safari/9537.53",
        "client_id": "'${client}'/10.00 (LInux; '${os}' 5.1.1;)",
        "epoch": 1486036540,
        "location": {
            "lat": 55.665589,
            "lon": 37.561917
        },
        "tz": "Europe/Moscow",
        "uid": null,
        "uuid": "276756316",
        "utterance": "'${target}'"
    }
}'

host='bass-testing.n.yandex-team.ru'
#host='localhost:12345'
#host='fuzzy.search.yandex.net:12345'

curl -s -d "$request" "http://$host/vins" | jq .

