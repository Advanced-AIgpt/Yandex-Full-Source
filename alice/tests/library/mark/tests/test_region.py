import alice.tests.library.mark as mark
import alice.tests.library.region as region
import pytest


@pytest.fixture(scope='function')
def region_fixture(request):
    return mark.get_region(request)


class TestSingle(object):

    def test_none(self, region_fixture):
        assert region_fixture == region.Moscow

    @pytest.mark.region(region.Minsk)
    def test_define(self, region_fixture):
        assert region_fixture == region.Minsk


@pytest.mark.region(region.Minsk)
class TestNested(object):

    def test_none(self, region_fixture):
        assert region_fixture == region.Minsk

    @pytest.mark.region(region.Baku)
    def test_define(self, region_fixture):
        assert region_fixture == region.Baku


class TestUserDefinedRegionId(object):

    def test_none(self, region_fixture):
        assert region_fixture == region.Moscow
        assert not region_fixture.region_id

    @pytest.mark.region(region.Minsk)
    def test_define(self, region_fixture):
        assert region_fixture == region.Minsk
        assert not region_fixture.region_id

    @pytest.mark.region(region.Minsk, user_defined_region_id=False)
    def test_define_without_region_id(self, region_fixture):
        assert region_fixture == region.Minsk
        assert not region_fixture.region_id

    @pytest.mark.region(region.Minsk, user_defined_region_id=True)
    def test_define_with_region_id(self, region_fixture):
        assert region_fixture == region.Minsk
        assert region_fixture.region_id


class TestKwargs(object):

    @pytest.mark.region(lat=None, lon=None)
    def test_none(self, region_fixture):
        assert not region_fixture.location
        assert region_fixture.timezone == 'Europe/Moscow'
        assert not region_fixture.region_id
        assert not region_fixture.client_ip

    @pytest.mark.region(lat=1, lon=2)
    def test_define(self, region_fixture):
        assert region_fixture.location.Lat == 1
        assert region_fixture.location.Lon == 2
        assert region_fixture.timezone == 'Europe/Moscow'
        assert not region_fixture.region_id
        assert not region_fixture.client_ip

    @pytest.mark.region(
        lat=40.3899498, lon=49.83001328, timezone='Asia/Baku', client_ip='5.191.19.110', region_id=10253,
    )
    def test_define_all(self, region_fixture):
        assert region_fixture.location.Lat == 40.3899498
        assert region_fixture.location.Lon == 49.83001328
        assert region_fixture.timezone == 'Asia/Baku'
        assert region_fixture.region_id == 10253
        assert region_fixture.client_ip == '5.191.19.110'

    @pytest.mark.region(
        lat=40.3899498, lon=49.83001328, accuracy=15000,
    )
    def test_define_location(self, region_fixture):
        assert region_fixture.location.Lat == 40.3899498
        assert region_fixture.location.Lon == 49.83001328
        assert region_fixture.location.Accuracy == 15000
