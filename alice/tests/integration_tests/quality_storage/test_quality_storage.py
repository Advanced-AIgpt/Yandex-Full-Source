import alice.tests.library.surface as surface
import alice.tests.library.scenario as scenario
import alice.tests.library.auth as auth
import pytest


@pytest.mark.oauth(auth.YandexPlus)
@pytest.mark.parametrize('surface', [
    surface.searchapp,
    surface.station,
])
class TestQualityStorage(object):
    owners = ('fkuyanov', 'tolyandex')

    @pytest.mark.parametrize('command', [
        'Как у тебя дела?', 'Программа телепередач', 'Включи последний альбом металлика'])
    def test_quality_storage(self, alice, command):
        response = alice(command)
        quality_storage = response.quality_storage

        assert quality_storage.post_win_reason, 'No post_win_reason'
        assert quality_storage.pre_predicts, 'No pre_predicts'
        assert quality_storage.post_predicts, 'No post_predicts'

        for loser_scenario in [scenario.Vins,
                               scenario.Video,
                               scenario.Search,
                               scenario.HollywoodMusic,
                               scenario.GeneralConversation]:
            if loser_scenario != response.scenario:
                assert quality_storage.scenarios_information[loser_scenario], 'Loser scenarios must have information'

        stages = []
        for _, information in quality_storage.scenarios_information:
            assert information.reason, 'Scenario information must contain reason'
            stages.append(information.get('classification_stage', 'ECS_PRE'))
        assert 'ECS_PRE' in stages, 'No preclassifier stage'
        assert 'ECS_POST' in stages, 'No postclassifier stage'
