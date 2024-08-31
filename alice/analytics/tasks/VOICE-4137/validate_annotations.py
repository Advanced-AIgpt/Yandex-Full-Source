#!/usr/bin/env python
# encoding: utf-8
import json


VALIDATION_MAP = json.load(open('tmp/mapsyari_validation_500_map.json'))

PROD_ANNOTATIONS = map(json.loads, open('tmp/instances/prod_annotations.json').readlines())
PROD_BAD = json.load(open('tmp/instances/prod_bad.json'))


ANN_3_1_1 = map(json.loads, open('tmp/instances/ann_3+1+1.json').readlines())
BAD_3_1_1 = json.load(open('tmp/instances/bad_3+1+1.json'))

ANN_2 = map(json.loads, open('tmp/instances/ann_2.json').readlines())
BAD_2 = json.load(open('tmp/instances/ann_2_bad.json'))

UNFORMAT_2 = map(json.loads, open('tmp/instances/unformat_2.json').readlines())
UNFORMAT_BAD_2 = json.load(open('tmp/instances/unformat_2_bad.json'))

UNFORMAT_2B = map(json.loads, open('tmp/instances/unformat_2b.json').readlines())
UNFORMAT_BAD_2B = json.load(open('tmp/instances/unformat_bad_2b.json'))

UNFORMAT_3 = map(json.loads, open('tmp/instances/unformat_3.json').readlines())
UNFORMAT_BAD_3 = json.load(open('tmp/instances/unformat_bad_3.json'))


EXP_LIST = [('prod', PROD_ANNOTATIONS, PROD_BAD),
            ('311', ANN_3_1_1, BAD_3_1_1),
            ('2', ANN_2, BAD_2),
            ('unf_2', UNFORMAT_2, UNFORMAT_BAD_2),
            ('unf_2b:', UNFORMAT_2B, UNFORMAT_BAD_2B),
            ('unf_3', UNFORMAT_3, UNFORMAT_BAD_3)]


def calc_stat(annotations, bad_list, valid_map):
    stat = {
        'validation_size': len(valid_map),  # Размер валидационного сета
        'solved': 0,  # Сколько задач из валидационного сета оказались размеченными
        'match': 0,   # Сколько задач совпали разметкой с валидацией
    }
    for task in annotations:
        if task['source'] == 'validation pool':
            stat['solved'] += 1
            name = task['mds_key'].split('/')[-1].split('.')[0]  # 'mds_key': u'validation/3003505343.wav'
            if task['text'] == valid_map[name]:
                stat['match'] += 1

    stat['bad_count'] = sum(int('/validation/' in t['url']) for t in bad_list)
    stat['full_size'] = stat['bad_count'] + stat['solved']
    stat['unmarked'] = 100.0 *(stat['validation_size'] - stat['full_size']) / stat['validation_size']

    stat['match_perc'] = 100.0 * stat['match'] / stat['full_size']
    stat['ok_match_perc'] = 100.0 * stat['match'] / stat['solved']
    stat['solved_perc'] = 100.0 * stat['solved'] / stat['full_size']

    return stat


def find_matchings(annotations, valid_map):
    matched = {}  # name: value
    not_matched = {}  # name: (result_value, valid_text)
    for task in annotations:
        if task['source'] == 'validation pool':
            name = task['mds_key'].split('/')[-1].split('.')[0]  # 'mds_key': u'validation/3003505343.wav'
            if task['text'] == valid_map[name]:
                matched[name] = task
            else:
                not_matched[name] = task, valid_map[name]

    return matched, not_matched


def match_intersections(annotations1, annotations2, valid_map):
    match1, not_match1 = find_matchings(annotations1, valid_map)
    match2, not_match2 = find_matchings(annotations2, valid_map)
    m1_against_m2 = set(not_match1) - set(not_match2)
    m2_against_m1 = set(not_match2) - set(not_match1)
    print len(set(not_match1) & set(not_match2)), len(m1_against_m2), len(m2_against_m1)
    print '=== 1 ==='
    for url in m1_against_m2:
        task, valid_text = not_match1[url]
        print u'|| %s | %s | %s ||' % (task['text'], valid_text, task['mds_key'])
    print '=== 2 ==='
    for url in m2_against_m1:
        task, valid_text = not_match2[url]
        print u'|| %s | %s ||' % (task['text'], valid_text)


MOBILE_EXP_LIST = [('prod',
                    map(json.loads, open('tmp/mobile/prod_ann.json').readlines()),
                    json.load(open('tmp/mobile/prod_bad.json'))),

                   ('311',
                    map(json.loads, open('tmp/mobile/311_ann.json').readlines()),
                    json.load(open('tmp/mobile/311_bad.json')))]


MOBILE_VALIDATION_MAP = json.load(open('tmp/mobile_validation_1000_map.json'))


if __name__ == '__main__':
    # for name, ann, bad in EXP_LIST:
    #     print '%s:\t%s' % (name, calc_stat(ann, bad, VALIDATION_MAP))

    for name, ann, bad in MOBILE_EXP_LIST:
        print '%s:\t%s' % (name, calc_stat(ann, bad, MOBILE_VALIDATION_MAP))

    #match_intersections(PROD_ANNOTATIONS, COMRADE_ANNOTATIONS, VALIDATION_MAP)

    #for name, ann, bad in EXP_LIST:
    #    print ('%.1f |' % calc_stat(ann, bad, VALIDATION_MAP)['unmarked']),

    # CLEANED_2 = map(json.loads, open('tmp/cleaned/ann_2.json').readlines())
    # BAD_2 = json.load(open('tmp/instances/ann_2_bad.json'))
    # print 'cleaned:\t%s' % (calc_stat(CLEANED_2, BAD_2, VALIDATION_MAP))
