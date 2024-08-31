# -*- coding: utf-8 -*-
from __future__ import unicode_literals

from alice.analytics.operations.priemka.alice_parser.lib.utils import (
    parse_downloader_client_time,
    extract_key_from_mds_filename,
    split_sessions_by_duplicates,
    prepare_voice_url,
    get_voice_url_from_mds_key,
)

from alice.analytics.operations.priemka.alice_parser.utils.errors import (
    Ue2eResultError as DRE,
    get_preliminary_downloader_result,
    get_most_session_unanswer,
)
from alice.analytics.operations.priemka.alice_parser.utils.queries_utils import (
    prepare_toloka_queries,
    clean_voice_text_from_tags,
)


VOICE_TEXT_MAPPING = {
    # теги акцентов (интонационных ударений) + транскрипции
    "Какую <[ k a k ou u | accented ]> песню ?": "Какую песню ?",
    "Какую <[ accented ]> песню ?": "Какую песню ?",
    # метки про род
    "Время приготовления #fem 40 минут. Вам понадобятся венчик, форма для выпечки, #neu 3 яблока, #mas 110 грамм сахара":
        "Время приготовления 40 минут. Вам понадобятся венчик, форма для выпечки, 3 яблока, 110 грамм сахара",
    # спикер-теги
    "<speaker audio=\"shitova_skorogovorka_3.opus\">": "",
    "<speaker audio=\"coin-flip-shimmer.opus\"/>.sil<[100]><speaker voice=\"shitova.us\"> Решка.": "Решка.",
    "<speaker voice=\"alyss\" effect=\"translate_alyss_omni\" speed=\"0.9\">хуиняо": "хуиняо",
    "<speaker voice=\"kolya.gpu\">Посмотрите еще раз на рисунок.": "Посмотрите еще раз на рисунок.",
    # спикер-тег + транскрипция
    "<speaker voice=\"alyss\" effect=\"translate_alyss_omni\" speed=\"0.9\">heuschrecke <[ h oo j sh r ee k schwa ]>": "heuschrecke",
    # не портим исходный текст
    "Скажите мне: «включи Bluetooth», подключитесь к устройству и слушайте вашу музыку.":
        "Скажите мне: «включи Bluetooth», подключитесь к устройству и слушайте вашу музыку.",
    "İşte burada.": "İşte burada.",
    # метки ударений на гласные
    "Для того, чтобы люди мимо очереди не забег+али к терапевту - только спросить - в больницах перед кабинетами будут установлены \"лежачие доктор+а\"":
        "Для того, чтобы люди мимо очереди не забегали к терапевту - только спросить - в больницах перед кабинетами будут установлены \"лежачие доктора\"",
    "Бон-ж+ур.": "Бон-жур.",
    # music-тег
    "<[domain music]> Включаю подборку \"Колыбельные песни\" <[/domain]>": "Включаю подборку \"Колыбельные песни\"",
    # sil-тег
    ".sil<[1000]> До следующего выпуска. Хорошего дня.": "До следующего выпуска. Хорошего дня.",
    "Как зовут ее? Что это? sil <[500]> Если никак не можешь отгадать, скажи: \"Сдаюсь\"":
        "Как зовут ее? Что это? Если никак не можешь отгадать, скажи: \"Сдаюсь\""
}


def test_clean_voice_text_from_tags():
    for voice_text, clean_text in list(VOICE_TEXT_MAPPING.items()):
        assert clean_voice_text_from_tags(voice_text) == clean_text


def test_parse_downloader_client_time():
    assert parse_downloader_client_time('20201203T050727') == 1606972047
    assert parse_downloader_client_time('') is None
    assert parse_downloader_client_time(None) is None


def test_get_preliminary_result():
    assert get_preliminary_downloader_result('text', 'req_id', 'vins_response', 'EMPTY') is None
    assert get_preliminary_downloader_result(None, 'req_id', 'vins_response', 'EMPTY') is None
    assert get_preliminary_downloader_result('', 'req_id', 'vins_response', 'EMPTY') == DRE.NONEMPTY_SIDESPEECH_RESPONSE
    assert get_preliminary_downloader_result(None, 'req_id', 'vins_res', 'side_speech') is DRE.EMPTY_SIDESPEECH_RESPONSE
    assert get_preliminary_downloader_result('', 'req_id', 'vins_r', 'side_speech') == DRE.EMPTY_SIDESPEECH_RESPONSE
    assert get_preliminary_downloader_result('text', 'req_id', 'vins_r', 'side_speech') == DRE.EMPTY_SIDESPEECH_RESPONSE

    assert get_preliminary_downloader_result('', 'req_id', None, None) == DRE.EMPTY_VINS_RESPONSE
    assert get_preliminary_downloader_result('text', 'req_id', None, None) == DRE.EMPTY_VINS_RESPONSE

    assert get_preliminary_downloader_result('', None, 'vins_response', None) == DRE.UNIPROXY_ERROR
    assert get_preliminary_downloader_result('text', None, 'vins_response', None) == DRE.UNIPROXY_ERROR
    assert get_preliminary_downloader_result('', None, 'vins_response', 'side_speech') == DRE.UNIPROXY_ERROR
    assert get_preliminary_downloader_result('text', None, 'vins_response', 'side_speech') == DRE.UNIPROXY_ERROR
    assert get_preliminary_downloader_result('text', None, None, None) == DRE.UNIPROXY_ERROR
    assert get_preliminary_downloader_result(None, None, None, None) == DRE.UNIPROXY_ERROR


