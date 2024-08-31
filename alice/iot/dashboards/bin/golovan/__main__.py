# -*- coding: utf8 -*-
# flake8: noqa
"""
A alert / check generator for YASM + Juggler

Fairly simple.

Main points:
    - have everything under `quasar` prefix in yasm and namespace in juggler
    - have `quasar` juggler tag so our notification rule catches that up
"""
from __future__ import print_function

from collections import namedtuple
from copy import deepcopy
from enum import Enum

import requests

UI = 'https://yasm.yandex-team.ru/'
API = 'https://yasm.yandex-team.ru/srvambry/'
PANEL_OWNER_USER = 'robot-quasar'  # SOMEHOW each panel must have a user but it is not checked anywhere! Just use our robot here.

bulbasaur_common_prefix = 'unistat-quasar_bulbasaur_'
bulbasaur_signals_tag = "itype=bulbasaur;prj=bulbasaur;ctype=production"
bulbasaur_perf_prefix = bulbasaur_common_prefix + 'app_perf_'

tuya_common_prefix = 'unistat-quasar_bulbasaur_tuya_'
tuya_signals_tag = "itype=unknown;prj=tuya_adapter;ctype=production"

provider_handlers = [
    'action',
    'query',
    'discovery',
    'unlink',
    'delete',
]

providers_errors_list = [
    'device_unreachable',
    'device_busy',
    'device_not_found',
    'internal_error',
    'invalid_action',
    'invalid_value',
    'not_supported_in_current_mode',
    'unknown_error',
]

provider = namedtuple('Provider', 'name skill_id')


class Providers(Enum):
    TUYA = provider(name='Tuya', skill_id='T')
    XIAOMI = provider(name='Xiaomi', skill_id='ad26f8c2-fc31-4928-a653-d829fda7e6c2')
    PHILIPS = provider(name='Philips', skill_id='4a8cbce2-61d3-4e58-9f7b-6b30371d265c')
    REDMOND = provider(name='Redmond', skill_id='d1ba0dfe-e7b5-4a1c-af18-2e35ea0adb3c')
    MAJORDOMO = provider(name='Majordomo', skill_id='3d1b38d7-ab99-44fe-a799-0daa65202358')


class ProviderError(Enum):
    ERROR = bulbasaur_common_prefix + 'provider_error_{}_dmmm'
    ACTION = bulbasaur_common_prefix + 'provider_action_error_{}_dmmm'
    VALIDATION = bulbasaur_common_prefix + 'provider_validation_error_{}_discovery_skipped_devices_dmmm'


class ProvidersTotalActions(Enum):
    CALL = bulbasaur_common_prefix + "providers_total_calls_dmmm"
    FAILURE = bulbasaur_common_prefix + "providers_total_calls_failures_dmmm"
    FAILURE_4XX = bulbasaur_common_prefix + "providers_total_http_4xx_errors_dmmm"
    FAILURE_5XX = bulbasaur_common_prefix + "providers_total_http_5xx_errors_dmmm"


class ProviderAction(Enum):
    CALL = bulbasaur_common_prefix + "provider_call_{}_calls_dmmm"
    FAILURE = bulbasaur_common_prefix + "provider_call_{}_failures_dmmm"
    QUANT = "div(quant(" + bulbasaur_common_prefix + "provider_call_{}_duration_dhhh,999),1000)"


def create_panel(panel_key, panel_data):
    result = requests.post(
        API + 'upsert',
        json={
            'keys': {'key': panel_key, 'user': PANEL_OWNER_USER},
            'values': panel_data,
        },
    )

    result.raise_for_status()

    print('Upserted %spanel/%s' % (UI, panel_key))


def panel_chart(geometry, type='graphic', title=None, minValue=0, **data):
    """
    :param geometry: a string like 1_2_3_4
        1 -- row
        2 -- column
        3 -- width
        4 -- height

        i.e. 2_2_1_1 -- second row, second column, 1x1-panel_chart
    """

    row, column, width, height = map(int, geometry.split('_'))

    return dict(
        row=row,
        height=height,
        col=column,
        width=width,
        title=title,
        type=type,
        minValue=minValue,
        **data
    )


