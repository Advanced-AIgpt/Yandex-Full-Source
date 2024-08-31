import alice.tests.library.directives as directives
import alice.tests.library.surface as surface
import pytest


@pytest.mark.voice
@pytest.mark.parametrize('surface', [surface.station(sound_level=2, sound_muted=False)])
class _TestVolume(object):
    owners = ('mihajlova', 'makatunkin', 'tolyandex', )


class TestUpDownVolume(_TestVolume):

    @pytest.mark.parametrize('command', ['громче', 'сделай громче', 'погромче', 'увеличить громкость'])
    def test_sound_louder(self, alice, command):
        start_sound_level = alice.device_state.SoundLevel
        response = alice(command)
        assert response.directive.name == directives.names.SoundLouderDirective
        assert alice.device_state.SoundLevel > start_sound_level

    @pytest.mark.parametrize('command', ['тише', 'сделай потише', 'потише', 'уменьшить громкость'])
    def test_sound_quiter(self, alice, command):
        start_sound_level = alice.device_state.SoundLevel
        response = alice(command)
        assert response.directive.name == directives.names.SoundQuiterDirective
        assert alice.device_state.SoundLevel < start_sound_level


class TestSoundMuted(_TestVolume):

    def test_sound_mute(self, alice):
        start_sound_level = alice.device_state.SoundLevel
        response = alice('выключи звук')
        assert response.directive.name == directives.names.SoundMuteDirective
        assert alice.device_state.SoundMuted
        assert alice.device_state.SoundLevel == start_sound_level

    def test_sound_unmute(self, alice):
        start_sound_level = alice.device_state.SoundLevel
        response = alice('включи звук')
        assert response.directive.name == directives.names.SoundUnmuteDirective
        assert not alice.device_state.SoundMuted
        assert alice.device_state.SoundLevel == start_sound_level


class TestSoundLevel(_TestVolume):

    @pytest.mark.parametrize('command', ['какая сейчас громкость', 'какой уровень громкости установлен'])
    def test_get_sound_level(self, alice, command):
        response = alice(command)
        assert response.intent.endswith('sound_get_level')
        assert response.text in [
            str(alice.device_state.SoundLevel),
            f'Сейчас {alice.device_state.SoundLevel}',
            f'Сейчас громкость {alice.device_state.SoundLevel}',
            f'Текущий уровень громкости {alice.device_state.SoundLevel}',
        ]

    @pytest.mark.parametrize('command, expected_sound_level', [
        ('уровень громкости', 9),
        ('поставь громкость', 4),
    ])
    def test_set_sound_level(self, alice, command, expected_sound_level):
        response = alice(f'{command} {expected_sound_level}')
        assert response.directive.name == directives.names.SoundSetLevelDirective
        assert alice.device_state.SoundLevel == expected_sound_level