def test_prepare_voice_url():
    #  asr voice_url good, no changes
    assert prepare_voice_url(
        'https://speechbase.voicetech.yandex-team.ru/getaudio/asr-logs/2022-03-01/124029b-8fd6991d-ad710012-4545dcbe?norm=1&storage-type=s3&s3-bucket=voicelogs') == \
        'https://speechbase.voicetech.yandex-team.ru/getaudio/asr-logs/2022-03-01/124029b-8fd6991d-ad710012-4545dcbe?norm=1&storage-type=s3&s3-bucket=voicelogs'
    #  asr voice_url good, change handle
    assert prepare_voice_url(
        'https://speechbase.voicetech.yandex-team.ru/getfile/asr-logs/2022-03-01/124029b-8fd6991d-ad710012-4545dcbe?norm=1&storage-type=s3&s3-bucket=voicelogs') == \
        'https://speechbase.voicetech.yandex-team.ru/getaudio/asr-logs/2022-03-01/124029b-8fd6991d-ad710012-4545dcbe?norm=1&storage-type=s3&s3-bucket=voicelogs'
    #  empty url  - return Null
    assert prepare_voice_url('') is None
    #  empty url  - return Null
    assert prepare_voice_url(None) is None
    #  upx voice_url good, no changes
    assert prepare_voice_url(
        'https://speechbase.voicetech.yandex-team.ru/getaudio/6223579/6528c3e3-a9d6fde2-3ec160b8-90f08d79_92fcf013-9cb7-41c8-af91-0b9a7a6cdd97_1.opus?norm=1') == \
        'https://speechbase.voicetech.yandex-team.ru/getaudio/6223579/6528c3e3-a9d6fde2-3ec160b8-90f08d79_92fcf013-9cb7-41c8-af91-0b9a7a6cdd97_1.opus?norm=1'
    #  upx voice_url good, change handle and add norm
    assert prepare_voice_url(
        'https://speechbase.voicetech.yandex-team.ru/getfile/6223579/6528c3e3-a9d6fde2-3ec160b8-90f08d79_92fcf013-9cb7-41c8-af91-0b9a7a6cdd97_1.opus') == \
        'https://speechbase.voicetech.yandex-team.ru/getaudio/6223579/6528c3e3-a9d6fde2-3ec160b8-90f08d79_92fcf013-9cb7-41c8-af91-0b9a7a6cdd97_1.opus?norm=1'
    #  upx voice_url hypothetical example with extra query key
    assert prepare_voice_url(
        'https://speechbase.voicetech.yandex-team.ru/getfile/6223579/6528c3e3-a9d6fde2-3ec160b8-90f08d79_92fcf013-9cb7-41c8-af91-0b9a7a6cdd97_1.opus?kek=lol') == \
        'https://speechbase.voicetech.yandex-team.ru/getaudio/6223579/6528c3e3-a9d6fde2-3ec160b8-90f08d79_92fcf013-9cb7-41c8-af91-0b9a7a6cdd97_1.opus?norm=1&kek=lol'


def test_get_voice_url_from_mds_key():
    # asr voice
    assert get_voice_url_from_mds_key('asr-logs/2022-03-01/457eebec-510f1329-cd130a85-76a7102f') == \
           'https://speechbase.voicetech.yandex-team.ru/getaudio/asr-logs/2022-03-01/457eebec-510f1329-cd130a85-76a71' \
           '02f?norm=1&storage-type=s3&s3-bucket=voicelogs'
    # upx voice
    assert get_voice_url_from_mds_key('6223579/6528c3e3-a9d6fde2-3ec160b8-90f08d79_92fcf013-9cb7-41c8-af91-0b9a7a6cdd97_1.opus') == \
           'https://speechbase.voicetech.yandex-team.ru/getaudio/6223579/6528c3e3-a9d6fde2-3ec160b8-90f08d79_92fcf013-9cb7-41c8-af91-0b9a7a6cdd97_1.opus?norm=1'


