import alice.tests.library.auth as auth
import alice.tests.library.directives as directives
import alice.tests.library.surface as surface
import pytest

from alice.protos.data.scenario.centaur.my_screen.widgets_pb2 import TCentaurWidgetConfigData, TWidgetPosition
from alice.tests.library.uniclient.proto_wrapper import ProtoWrapper


class WidgetConfigDataSlot(ProtoWrapper):
    proto_cls = TCentaurWidgetConfigData


class WidgetPositionSlot(ProtoWrapper):
    proto_cls = TWidgetPosition


def check_my_screen_tab(tab):
    assert tab.title == 'Мой экран'
    assert len(tab.blocks) == 1

    columns = tab.blocks[0].div_block.card.body.card.states[0].div.items
    assert len(columns) == 3

    assert len(columns[0].items) == 1
    assert len(columns[1].items) == 2
    assert len(columns[2].items) == 2


def check_music_tab(tab):
    assert tab.title == 'Музыка'
    assert len(tab.blocks) == 1
    assert tab.blocks[0].div_block.id == 'music.gallery.tab.block'


def check_smart_home_tab(tab):
    assert tab.title == 'Умный Дом'
    assert len(tab.blocks) == 1
    assert tab.blocks[0].div_block.id == 'smart.home.webview.tab'
    card = tab.blocks[0].div_block.card.body.card
    assert card.log_id == 'webview'
    assert card.states[0].div.items[0].custom_props.url == 'https://yandex.ru/iot'


def check_services_tab(tab):
    assert tab.title == 'Сервисы'
    assert len(tab.blocks) == 1
    assert tab.blocks[0].div_block.id == 'services.webview.tab'


def check_discovery_tab(tab):
    assert tab.title == 'Что я умею'
    assert len(tab.blocks) == 1
    assert tab.blocks[0].div_block.id == 'discovery.tab.block'


def check_tabs(tabs):
    check_my_screen_tab(tabs[0])
    check_music_tab(tabs[1])
    check_smart_home_tab(tabs[2])
    check_services_tab(tabs[3])
    check_discovery_tab(tabs[4])


def check_main_screen(response):
    assert response.combinator_product_name == 'CentaurMainScreen'
    assert len(response.directives) == 2

    set_main_screen = response.get_directive(directives.names.SetMainScreenDirective)
    assert set_main_screen.name == directives.names.SetMainScreenDirective
    set_upper_shutter = response.get_directive(directives.names.SetUpperShutterDirective)
    assert set_upper_shutter.name == directives.names.SetUpperShutterDirective

    check_tabs(set_main_screen.payload.tabs)


@pytest.mark.oauth(auth.YandexPlus)
@pytest.mark.experiments('mm_enable_combinators', 'mm_enable_combinator=CentaurMainScreen', 'main_screen_2.0',
                         'mm_enable_protocol_scenario=SmartDeviceExternalApp', 'scenario_widget_mechanics', 'main_screen_services_tab_enable')
@pytest.mark.parametrize('surface', [surface.smart_display])
class TestSmartDisplayMainScreen(object):
    owners = ('nkodosov', )

    def test_collect_main_screen(self, alice):
        response = alice.collect_main_screen()
        check_main_screen(response)

    def test_collect_widget_gallery(self, alice):
        widget_gallery_position = WidgetPositionSlot(Column=2, Row=1).dict()
        response = alice.collect_main_screen(widget_gallery_position)

        assert response.combinator_product_name == 'CentaurMainScreen'
        assert len(response.directives) == 1

        collect_widget_gallery = response.directive
        assert collect_widget_gallery.name == directives.names.ShowViewDirective
        collect_widget_gallery.payload.div2_card.body.card.states[0].div.items[0].items[0].text == 'Добавление виджета'

    def test_add_widget_from_gallery(self, alice):
        column = 2
        row = 1
        widget_data_slot = WidgetConfigDataSlot(WidgetType='weather').dict()
        response = alice.add_widget_from_gallery(column, row, widget_data_slot)
        check_main_screen(response)
