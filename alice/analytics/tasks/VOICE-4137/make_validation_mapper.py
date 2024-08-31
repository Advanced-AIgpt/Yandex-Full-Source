#!/usr/bin/env python
# encoding: utf-8
import json
from urllib2 import urlopen


VALIDATION = json.load(open('tmp/full_mapsyari_validation.json'))

SELECTED_500 = json.load(open('tmp/mapsyari_validation_500_mds.json'))


def selected_names(uploaded_wavs=SELECTED_500):
    return {wav['initialFileName'].split('_')[-1].split('.')[0]
            for wav in uploaded_wavs}


#print selected_names()

def download(url):
    while True:
        try:
            return unicode(urlopen(url).read().strip(), 'utf-8')
        except Exception, err:
            print 'ERR:', err


def make_mapping(names, full_validation=VALIDATION):
    objects = ((obj['initialFileName'].split('.'),
                obj['downloadUrl'])
               for obj in full_validation)
    return {name: download(url)
            for (name, ext), url in objects
            if ext == 'txt' and name in names}


if __name__ == '__main__':
    with open('tmp/mapsyari_validation_500_map.json', 'w') as out:
        json.dump(make_mapping(selected_names()), out, indent=2)

