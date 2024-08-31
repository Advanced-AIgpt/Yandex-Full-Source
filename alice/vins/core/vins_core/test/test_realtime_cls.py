# coding: utf-8
from __future__ import unicode_literals

from vins_core.nlu.sample_processors.expected_value import RealTimeTextClassifier


def test_cls():
    c = RealTimeTextClassifier(
        [
            ['пойти в кино', 'пойти в кино'],
            ['сходить в театр', 'сходить в театр'],
            ['поесть пиццу', 'поесть пиццу'],
            ['приготовить пиццу', 'приготовить пиццу'],
            ['пойти на прогулку', 'пойти на прогулку'],
            ['пойти в парк', 'пойти в парк'],
            ['бегать в парке', 'бегать в парке'],
        ]
    )
    assert c.predict('кино')[0] == 'пойти в кино'
    assert c.predict('театр')[0] == 'сходить в театр'
    assert c.predict('поесть')[0] == 'поесть пиццу'
    assert c.predict('поесть')[1] > .8
    assert c.predict('пойти') == (None, 0)
    assert c.predict('на прогулку было бы здорово')[0] == 'пойти на прогулку'
    assert c.predict('сегодня наверно кино')[0] == 'пойти в кино'


def test_cls_2(samples_extractor):
    data = [
        '"Хайвей 1 Гб" (абонентская плата 200.00 ₽/мес)',
        '"Хайвей 4 Гб" (абонентская плата 400.00 ₽/мес)',
        '"Хайвей 4 Гб (плата в сутки)" (абонентская плата 18.00 ₽/сут)',
        '"Хайвей 8 Гб" (абонентская плата 600.00 ₽/мес)',
        '"Хайвей 12 Гб" (абонентская плата 700.00 ₽/мес)',
        '"Хайвей 20 Гб" (абонентская плата 1200.00 ₽/мес)',
        '"Хайвей 1Гб (плата в сутки)" (абонентская плата 7.00 ₽/сут, подключение 200.00 ₽)',
        '"Хайвей 1 Гб (неделя бесплатно)" (абонентская плата 7.00 ₽/сут)',
    ]
    samples = samples_extractor(data)
    c = RealTimeTextClassifier([(' '.join(s.tokens), ' '.join(s.tokens)) for s in samples])
    assert c.predict('8 гб за 600')[0] == 'хайвей 8 гб абонентская плата 600.00 ₽ мес'
    assert c.predict('8 гб')[0] == 'хайвей 8 гб абонентская плата 600.00 ₽ мес'
    assert c.predict('4 гб сутки')[0] == 'хайвей 4 гб плата в сутки абонентская плата 18.00 ₽ сут'


def test_speed():
    import timeit

    N = 100
    t = timeit.timeit('RealTimeTextClassifier(data)',
                      number=N, setup='''\
from vins_core.ner.fst_normalizer import DEFAULT_RU_NORMALIZER_NAME
from vins_core.nlu.sample_processors.expected_value import RealTimeTextClassifier
from vins_core.nlu.samples_extractor import SamplesExtractor
from vins_core.nlu.sample_processors.normalize import NormalizeSampleProcessor


samples_extractor = SamplesExtractor([NormalizeSampleProcessor(DEFAULT_RU_NORMALIZER_NAME)])

data = [
    'пойти в кино',
    'сходить в театр',
    'поесть пиццу',
    'приготовить пиццу',
    'пойти на прогулку',
    'пойти в парк',
    'бегать в парке',
]
data = map(lambda s: (' '.join(s.tokens), ' '.join(s.tokens)), samples_extractor(data))
    ''')
    assert t/N < 0.05
