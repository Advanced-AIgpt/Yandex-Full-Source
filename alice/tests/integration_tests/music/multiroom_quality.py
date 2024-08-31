import alice.tests.library.auth as auth
import alice.tests.library.directives as directives
import alice.tests.library.scenario as scenario
import alice.tests.library.surface as surface
import pytest

import music.multiroom_iot_config as iot_config

from music.multiroom import ALL_ROOM_ID


@pytest.mark.oauth(auth.YandexPlus)
@pytest.mark.parametrize('surface', [surface.station])
@pytest.mark.iot(iot_config.config_for_quality_test)
class TestMusicPlayTaggerQualityForMultiroomCommands(object):

    owners = ('igor-darov',)

    commands_with_rooms = [
        ('включи музыку на кухне', 'kitchen'),
        ('в спальне вруби что-нибудь', 'bedroom'),
        ('хочу музыку в зале', 'hall'),
        ('поставь в гостиной музыку пожалуйста алиса', 'living-room'),
        ('алиса включи музыку в детской', 'kids-room'),

        ('включи morcheeba на кухне', 'kitchen'),
        ('запусти beatles в комнате спальня', 'bedroom'),
        ('алиса включи я в моменте и чтобы играла в зале только', 'hall'),
        ('поставь в гостиной группу би 2', 'living-room'),
        ('в детской включи трек пополам и погромче', 'kids-room'),

        ('включи нам музыку на миниках', 'minis-group'),
        ('поставь что-нибудь на станциях', 'stations-group'),
        ('включай музыку на окне давай', 'window-group'),
        ('включи музыку на группе шкаф', 'wardrobe-group'),
        ('вруби музыку на группе один', 'group-1'),

        ('pink floyd на миниках играй', 'minis-group'),
        ('поставь нам beatles на группе станции', 'stations-group'),
        ('алиса на окне проиграй киркорова', 'window-group'),
        ('на шкафе включай артик и асти', 'wardrobe-group'),
        ('включи niletto на группе группа 1', 'group-1'),

        ('включи музыку везде', ALL_ROOM_ID),
        ('включи музон на всех колонках', ALL_ROOM_ID),
        ('поставь музыку во всей квартире пожалуйста', ALL_ROOM_ID),
        ('на всех устройствах алиса вруби музыку', ALL_ROOM_ID),
        ('музыку на везде включи', ALL_ROOM_ID),

        ('включи моргенштерна везде', ALL_ROOM_ID),
        ('алиса а включи музыку про медведей везде', ALL_ROOM_ID),
        ('включи новую песню лайзера везде', ALL_ROOM_ID),
        ('алиса включи алоэвера лето везде', ALL_ROOM_ID),
        ('включи сектор газа чебурашка везде', ALL_ROOM_ID),
    ]

    @staticmethod
    def _small_check_multiroom_response(response, room_id):
        return (
            response.scenario == scenario.HollywoodMusic and
            len(response.directives) == 2 and
            hasattr(response.directives[0].payload, 'room_id') and
            response.directives[0].payload.room_id == room_id and
            response.directives[1].name == directives.names.MusicPlayDirective
        )

    @pytest.mark.oauth(auth.RobotMultiroom)
    def test_quality(self, alice):
        success_count = sum(
            self._small_check_multiroom_response(response=alice(command), room_id=room_id)
            for command, room_id in self.commands_with_rooms
        )
        success_rate = success_count / len(self.commands_with_rooms)

        assert_message = ('Multiroom quality fell bellow 0.8, verify music_play tagger. If the decrease is '
                          'not dramatic, may be lower the threshold as the test might be noisy (only 30 samples).')
        assert success_rate >= 0.8, assert_message