def unnormalized_simple_metric(title, name, tag):
    metric = {
        'title': title,
        'normalizable': False,
        'tag': tag,
        'host': "ASEARCH",
        'name': name,
    }

    return metric


def deploy_bulbasaur():
    def split_metric_by_geo(metric):
        metrics = []

        if metric.get('tag') is not None:
            tags = metric.get('tag').split(';')
            tags_map = {tag.split('=')[0]: tag.split('=')[1] for tag in tags if tag}

            for region in ('sas', 'vla', 'man'):
                geo_metric = deepcopy(metric)

                tags_map['geo'] = region
                geo_metric['title'] = metric['title'] + ' ' + region
                geo_metric['tag'] = ';'.join(['='.join([tag_name, tag_value]) for tag_name, tag_value in tags_map.items()])
                metrics.append(geo_metric)
        else:
            metrics.append(metric)

        return metrics

    class APIAction(Enum):
        CALL = bulbasaur_common_prefix + 'method_{}_calls_dmmm'
        FAILURE = bulbasaur_common_prefix + 'method_{}_failures_dmmm'
        QUANT = 'div(quant(' + bulbasaur_common_prefix + 'method_{}_duration_dhhh,999),1000)'

    def api_methods(action, methods):
        return [
            {"title": "/{} - {}".format(u, m),
             "active": False if (u == 'unistat' or u == 'ping') else True,
             "tag": bulbasaur_signals_tag,
             "host": "ASEARCH",
             "name": action.format(u.replace('/', '_').replace('.', '-') + '_' + m.lower()),
             } for u, m in methods]

    def ui_methods(action):
        return api_methods(action, [('m/user/devices', 'GET'),
                                    ('m/user/devices/@', 'PUT'),
                                    ('m/user/devices/@', 'DELETE'),
                                    ('m/user/devices/@', 'GET'),
                                    ('m/user/devices/@/actions', 'POST'),
                                    ('m/user/devices/@/configuration', 'GET'),
                                    ('m/user/devices/@/edit', 'GET'),
                                    ('m/user/devices/@/groups', 'GET'),
                                    ('m/user/devices/@/groups', 'PUT'),
                                    ('m/user/devices/@/room', 'PUT'),
                                    ('m/user/devices/@/rooms', 'GET'),
                                    ('m/user/devices/@/suggestions', 'GET'),
                                    ('m/user/groups', 'GET'),
                                    ('m/user/groups', 'POST'),
                                    ('m/user/groups/add', 'GET'),
                                    ('m/user/groups/@', 'GET'),
                                    ('m/user/groups/@', 'PUT'),
                                    ('m/user/groups/@', 'DELETE'),
                                    ('m/user/groups/@/actions', 'POST'),
                                    ('m/user/groups/@/edit', 'GET'),
                                    ('m/user/rooms', 'GET'),
                                    ('m/user/rooms', 'POST'),
                                    ('m/user/rooms/add', 'GET'),
                                    ('m/user/rooms/@', 'DELETE'),
                                    ('m/user/rooms/@', 'PUT'),
                                    ('m/user/rooms/@/edit', 'GET'),
                                    ('m/user/scenarios', 'POST'),
                                    ('m/user/scenarios', 'GET'),
                                    ('m/user/scenarios/add', 'GET'),
                                    ('m/user/scenarios/icons', 'GET'),
                                    ('m/user/scenarios/triggers', 'GET'),
                                    ('m/user/scenarios/@', 'PUT'),
                                    ('m/user/scenarios/@', 'DELETE'),
                                    ('m/user/scenarios/@/actions', 'POST'),
                                    ('m/user/scenarios/@/edit', 'GET'),
                                    ('m/user/skills', 'GET'),
                                    ('m/user/skills/@', 'GET'),
                                    ('m/user/skills/@/discovery', 'POST'),
                                    ('m/user/skills/@/unbind', 'POST')])

    def megamind_methods(action):
        return api_methods(action, [('v1.0/map/colors', 'GET'),
                                    ('v1.0/user/devices/actions', 'POST'),
                                    ('v1.0/user/hypotheses', 'POST'),
                                    ('v1.0/user/info', 'GET'),
                                    ('test/bb/user/ticket', 'GET'),
                                    ('test/bb/user/uid', 'GET'),
                                    ('test/devices/@/action', 'POST'),
                                    ('test/dialogs/skill/@', 'GET'),
                                    ('test/discover', 'POST'),
                                    ('test/palette', 'GET'),
                                    ('test/socialism/user', 'GET'),
                                    ('test/tvm/checksrv', 'GET'),
                                    ('test/tvm/service/ticket', 'GET'),
                                    ('ping', 'GET'),
                                    ('unistat', 'GET')])

    create_panel(
        'quasar_bulbasaur',
        {
            u'editors': [],
            u'title': u'Quasar Bulbasaur',
            u'type': u'panel',
            u'charts': [
                # Line 1
                panel_chart('1_1_2_1', title=u"Методы UI - RPS", stacked=True, normalize=True, signals=ui_methods(APIAction.CALL.value)),
                panel_chart('1_3_2_1', title=u"Методы UI - failures RPS", stacked=True, normalize=True, signals=ui_methods(APIAction.FAILURE.value)),
                panel_chart('1_5_2_1', title=u"Методы UI - время, мс (квантиль 99.9%)", normalize=True, signals=ui_methods(APIAction.QUANT.value)),

                # Line 2
                panel_chart('2_1_2_1', title=u"Методы MM+Other - RPS", stacked=True, normalize=True, signals=megamind_methods(APIAction.CALL.value)),
                panel_chart('2_3_2_1', title=u"Методы MM+Other - failures RPS", stacked=True, normalize=True, signals=megamind_methods(APIAction.FAILURE.value)),
                panel_chart('2_5_2_1', title=u"Методы MM+Other - время, мс (квантиль 99.9%)", normalize=True,signals=megamind_methods(APIAction.QUANT.value)),

                # Line 3
                panel_chart('3_1_2_1', title=u"GC starts num", normalize=False, signals=split_metric_by_geo(unnormalized_simple_metric(title='gc starts', tag=bulbasaur_signals_tag, name=bulbasaur_perf_prefix + 'gc_starts_num_dmmm'))),
                panel_chart('3_2_2_1', title=u"GC pause, ns", normalize=False, signals=split_metric_by_geo(unnormalized_simple_metric(title='gc last pause', tag=bulbasaur_signals_tag, name=bulbasaur_perf_prefix + 'gc_pause_total_ns_dmmm'))),
                panel_chart('3_3_1_1', title=u"GC forces starts num", normalize=False, signals=split_metric_by_geo(unnormalized_simple_metric(title='gc forced starts', tag=bulbasaur_signals_tag, name=bulbasaur_perf_prefix + 'gc_forced_starts_num_dmmm'))),
                panel_chart('3_4_1_1', title=u"Goroutine nums", normalize=False, signals=split_metric_by_geo(unnormalized_simple_metric(title='goroutines', tag=bulbasaur_signals_tag, name=bulbasaur_perf_prefix + 'goroutine_nums_dmmm'))),

                # Line 4
                panel_chart('4_1_1_1', title=u"Total alloc, Mb", normalize=False, signals=split_metric_by_geo(unnormalized_simple_metric(title='total alloc', tag=bulbasaur_signals_tag, name='conv({}, 1, Mi)'.format(bulbasaur_perf_prefix + 'mem_total_alloc_bytes_dmmm')))),
                panel_chart('4_2_2_1', title=u"Heap alloc, Mb", normalize=False, signals=split_metric_by_geo(unnormalized_simple_metric(title='heap alloc', tag=bulbasaur_signals_tag, name='conv({}, 1, Mi)'.format(bulbasaur_perf_prefix + 'mem_heap_alloc_bytes_dmmm')))),
                panel_chart('4_3_2_1', title=u"Heap in use, Mb", normalize=False, signals=split_metric_by_geo(unnormalized_simple_metric(title='heap in use', tag=bulbasaur_signals_tag, name='conv({}, 1, Mi)'.format(bulbasaur_perf_prefix + 'mem_heap_in_use_bytes_dmmm')))),
                panel_chart('4_4_1_1', title=u"Stack in use, Mb", normalize=False, signals=split_metric_by_geo(unnormalized_simple_metric(title='stack in use', tag=bulbasaur_signals_tag, name='conv({}, 1, Mi)'.format(bulbasaur_perf_prefix + 'mem_stack_in_use_bytes_dmmm')))),

                # Line 5
                panel_chart('5_1_2_1', title=u"Heap objects allocated", normalize=False, signals=split_metric_by_geo(unnormalized_simple_metric(title='heap objects allocated', tag=bulbasaur_signals_tag, name=bulbasaur_perf_prefix + 'mem_heap_objects_allocated_dmmm'))),
                panel_chart('5_2_2_1', title=u"Heap objects", normalize=False, signals=split_metric_by_geo(unnormalized_simple_metric(title='heap objects', tag=bulbasaur_signals_tag, name=bulbasaur_perf_prefix + 'mem_heap_objects_dmmm'))),
                panel_chart('5_3_2_1', title=u"Heap objects freed", normalize=False, signals=split_metric_by_geo(unnormalized_simple_metric(title='heap objects freed', tag=bulbasaur_signals_tag, name=bulbasaur_perf_prefix + 'mem_heap_objects_freed_dmmm'))),
            ],
        },
    )


