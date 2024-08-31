import alice.tests.library.mark as mark
import pytest


@pytest.fixture(scope='function')
def device_state(request):
    return mark.get_device_state(request)


class TestSingle(object):

    def test_none(self, device_state):
        assert device_state == {}

    @pytest.mark.device_state({'is_tv_plugged_in': False})
    def test_args(self, device_state):
        assert device_state == {'is_tv_plugged_in': False}

    @pytest.mark.device_state(sound_level=2, sound_muted=False)
    def test_kwargs(self, device_state):
        assert device_state == {'sound_level': 2, 'sound_muted': False}

    @pytest.mark.device_state({'is_tv_plugged_in': False}, is_tv_plugged_in=True)
    def test_repeat_name(self, device_state):
        assert device_state == {'is_tv_plugged_in': True}

    @pytest.mark.device_state({'is_tv_plugged_in': False}, sound_level=2, sound_muted=False)
    def test_args_and_kwargs(self, device_state):
        assert device_state == {'is_tv_plugged_in': False, 'sound_level': 2, 'sound_muted': False}


@pytest.mark.device_state(sound_level=2, sound_muted=False)
class TestNested(object):

    def test_none(self, device_state):
        assert device_state == {'sound_level': 2, 'sound_muted': False}

    @pytest.mark.device_state({'is_tv_plugged_in': False})
    def test_args(self, device_state):
        assert device_state == {'is_tv_plugged_in': False, 'sound_level': 2, 'sound_muted': False}

    @pytest.mark.device_state(is_tv_plugged_in=True, sound_level=2, sound_muted=False)
    def test_kwargs(self, device_state):
        assert device_state == {'is_tv_plugged_in': True, 'sound_level': 2, 'sound_muted': False}

    @pytest.mark.device_state({'sound_level': 100})
    def test_overload_args(self, device_state):
        assert device_state == {'sound_level': 100, 'sound_muted': False}

    @pytest.mark.device_state(sound_muted=True)
    def test_overload_kwargs(self, device_state):
        assert device_state == {'sound_level': 2, 'sound_muted': True}


@pytest.mark.device_state({'is_tv_plugged_in': False})
class _TestBase(object):
    pass


@pytest.mark.device_state(sound_level=2, sound_muted=False)
class TestNestedFromBase(_TestBase):

    def test_none(self, device_state):
        assert device_state == {'is_tv_plugged_in': False, 'sound_level': 2, 'sound_muted': False}

    @pytest.mark.device_state({'device_config': {'content_settings': 'safe'}})
    def test_args(self, device_state):
        assert device_state == {
            'is_tv_plugged_in': False,
            'sound_level': 2,
            'sound_muted': False,
            'device_config': {'content_settings': 'safe'},
        }

    @pytest.mark.device_state(rcu={'is_rcu_connected': True})
    def test_kwargs(self, device_state):
        assert device_state == {
            'is_tv_plugged_in': False,
            'sound_level': 2,
            'sound_muted': False,
            'rcu': {'is_rcu_connected': True},
        }

    @pytest.mark.device_state({'is_tv_plugged_in': True})
    def test_overload_args(self, device_state):
        assert device_state == {'is_tv_plugged_in': True, 'sound_level': 2, 'sound_muted': False}

    @pytest.mark.device_state(is_tv_plugged_in=True)
    def test_overload_kwargs(self, device_state):
        assert device_state == {'is_tv_plugged_in': True, 'sound_level': 2, 'sound_muted': False}
