import alice.tests.library.mark as mark
import pytest


@pytest.fixture(scope='function')
def experiments(request):
    return mark.get_experiments(request)


class TestSingle(object):

    def test_none(self, experiments):
        assert experiments == {}

    @pytest.mark.experiments('foo_bar', 'scenario=FooBar')
    def test_args(self, experiments):
        assert experiments == {'foo_bar': '1', 'scenario=FooBar': '1'}

    @pytest.mark.experiments(foo_bar='DA', scenario='FooBar')
    def test_kwargs(self, experiments):
        assert experiments == {'foo_bar': 'DA', 'scenario': 'FooBar'}

    @pytest.mark.experiments('foo_bar', foo_bar='DA')
    def test_repeat_name(self, experiments):
        assert experiments == {'foo_bar': 'DA'}

    @pytest.mark.experiments('foo', 'scenario=FooBar', bar='DA')
    def test_args_and_kwargs(self, experiments):
        assert experiments == {'foo': '1', 'scenario=FooBar': '1', 'bar': 'DA'}


@pytest.mark.experiments('scenario=FooBar')
class TestNested(object):

    def test_none(self, experiments):
        assert experiments == {'scenario=FooBar': '1'}

    @pytest.mark.experiments('foo_bar')
    def test_args(self, experiments):
        assert experiments == {'foo_bar': '1', 'scenario=FooBar': '1'}

    @pytest.mark.experiments(foo_bar='DA', scenario='FooBar')
    def test_kwargs(self, experiments):
        assert experiments == {'foo_bar': 'DA', 'scenario': 'FooBar', 'scenario=FooBar': '1'}

    @pytest.mark.experiments('foo_bar', foo_bar='DA')
    def test_repeat_name(self, experiments):
        assert experiments == {'foo_bar': 'DA', 'scenario=FooBar': '1'}

    @pytest.mark.experiments('foo', 'scenario=FooBar', bar='DA')
    def test_args_and_kwargs(self, experiments):
        assert experiments == {'foo': '1', 'scenario=FooBar': '1', 'bar': 'DA'}


@pytest.mark.experiments(scenario='FooBar')
class _TestBase(object):
    pass


@pytest.mark.experiments('foo_bar')
class TestNestedFromBase(_TestBase):

    def test_none(self, experiments):
        assert experiments == {'foo_bar': '1', 'scenario': 'FooBar'}

    @pytest.mark.experiments('scenario=FooBar')
    def test_args(self, experiments):
        assert experiments == {'foo_bar': '1', 'scenario=FooBar': '1', 'scenario': 'FooBar'}

    @pytest.mark.experiments('scenario=FooBar', foo_bar=1, scenario='NONE')
    def test_overload(self, experiments):
        assert experiments == {'foo_bar': 1, 'scenario': 'NONE', 'scenario=FooBar': '1'}

    @pytest.mark.experiments('scenario=FooBar', foo_bar=None, scenario=None)
    def test_overload_none(self, experiments):
        assert experiments == {'foo_bar': None, 'scenario=FooBar': '1', 'scenario': None}
