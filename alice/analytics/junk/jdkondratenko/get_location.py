import csv
import time
import json
import sys
from urllib.request import urlopen
import logging

logging.basicConfig(level=logging.DEBUG)
logger = logging.getLogger(__name__)
RPS_BORDER = 90


# taken from https://gist.github.com/gregburek/1441055
def rate_limit_decorator(max_per_second):
    min_interval = 1.0 / float(max_per_second)

    def decorate(func):
        last_time_called = [0.0]

        def rate_limited_function(*args, **kargs):
            elapsed = time.process_time() - last_time_called[0]
            left_to_wait = min_interval - elapsed
            if left_to_wait > 0:
                time.sleep(left_to_wait)
            ret = func(*args, **kargs)
            last_time_called[0] = time.process_time()
            return ret

        return rate_limited_function

    return decorate


@rate_limit_decorator(RPS_BORDER)
def download_url(url, attempts=3):
    counter = 0
    sec = 1
    for i in range(attempts):
        if counter > 0:
            sec *= 2
            time.sleep(sec)
        try:
            # если всё прошло успешно, то кол-во rps контролируется декоратором и мы один раз зашли в try
            # если что-то пошло не так и словили exeption, то по counter и так заснём на пару секунд и не превысим rps
            json_string = urlopen(url).read()
            return json_string
        except:
            counter += 1
    return None


def get_exact_location(lon, lat):
    host = "http://addrs-testing.search.yandex.net/upper/stable/"
    geocode_link = host + 'yandsearch?ms=json&lang=ru&mode=reverse&ll=' + str(lon) + "," + str(
        lat) + "&type=geo&origin=alice_analytics&results=1"
    geocode_res = download_url(geocode_link)
    formatted = ""
    if geocode_res:
        geocode_json = json.loads(geocode_res.decode('utf-8'))
        if geocode_json != -1 and geocode_json['features']:
            properties = geocode_json['features'][0]['properties']
            if 'GeocoderMetaData' in properties:
                meta_data = properties['GeocoderMetaData']
                kind = meta_data['kind']
                if 'Address' in meta_data:
                    formatted = meta_data['Address']['formatted']
    return formatted

with open('input_data.tsv') as csvfile:
    reader = csv.DictReader(csvfile, delimiter='\t')
    with open('test_results.tsv', mode='a', newline='', encoding='utf-8') as outfile:
        fieldnames = ['request_id', 'lon', 'lat', 'exact_location']
        writer = csv.DictWriter(outfile, fieldnames=fieldnames, delimiter='\t')
        writer.writeheader()
        for row in reader:
            location = get_exact_location(row['lon'], row['lat'])
            print(location)
            writer.writerow({'request_id': row['request_id'], 'lon': row['lon'], 'lat': row['lat'], 'exact_location': location})