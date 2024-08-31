import alice.tests.library.directives as directives
import alice.tests.library.intent as intent
import alice.tests.library.scenario as scenario
import alice.tests.library.surface as surface
import pytest


@pytest.mark.parametrize('surface', [surface.navi_tr])
class TestFindPoiTr(object):
    """
    https://testpalm.yandex-team.ru/testcase/alice-2097
    https://testpalm.yandex-team.ru/testcase/alice-2100
    """

    owners = ('ardulat',)

    _responses = [
        'İşte burada.',
        'Şu anda gösteriyorum.',
        'Haritada gösteriyorum.',
        'Haritada senin için işaretledim, hemen görebilirsin.',
    ]

    _beg_your_pardon = [
        'Üzgünüm, daha iyi anlamam için başka türlü söyleyebilir misin?',
        'Tam olarak anlayamadım. Daha açık bir şekilde anlatabilir misin?',
        'Ne dediğinden emin olamadım. Biraz daha farklı bir şekilde söyleyebilir misin?',
    ]

    def _assert_scenario_response(self, response):
        assert response.scenario == scenario.FindPoiTr
        assert response.text in self._responses
        assert response.directive.name == directives.names.OpenUriDirective
        assert response.directive.payload.uri.startswith('yandexnavi://map_search')

    @pytest.mark.parametrize('command', [
        'tercüman sitesi zeytinburnu',
        'uhuvvet sokak no 10',
        '19 mayıs caddesini ara',
        'bir şey ara',
        'en yakın metro istasyonu bulabilir misin',
        'En yakın hastane',
        'Buraya yakın eczaneleri göster',
        'En yakın benzinlik nerede',
        'İstanbul Bağdat caddesi numara 12',
        'Tutkalcı Sokak No 26',
        'Kamer Hatun Mah., Beyoğlu',
        'Leman Kültür',
        'Cevahir AVM',
        'Mado',
        'Taksim meydanını bul',
        'İstiklal caddesi nerede',
        'Ankara',
        'Van gölü',
        'Atatürk barajı',
        'Geniş sokak, 125',
        'Nerede yemek yiyebilirim',
        'Benzin istasyonu bul',
        pytest.param(
            'Havuz burada nerede?',
            marks=pytest.mark.xfail(reason='https://st.yandex-team.ru/HOLLYWOOD-210')
        )
    ])
    def test_alice_2097_2100(self, alice, command):
        response = alice(command)
        self._assert_scenario_response(response)

    @pytest.mark.parametrize('command', [
        'Senianlamam köyü'
    ])
    def test_alice_2100_general_conversation(self, alice, command):
        # Non-existent object requests return GeneralConversationTr
        response = alice(command)
        assert response.scenario == scenario.GeneralConversationTr
        assert response.intent == intent.GeneralConversationVinslessPardon
        assert response.text in self._beg_your_pardon

    @pytest.mark.xfail(reason='https://st.yandex-team.ru/HOLLYWOOD-210')
    @pytest.mark.experiments(
        'bg_enable_find_poi_experimental',
        'bg_fresh_granet_experiment=bg_enable_find_poi_experimental',
    )
    @pytest.mark.parametrize('command', [
        'Havuz burada nerede',
        'silisen her alisa bilirsek',
        'gebze öylüyorsun yandex de cevap biliyor',
    ])
    def test_repeated_slot(self, alice, command):
        response = alice(command)
        self._assert_scenario_response(response)
