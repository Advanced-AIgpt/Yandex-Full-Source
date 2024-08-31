# coding: utf-8
from __future__ import unicode_literals

from alice.nlu.py_libs.utils.lemmer import Lemmer, Inflector


def test_lemmer():
    lemmer = Lemmer(['ru', 'en'])
    word = 'по-неаполитански'

    lemms = lemmer.parse(word)
    assert lemms[0].word == word

    word = 'красное'
    lemms = lemmer.parse(word)
    assert lemms[0].normal_form == 'красный'
    assert lemms[0].tag.POS == 'ADJF'
    assert lemms[0].tag.case == 'accs'
    assert lemms[0].inflect({'femn'}).word == 'красную'


def test_inflector():
    infl = Inflector('ru')
    cases = ['nomn', 'gent', 'datv', 'accs', 'loct', 'ablt']
    result = [infl.inflect('московский государственный университет', {'sg', cs}) for cs in cases]
    assert result == [
        'московский государственный университет',
        'московского государственного университета',
        'московскому государственному университету',
        'московский государственный университет',
        'московском государственном университете',
        'московским государственным университетом'
    ]


def test_inflector_spaces():
    infl = Inflector('ru')
    cases = ['nomn', 'gent', 'datv', 'accs', 'loct', 'ablt']
    result = [infl.inflect(' \n \n московский государственный университет \n \n ', {'sg', cs}) for cs in cases]
    assert result == [
        'московский государственный университет',
        'московского государственного университета',
        'московскому государственному университету',
        'московский государственный университет',
        'московском государственном университете',
        'московским государственным университетом'
    ]


def test_inflector_geo():
    infl = Inflector('ru')
    cases = ['nomn', 'gent', 'datv', 'accs', 'loct', 'ablt']
    result = [infl.inflect('улицу льва толстого', {'sg', cs}) for cs in cases]
    assert result == [
        'улица льва толстого',
        'улицы льва толстого',
        'улице льва толстого',
        'улицу льва толстого',
        'улице льва толстого',
        'улицей льва толстого'
    ]


def test_inflector_geo_to_nominal():
    infl = Inflector('ru')
    cities = ['москве', 'питере', 'сан-франциско', 'нальчике', 'набережных челнах', 'магадане', 'лондоне']
    cities_nominal = [infl.inflect(city, {'nomn'}) for city in cities]
    assert cities_nominal == [
        'москва',
        'питер',
        'сан-франциско',
        'нальчик',
        'набережные челны',
        'магадан',
        'лондон'
    ]


def test_inflector_pluralize():
    infl = Inflector('ru')
    numbers = [1, 2, 5, 27]
    result = [infl.pluralize('бакинский комиссар', n) for n in numbers]
    assert result == [
        'бакинский комиссар',
        'бакинских комиссара',
        'бакинских комиссаров',
        'бакинских комиссаров',
    ]


def test_inflector_singularize():
    infl = Inflector('ru')
    assert infl.singularize('колобочков', 5) == 'колобочек'


def test_inflector_pluralize_with_case():
    infl = Inflector('ru')
    case = 'acc'
    numbers = [1, 2, 5, 27]
    result = [infl.pluralize('бакинская сосиска', n, case) for n in numbers]
    assert result == [
        'бакинскую сосиску',
        'бакинские сосиски',    # "я купил две бакинские сосиски" is a valid sentence
        'бакинских сосисок',
        'бакинских сосисок',
    ]


def test_lemmer_pluralize():
    lemmer = Lemmer(['ru', 'en'])
    word = 'сосиска'
    lemma = lemmer.parse(word)[0]
    numbers = [1, 2, 5, 27]
    result = [lemma.make_agree_with_number(n).word for n in numbers]
    assert result == [
        'сосиска',
        'сосиски',
        'сосисок',
        'сосисок'
    ]


def test_multi_call():
    lemmer = Lemmer(['ru', 'en'])
    words = ['сосиска', 'сарделька', 'пончик', 'греча', 'кура']
    from multiprocessing.pool import ThreadPool

    def check_single_case(word):
        result = lemmer.parse(word)[0]
        assert result.word == word

    pool = ThreadPool(20)
    pool.map(check_single_case, words * 2000)