def test_get_most_session_unanswer():
    def test_fn(results_list):
        return get_most_session_unanswer([{'result': x} for x in results_list])

    assert test_fn([]) is None
    assert test_fn([None]) is None
    assert test_fn([None, None]) is None
    assert test_fn(['something']) is None
    assert test_fn([None, 'something', None]) is None

    assert test_fn([DRE.UNIPROXY_ERROR]) is DRE.UNIPROXY_ERROR
    assert test_fn([None, DRE.UNIPROXY_ERROR, None]) is DRE.UNIPROXY_ERROR
    assert test_fn([DRE.EMPTY_VINS_RESPONSE]) is DRE.EMPTY_VINS_RESPONSE
    assert test_fn([None, DRE.EMPTY_VINS_RESPONSE, None]) is DRE.EMPTY_VINS_RESPONSE
    assert test_fn([DRE.NONEMPTY_SIDESPEECH_RESPONSE]) is None
    assert test_fn([None, DRE.NONEMPTY_SIDESPEECH_RESPONSE, None]) is None

    assert test_fn([DRE.UNIPROXY_ERROR, DRE.EMPTY_VINS_RESPONSE]) is DRE.UNIPROXY_ERROR
    assert test_fn([DRE.EMPTY_VINS_RESPONSE, DRE.UNIPROXY_ERROR]) is DRE.UNIPROXY_ERROR

    assert test_fn([DRE.NONEMPTY_SIDESPEECH_RESPONSE, DRE.EMPTY_VINS_RESPONSE]) is DRE.EMPTY_VINS_RESPONSE
    assert test_fn([
        DRE.NONEMPTY_SIDESPEECH_RESPONSE, DRE.EMPTY_VINS_RESPONSE, DRE.UNIPROXY_ERROR
    ]) is DRE.UNIPROXY_ERROR
    assert test_fn([
        None, DRE.NONEMPTY_SIDESPEECH_RESPONSE, None, None, DRE.EMPTY_VINS_RESPONSE, DRE.UNIPROXY_ERROR, None
    ]) is DRE.UNIPROXY_ERROR


def test_extract_key_from_mds_filename():
    assert extract_key_from_mds_filename(None) is None
    assert extract_key_from_mds_filename(
        '85307bbbc91ad89f6e189973dfa76288') == '85307bbbc91ad89f6e189973dfa76288'
    assert extract_key_from_mds_filename(
        'ffffffff-ffff-ffff-1387-3dc173dd5420') == 'ffffffff-ffff-ffff-1387-3dc173dd5420'
    assert extract_key_from_mds_filename(
        'screenshots/85307bbbc91ad89f6e189973dfa76288.png') == '85307bbbc91ad89f6e189973dfa76288'
    assert extract_key_from_mds_filename(
        'screenshots/ffffffff-ffff-ffff-1387-3dc173dd5420.png') == 'ffffffff-ffff-ffff-1387-3dc173dd5420'


def test_prepare_toloka_queries():
    assert list(prepare_toloka_queries(None)) == [None]
    assert list(prepare_toloka_queries('')) == ['']
    assert list(prepare_toloka_queries('запрос')) == ['запрос']
    assert list(prepare_toloka_queries('aaa bbb ccc')) == ['aaa bbb ccc']
    assert list(prepare_toloka_queries('алиса <EOS> алиса как дела <EOS> какие новости')) \
           == ['алиса алиса как дела какие новости']
    assert list(prepare_toloka_queries('алиса <EOSp> яндекс <EOSp> алиса как дела')) \
           == ['алиса', 'алиса яндекс алиса как дела']


def test_split_sessions_by_duplicates():
    assert list(split_sessions_by_duplicates([{'req_id': 1}, {'req_id': 2}])) == [[{'req_id': 1}, {'req_id': 2}]]
    assert list(split_sessions_by_duplicates([{'req_id': 2}, {'req_id': 1}])) == [[{'req_id': 2}, {'req_id': 1}]]

    assert list(split_sessions_by_duplicates([
        {'i': 1, 'req_id': 'dup'},
        {'i': 2, 'req_id': 'dup'}
    ])) == [
        [{'i': 1, 'req_id': 'dup'}],
        [{'i': 2, 'req_id': 'dup'}]
    ]

    assert list(split_sessions_by_duplicates([
        {'i': 1, 'req_id': '1'},
        {'i': 2, 'req_id': '2'},
        {'i': 3, 'req_id': '3'},
        {'i': 4, 'req_id': 'dup'},
        {'i': 5, 'req_id': 'dup'}
    ])) == [
        [{'i': 1, 'req_id': '1'}, {'i': 2, 'req_id': '2'}, {'i': 3, 'req_id': '3'}, {'i': 4, 'req_id': 'dup'}],
        [{'i': 1, 'req_id': '1'}, {'i': 2, 'req_id': '2'}, {'i': 3, 'req_id': '3'}, {'i': 5, 'req_id': 'dup'}],
    ]
