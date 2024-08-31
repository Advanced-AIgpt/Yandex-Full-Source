#!/bin/bash

startdate='2019-08-04'
enddate='2020-01-30'

enddate=$( date -d "$enddate + 1 day" +%Y-%m-%d )   # rewrite in YYYYMMDD format
                                                  #  and take last iteration into account
thedate=$( date -d "$startdate" +%Y-%m-%d )

# date, from which we have AppId, so that we can filter data by needed app ids
newlogdate=$( date -d "2019-12-12" +%Y-%m-%d )

while [ "$thedate" != "$enddate" ]; do
    now=$(date +"%Y-%m-%d %T")

    printf '%s: working with "%s" table\n' "$now" "$thedate"
    
    if [[ "$thedate" < "$newlogdate" ]]; then
        ./uniproxy-sessionlog-vins-parser --src "//home/logfeller/logs/alice-production-uniproxy/1d/"$thedate --dest-vins-input "//home/alice/irinfox/VA-1190/vins_data_for_empty_asr/"$thedate --dest "//home/voice/irinfox/VA-1190/some_data/"$thedate --pool "voice"
    else
        ./uniproxy-sessionlog-vins-parser --src "//home/logfeller/logs/alice-production-uniproxy/1d/"$thedate --appid-vins-input "ru.yandex.searchplugin,ru.yandex.mobile,ru.yandex.mobile.search,ru.yandex.mobile.search.ipad,com.yandex.browser,YaBro,winsearchbar,com.yandex.launcher,ru.yandex.quasar.services,aliced,ru.yandex.quasar.app" --dest-vins-input "//home/alice/irinfox/VA-1190/vins_data_for_empty_asr/"$thedate --dest "//home/voice/irinfox/VA-1190/some_data/"$thedate --pool "voice"
    fi

    thedate=$( date -d "$thedate + 1 days" +%Y-%m-%d ) # increment by one day
done
