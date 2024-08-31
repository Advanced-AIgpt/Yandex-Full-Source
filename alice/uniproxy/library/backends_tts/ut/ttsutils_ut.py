# -*- coding: utf-8
from alice.uniproxy.library.backends_tts.ttsutils import split_text_by_speaker_tags as split


def test_plain_text():
    assert split(u"Привет!") == [{"text": u"Привет!"}]

    assert split(u"2 > 1") == [{"text": u"2 > 1"}]


def test_speak_tag():
    assert(
        split(u'<speaker voice="shitova">Яндекс.Новости: одна новость охренительнее другой!')
        ==
        [{"text": u"Яндекс.Новости: одна новость охренительнее другой!", "voice": "shitova"}]
    )

    assert(
        split(u"Привет! <speaker voice='ermil'>Меня зовут Ермил!<speaker voice='shitova'> И я говорю голосом Шитовой.<speaker>А это дефолт.")
        ==
        [{"text": u"Привет!"}, {"text": u"Меня зовут Ермил!", "voice": "ermil"}, {"text": u"И я говорю голосом Шитовой.", "voice": "shitova"}, {"text": u"А это дефолт."}]
    )


def test_speak_tag_whisper():
    assert(
        split(u'<speaker is_whisper="true"> Читала шутки в интернете, смеялась.', {'tts_allow_whisper': True})
        ==
        [{"text": u"Читала шутки в интернете, смеялась.", "is_whisper": True}]
    )

    assert(
        split(u'<speaker is_whisper="true"> Читала шутки в интернете, смеялась.', {'tts_allow_whisper': False})
        ==
        [{"text": u"Читала шутки в интернете, смеялась."}]
    )

    assert(
        split(u'<speaker is_whisper="true"> Читала шутки в интернете, смеялась.', {})
        ==
        [{"text": u"Читала шутки в интернете, смеялась."}]
    )

    assert(
        split(u'<speaker is_whisper="true"> Читала шутки в интернете, смеялась.')
        ==
        [{"text": u"Читала шутки в интернете, смеялась."}]
    )


def test_tts_tags():
    assert(
        split(u"рыба <[ mm aa s schwa | case=nom ]>")
        ==
        [{"text": u"рыба <[ mm aa s schwa | case=nom ]>"}]
    )

    assert(
        split(u"<speaker voice='oksana'>рыба <[ mm aa s schwa | case=nom ]>")
        ==
        [{"text": u"рыба <[ mm aa s schwa | case=nom ]>", "voice": "oksana"}]
    )


def test_audio_tag():
    assert(
        split(u'<speaker audio="http://yandex.ru/something.wav">Яндекс.Новости: одна новость охренительнее другой!')
        ==
        [{"text": u"Яндекс.Новости: одна новость охренительнее другой!", "audio": "http://yandex.ru/something.wav"}]
    )

    assert(
        split(u'<speaker audio="123">')
        ==
        [{"text": "", "audio": "123"}]
    )

    assert(
        split(u'<speaker audio="123">456')
        ==
        [{"text": "456", "audio": "123"}]
    )

    assert(
        split(u'456<speaker audio="123">')
        ==
        [{"text": u"456"}, {"text": "", "audio": "123"}]
    )

    assert(
        split(u'<speaker audio="123"><speaker voice="oksana">456')
        ==
        [{"text": "", "audio": "123"}, {"text": u"456", "voice": "oksana"}]
    )

    assert(
        split(u'<speaker audio="123"><speaker audio="456">22<speaker audio="789">')
        ==
        [{"text": "", "audio": "123"}, {"text": "22", "audio": "456"}, {"text": "", "audio": "789"}]
    )

    assert(
        split(u'<speaker lang="en">Who is mr<speaker voice="oksana">Путин?')
        ==
        [{"text": "Who is mr", "lang": "en"}, {"text": u"Путин?", "voice": "oksana"}]
    )

    assert(
        split(u'<speaker lang="en">Who is mr<speaker voice="oksana">Путин,<speaker voice="oksana" lang="en" volume="3.0">blyat?')
        ==
        [{"text": "Who is mr", "lang": "en"}, {"text": u"Путин,", "voice": "oksana"}, {"text": u"blyat?", "voice": "oksana", "lang": "en", "volume": 3.0}]
    )

    assert(
        split(u'<speaker lang="en">Who is mr<speaker voice="oksana">Путин,<speaker voice="oksana" lang="en" volume="high">blyat?')
        ==
        [{"text": "Who is mr", "lang": "en"}, {"text": u"Путин,", "voice": "oksana"}, {"text": u"blyat?", "voice": "oksana", "lang": "en"}]
    )

    assert(
        split(u'<speaker lang="en">Who is mr<speaker voice="oksana">Путин,<speaker voice="oksana" lang="en" volume="high" effect="translate_oksana_en">blyat?')
        ==
        [{"text": "Who is mr", "lang": "en"}, {"text": u"Путин,", "voice": "oksana"}, {"text": u"blyat?", "voice": "oksana", "lang": "en", "effect": "translate_oksana_en"}]
    )

    assert(
        split(u'<speaker lang="en" speed="azaza">Who is mr<speaker voice="oksana" speed="2.0">Путин?')
        ==
        [{"text": "Who is mr", "lang": "en"}, {"text": u"Путин?", "voice": "oksana", "speed": 2.0}]
    )

    # assert(
    #     split(u'<speaker lang="en" speed="azaza">Who is mr<speaker voice="oksana" speed="2.0" background="blessing">Путин?')
    #     ==
    #     [{"text": "Who is mr", "lang": "en"}, {"text": u"Путин?", "voice": "oksana", "speed": 2.0, "background": "blessing"}]
    # )

    assert(
        split(u'<speaker lang="en" speed="azaza">Who is mr<speaker voice="oksana" speed="2.0" emotion="super-evil">Путин?')
        ==
        [{"text": "Who is mr", "lang": "en"}, {"text": u"Путин?", "voice": "oksana", "speed": 2.0, "emotion": "super-evil"}]
    )