def deploy_bulbasaur_providers():
    shown_providers = [
        Providers.TUYA.value,
        Providers.XIAOMI.value,
        Providers.PHILIPS.value,
        Providers.REDMOND.value,
        Providers.MAJORDOMO.value,
    ]

    def provider_calls(action, provider_id):
        return [
            {"title": "{}".format(handler),
             "tag": bulbasaur_signals_tag,
             "host": "ASEARCH",
             "name": action.format(provider_id + '_' + handler),
             } for handler in provider_handlers]

    def providers_total_calls(line_title, action):
        return [{
            "title": line_title,
            "tag": bulbasaur_signals_tag,
            "host": "ASEARCH",
            "name": action,
        }]

    def providers_total_calls_err_percent():
        return [
            unnormalized_simple_metric(title="Total http errors", tag=bulbasaur_signals_tag, name="perc({}, {})".format(ProvidersTotalActions.FAILURE.value, ProvidersTotalActions.CALL.value)),
        ]

    def provider_calls_err_percent(provider_id):
        name_template = "perc(" + bulbasaur_common_prefix + "provider_call_{}_<failures|calls>_dmmm)"
        return [
            unnormalized_simple_metric(title=handler, tag=bulbasaur_signals_tag, name=name_template.format(provider_id + '_' + handler))
            for handler in provider_handlers
        ]

    def provider_errors(action, provider_id):
        return [
            unnormalized_simple_metric(title=err, tag=bulbasaur_signals_tag, name=action.format(provider_id + '_' + err))
            for err in providers_errors_list
        ]

    def provider_action_errors_percent(provider_id):
        name_template = "perc({}," + bulbasaur_common_prefix + "provider_{}_actions_cnt_dmmm)"
        return [
            unnormalized_simple_metric(title=err, tag=bulbasaur_signals_tag, name=name_template.format(ProviderError.ACTION.value.format(provider_id + '_' + err), provider_id))
            for err in providers_errors_list
        ]

    def skipped_devices():
        name_template = ProviderError.VALIDATION.value
        return [
            unnormalized_simple_metric(title=shown_provider.name, tag=bulbasaur_signals_tag, name=name_template.format(shown_provider.skill_id))
            for shown_provider in shown_providers
        ]

    def provider_panel(provider_name, provider_id, offset_y, offset_x):
        return [panel_chart('{0}_{1}_1_1'.format(offset_y, offset_x), title=u"{}: Вызовы (RPS)".format(provider_name), stacked=True, normalize=True, signals=provider_calls(ProviderAction.CALL.value, provider_id)),
                panel_chart('{0}_{1}_1_1'.format(offset_y, offset_x + 1), title=u"{}: % HTTP Ошибок".format(provider_name), stacked=True, normalize=False, signals=provider_calls_err_percent(provider_id)),
                panel_chart('{0}_{1}_1_1'.format(offset_y, offset_x + 2), title=u"{}: Ошибки протокола УД".format(provider_name), stacked=True, normalize=False, signals=provider_errors(ProviderError.ERROR.value, provider_id)),
                panel_chart('{0}_{1}_1_1'.format(offset_y, offset_x + 3), title=u"{}: % ошибок исполнения команд".format(provider_name), stacked=True, normalize=False, signals=provider_action_errors_percent(provider_id)),
                panel_chart('{0}_{1}_1_1'.format(offset_y, offset_x + 4), title=u"{}: Тайминги, мс (квантиль 99.9%)".format(provider_name), normalize=True, signals=provider_calls(ProviderAction.QUANT.value, provider_id))]

    def providers_total_panel(offset_y, offset_x):
        return [
            panel_chart('{0}_{1}_2_1'.format(offset_y, offset_x), title=u"Все провайдеры: Вызовы (RPS)", stacked=True, normalize=True, signals=providers_total_calls("Total calls", ProvidersTotalActions.CALL.value)),
            panel_chart('{0}_{1}_2_1'.format(offset_y, offset_x + 1), title=u"Все провайдеры: % HTTP Ошибок", stacked=True, normalize=False, signals=providers_total_calls_err_percent()),
            panel_chart('{0}_{1}_1_1'.format(offset_y, offset_x + 2), title=u"Discovery skipped devices", stacked=False, normalize=False, signals=skipped_devices()),
        ]

    create_panel(
        'quasar_bulbasaur_providers',
        {
            u'editors': [],
            u'title': u'Quasar Bulbasaur Providers Stat',
            u'type': u'panel',
            u'charts':
                provider_panel(Providers.TUYA.value.name, Providers.TUYA.value.skill_id, 1, 1)
                + provider_panel(Providers.XIAOMI.value.name, Providers.XIAOMI.value.skill_id, 2, 1)
                + provider_panel(Providers.PHILIPS.value.name, Providers.PHILIPS.value.skill_id, 3, 1)
                + provider_panel(Providers.REDMOND.value.name, Providers.REDMOND.value.skill_id, 4, 1)
                + provider_panel(Providers.MAJORDOMO.value.name, Providers.MAJORDOMO.value.skill_id, 5, 1)
                + providers_total_panel(6, 1),
        },
    )


