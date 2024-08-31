# coding: utf-8
from __future__ import unicode_literals

from vins_tools.nlu.ner.fst_utils import put_case, put_cases, put_genders_cases


def test_put_case():
    assert put_case('петр', 'accs') == 'петра'


def test_put_cases():
    assert put_cases('петр петрович петров') == [
        'петр петрович петров',
        'петра петровича петрова',
        'петру петровичу петрову',
        'петра петровича петрова',
        'петром петровичем петровым',
        'петре петровиче петрове',
    ]


def test_put_genders_cases():
    assert put_genders_cases('завтрашний') == [
        u'завтрашний', u'завтрашнего', u'завтрашнему',
        u'завтрашний', u'завтрашним', u'завтрашнем',
        u'завтрашняя', u'завтрашней', u'завтрашней',
        u'завтрашнюю', u'завтрашней', u'завтрашней',
        u'завтрашнее', u'завтрашнего', u'завтрашнему',
        u'завтрашнее', u'завтрашним', u'завтрашнем',
        u'завтрашние', u'завтрашних', u'завтрашним',
        u'завтрашние', u'завтрашними', u'завтрашних'
        ]
