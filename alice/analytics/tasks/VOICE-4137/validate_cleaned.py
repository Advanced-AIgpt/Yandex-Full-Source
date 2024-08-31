#!/usr/bin/env python
# encoding: utf-8
import json
from validate_annotations import calc_stat, VALIDATION_MAP


EXP_LIST = [('prod', 'tmp/cleaned/prod.json', 'tmp/cleaned/bad_prod.json'),
            ('311', 'tmp/cleaned/overlap3.json', 'tmp/cleaned/bad_overlap3.json'),
            ('2', 'tmp/cleaned/overlap2.json', 'tmp/cleaned/bad_overlap2.json'),
            ('unf_2', 'tmp/cleaned/unformat2.json', 'tmp/cleaned/bad_unformat2.json'),
            ('unf_2b:', 'tmp/cleaned/unformat2b.json', 'tmp/cleaned/bad_unformat2b.json'),
            ('unf_3', 'tmp/cleaned/unformat3.json', 'tmp/cleaned/bad_unformat3.json')]


STAT_LIST = [calc_stat(map(json.loads, open(ann).readlines()),
                       json.load(open(bad)),
                       VALIDATION_MAP)
             for name, ann, bad in EXP_LIST]


METRICS = ['solved_perc', 'match_perc', 'ok_match_perc', 'unmarked']


for m in METRICS:
    print '|| {} |'.format(m),
    print ' | '.join('{:.1f}'.format(s[m]) for s in STAT_LIST),
    print '||'
