# coding=utf-8
import requests
import sys
import argparse
import codecs


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('-o', '--output', dest='output', help='')
    parser.add_argument('-d', '--date', dest='date', help='')
    parser.add_argument('-u', '--user', dest='user', help='user name')
    parser.add_argument('-p', '--password', dest='password', help='clickhouse secret')
    parser.add_argument('-a', '--app', dest='app', help='pp or navi', default='pp')
    context = parser.parse_args(sys.argv[1:])

    if context.app == "pp":
      request = "SELECT StartDate, uniqIf(DeviceIDHash, SessionType = 0) " \
             "FROM mobile.events_all " \
             "WHERE (StartDate = toDate('"+context.date+"')) AND ((AppID = 'ru.yandex.searchplugin' AND length(splitByChar('.', AppVersionName)) > 1 AND (toUInt16OrZero(splitByChar('.', AppVersionName)[1]) > 7 OR (splitByChar('.', AppVersionName)[1] = '7' AND toUInt16OrZero(splitByChar('.', AppVersionName)[2]) >= 6))) OR (AppID = 'ru.yandex.mobile' AND toUInt32OrZero(AppVersionName) >= 3050000)) AND (APIKey IN (10321, 42989, 5746))" \
             "GROUP BY StartDate " \
             "FORMAT JSONCompact"
    elif context.app == "navi":
      request = "SELECT StartDate, uniqIf(DeviceIDHash, SessionType = 0) " \
             "FROM mobile.events_all " \
             "WHERE (StartDate = toDate('"+context.date+"')) AND (APIKey = 30488) AND (substring(AppVersionName, 1, 1) = '3') " \
             "GROUP BY StartDate " \
             "FORMAT JSONCompact"
    else:
      raise ValueError, 'bad app name'

    r = requests.post('http://mtsmart.yandex.ru:8123/', auth=(context.user, context.password), timeout=1500, data=request)
    if r.status_code == 200:
        with codecs.open(context.output, 'w', encoding='utf-8') as f:
            f.write(r.text)
    else:
        raise ValueError, r.text


if __name__ == "__main__":
    main()
