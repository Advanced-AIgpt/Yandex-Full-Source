import alice.tests.library.intent as intent
import alice.tests.library.scenario as scenario
import alice.tests.library.surface as surface
import pytest


@pytest.mark.experiments(
    'radionews_postroll_next_provider',
    'radionews_pp_enable',
    'radionews_navi_enable',
)
@pytest.mark.parametrize('surface', [
    surface.loudspeaker,
    surface.navi,
    surface.searchapp,
    surface.smart_tv,
    surface.station,
])
class TestRadioNews(object):

    owners = ('kuptservol', )

    @pytest.mark.parametrize('command', [
        'расскажи новости',
        'что нового на',
        'расскажи новое с',
        'новости',
    ])
    @pytest.mark.parametrize('source', [
        'Вести ФМ',
        'Радио России',
        'Коммерсант ФМ',
        'Радио Культура',
        'Серебряный дождь',
    ])
    def test_activate_radionews(self, alice, command, source):
        response = alice(f'{command} {source}')
        assert response.scenario == scenario.ExternalSkillFlashBriefing
        assert response.intent == intent.ActivateRadioNews

    @pytest.mark.parametrize('command', [
        'новости спорта',
        'новости политики',
        'новости в Москве',
    ])
    def test_activate_radionews_after_news(self, alice, command):
        response = alice(command)
        assert response.scenario != scenario.ExternalSkillFlashBriefing
        assert response.intent != intent.ActivateRadioNews

        response = alice('новости коммерсант фм')
        assert response.scenario == scenario.ExternalSkillFlashBriefing
        assert response.intent == intent.ActivateRadioNews