def deploy_bulbasaur_ydb():
    class APIAction(Enum):
        CALL = bulbasaur_common_prefix + 'ydb_{}_calls_dmmm'
        FAILURE = bulbasaur_common_prefix + 'ydb_{}_failures_dmmm'
        QUANT = 'div(max(' + bulbasaur_common_prefix + 'ydb_{}_durations_dhhh),1000)'

    def ydb_methods(action):
        return [
            {
                "title": m,
                "active": False if (m == 'selectUser') else True,
                "tag": bulbasaur_signals_tag,
                "host": "ASEARCH",
                "name": action.format(m.replace('/', '_').replace('.', '-').lower()),
            } for m in ['selectUserProviderDevicesSimple',
                        'selectUserGroupDevices',
                        'selectUserDevice',
                        'selectUserRoom',
                        'selectUserGroup',
                        'selectUserRooms',
                        'selectUserRoomsSimple',
                        'selectUserGroups',
                        'selectUserGroupsSimple',
                        'storeExternalUser',
                        'selectExternalUsers',
                        'deleteExternalUser',
                        'storeUserDevice',
                        'storeDeviceState',
                        'storeDevicesStates',
                        'updateUserRoomName',
                        'updateUserGroupName',
                        'deleteUserRoom',
                        'deleteUserGroup',
                        'deleteUserDevice',
                        'deleteUserDevices',
                        'updateUserDeviceName',
                        'updateUserDeviceRoom',
                        'updateUserDeviceGroups',
                        'createUserRoom',
                        'createUserGroup',
                        'selectUserScenario',
                        'createUserScenario',
                        'updateUserScenario',
                        'deleteUserScenario',
                        'selectStationOwners',
                        'selectUserSkills',
                        'storeUserSkill',
                        'deleteUserSkill']
        ]

    def ydb_mm_info_methods(action):
        return [
            {
                "title": m,
                "active": False if (m == 'selectUser' and action != APIAction.QUANT.value) else True,
                "tag": bulbasaur_signals_tag,
                "host": "ASEARCH",
                "name": action.format(m.replace('/', '_').replace('.', '-').lower()),
            } for m in ['selectUser',
                        'selectUserDevicesSimple',
                        'selectNonEmptyUserRooms',
                        'selectNonEmptyUserGroups',
                        'selectUserScenariosSimple']
        ]

    def ydb_mm_action_methods(action):
        return [
            {
                "title": m,
                "active": False if (m == 'selectUser' and action != APIAction.QUANT.value) else True,
                "tag": bulbasaur_signals_tag,
                "host": "ASEARCH",
                "name": action.format(m.replace('/', '_').replace('.', '-').lower()),
            } for m in ['selectUserDevices', 'selectUserScenarios']
        ]

    create_panel(
        'quasar_bulbasaur_ydb',
        {
            u'editors': [],
            u'title': u'Quasar Bulbasaur YDB',
            u'type': u'panel',
            u'charts': [
                panel_chart('1_1_2_2', title=u"Методы MM info YDB - RPS", normalize=True, signals=ydb_mm_info_methods(APIAction.CALL.value)),
                panel_chart('3_1_2_2', title=u"Методы MM info YDB - failures RPS", normalize=True, signals=ydb_mm_info_methods(APIAction.FAILURE.value)),
                panel_chart('5_1_2_2', title=u"Методы MM info YDB - время, мс (MAX)", normalize=True, signals=ydb_mm_info_methods(APIAction.QUANT.value)),

                panel_chart('1_3_2_2', title=u"Методы MM action YDB - RPS", normalize=True, signals=ydb_mm_action_methods(APIAction.CALL.value)),
                panel_chart('3_3_2_2', title=u"Методы MM action YDB - failures RPS", normalize=True, signals=ydb_mm_action_methods(APIAction.FAILURE.value)),
                panel_chart('5_3_2_2', title=u"Методы MM action YDB - время, мс (MAX)", normalize=True, signals=ydb_mm_action_methods(APIAction.QUANT.value)),

                panel_chart('1_5_2_2', title=u"Методы YDB (others) - RPS", normalize=True, signals=ydb_methods(APIAction.CALL.value)),
                panel_chart('3_5_2_2', title=u"Методы YDB (others) - failures RPS", normalize=True, signals=ydb_methods(APIAction.FAILURE.value)),
                panel_chart('5_5_2_2', title=u"Методы YDB (others) - время, мс (MAX)", normalize=True, signals=ydb_methods(APIAction.QUANT.value)),
            ],
        },
    )


if __name__ == '__main__':
    deploy_bulbasaur()
    deploy_bulbasaur_ydb()
    deploy_bulbasaur_providers()
