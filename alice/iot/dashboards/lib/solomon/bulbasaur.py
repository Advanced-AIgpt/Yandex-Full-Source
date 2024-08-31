# -*- coding: utf8 -*-
# flake8: noqa
"""
Bulbasaur graphics
"""

import inspect

from alice.iot.dashboards.lib.solomon.alert_builder import Alert, AlertBuilder, AlertBuilderV2
from alice.iot.dashboards.lib.solomon.aql import AQL
from alice.iot.dashboards.lib.solomon.dashboard_builder import DashboardBuilder, Panel
from alice.iot.dashboards.lib.solomon.graphic_builder import RGBA
from alice.iot.dashboards.lib.solomon.provider import NamedMetric
from alice.iot.dashboards.lib.solomon.service_graphics import ServiceGraphics, HttpHandler, NamedProcessor


class BulbasaurGraphics(ServiceGraphics):
    PROJECT = "alice-iot"
    CLUSTER = "bulbasaur_production|bulbasaur_beta|bulbasaur_dev"
    SERVICE = "bulbasaur"

    PARAMETERS = [
        {
            "name": "project",
            "value": PROJECT,
        },
        {
            "name": "cluster",
            "value": CLUSTER,
        },
        {
            "name": "service",
            "value": SERVICE,
        },
        {
            "name": "host",
            "value": "*",
        }
    ]
    CACHE_PARAMETERS = [
        {
            "name": "project",
            "value": PROJECT,
        },
        {
            "name": "cluster",
            "value": CLUSTER,
        },
        {
            "name": "service",
            "value": SERVICE,
        },
        {
            "name": "host",
            "value": "*",
        },
        {
            "name": "cache",
            "value": "pumpkin"
        }
    ]
    SOCIALISM_PARAMETERS = [
        {
            "name": "project",
            "value": PROJECT,
        },
        {
            "name": "cluster",
            "value": CLUSTER,
        },
        {
            "name": "service",
            "value": SERVICE,
        },
        {
            "name": "host",
            "value": "*",
        },
        {
            "name": "skill_id",
            "value": "*",
        }
    ]
    BY_DC_PARAMETERS = [
        {
            "name": "project",
            "value": PROJECT,
        },
        {
            "name": "cluster",
            "value": "bulbasaur_production",
        },
        {
            "name": "service",
            "value": SERVICE,
        }
    ]
    PERF_PARAMETERS = [
        {
            "name": "project",
            "value": PROJECT,
        },
        {
            "name": "cluster",
            "value": CLUSTER,
        },
        {
            "name": "service",
            "value": SERVICE,
        }
    ]
    PROCESSOR_PARAMETERS = [
        {
            "name": "project",
            "value": PROJECT,
        },
        {
            "name": "cluster",
            "value": CLUSTER,
        },
        {
            "name": "service",
            "value": SERVICE,
        },
        {
            "name": "host",
            "value": "*",
        },
        {
            "name": "step",
            "value": "*",
        }
    ]

    DC = [
        NamedMetric("Sas", RGBA(r=155, g=100, b=0), False),
        NamedMetric("Man", RGBA(r=0, g=100, b=155), False),
        NamedMetric("Vla", RGBA(r=100, g=155, b=0), False),
    ]

    PROCESSORS = [
        NamedProcessor("begemot", "Разбор гипотез Бегемота"),
        NamedProcessor("cancel_all_scenarios", "Отмена всех команд"),
        NamedProcessor("cancel_scenario", "Отмена последней команды"),
        NamedProcessor("time_specified_begemot", "Разбор гипотез Бегемота с дозапросом времени"),
    ]
    USER_HANDLERS = [
        HttpHandler(path="/v1.0/user/", method="GET"),
        HttpHandler(path="/v1.0/user/info", method="GET"),
    ]
    MM_HANDLERS = [
        HttpHandler(path="/megamind/run", method="POST"),
        HttpHandler(path="/megamind/apply", method="POST"),
    ]
    API_HANDLERS = [
        HttpHandler(path="/api/v1.0/devices/{deviceId}", method="GET"),
        HttpHandler(path="/api/v1.0/groups/{groupId}", method="GET"),
        HttpHandler(path="/api/v1.0/user/info", method="GET"),
        HttpHandler(path="/api/v1.0/devices/actions", method="POST"),
        HttpHandler(path="/api/v1.0/devices/{deviceId}/actions", method="POST"),
        HttpHandler(path="/api/v1.0/groups/{groupId}/actions", method="POST"),
        HttpHandler(path="/api/v1.0/scenarios/{scenarioId}/actions", method="POST"),
    ]
    UI_HANDLERS = [
        HttpHandler(path="/m/user/devices/", method="GET"),
        HttpHandler(path="/m/user/devices/prefetch", method="GET"),
        HttpHandler(path="/m/user/devices/{deviceId}", method="PUT"),
        HttpHandler(path="/m/user/devices/{deviceId}", method="DELETE"),
        HttpHandler(path="/m/user/devices/{deviceId}", method="GET"),
        HttpHandler(path="/m/user/devices/{deviceId}/actions", method="POST"),
        HttpHandler(path="/m/user/devices/{deviceId}/capabilities", method="GET"),
        HttpHandler(path="/m/user/devices/{deviceId}/configuration", method="GET"),
        HttpHandler(path="/m/user/devices/{deviceId}/controls", method="GET"),
        HttpHandler(path="/m/user/devices/{deviceId}/edit", method="GET"),
        HttpHandler(path="/m/user/devices/{deviceId}/groups", method="GET"),
        HttpHandler(path="/m/user/devices/{deviceId}/groups", method="PUT"),
        HttpHandler(path="/m/user/devices/{deviceId}/history", method="GET"),
        HttpHandler(path="/m/user/devices/{deviceId}/households", method="GET"),
        HttpHandler(path="/m/user/devices/{deviceId}/name", method="POST"),
        HttpHandler(path="/m/user/devices/{deviceId}/name", method="PUT"),
        HttpHandler(path="/m/user/devices/{deviceId}/name", method="DELETE"),
        HttpHandler(path="/m/user/devices/{deviceId}/room", method="PUT"),
        HttpHandler(path="/m/user/devices/{deviceId}/rooms", method="GET"),
        HttpHandler(path="/m/user/devices/{deviceId}/suggestions", method="GET"),
        HttpHandler(path="/m/user/devices/{deviceId}/type", method="PUT"),
        HttpHandler(path="/m/user/devices/{deviceId}/types", method="GET"),
        HttpHandler(path="/m/user/events/", method="POST"),
        HttpHandler(path="/m/user/favorites/devices/", method="POST"),
        HttpHandler(path="/m/user/favorites/devices/", method="GET"),
        HttpHandler(path="/m/user/favorites/devices/properties/", method="POST"),
        HttpHandler(path="/m/user/favorites/devices/properties/", method="GET"),
        HttpHandler(path="/m/user/favorites/devices/{deviceId}", method="POST"),
        HttpHandler(path="/m/user/favorites/groups/", method="GET"),
        HttpHandler(path="/m/user/favorites/groups/", method="POST"),
        HttpHandler(path="/m/user/favorites/groups/{groupId}", method="POST"),
        HttpHandler(path="/m/user/favorites/scenarios/", method="POST"),
        HttpHandler(path="/m/user/favorites/scenarios/", method="GET"),
        HttpHandler(path="/m/user/favorites/scenarios/{scenarioId}", method="POST"),
        HttpHandler(path="/m/user/groups/", method="GET"),
        HttpHandler(path="/m/user/groups/", method="POST"),
        HttpHandler(path="/m/user/groups/add", method="GET"),
        HttpHandler(path="/m/user/groups/add/devices/available", method="GET"),
        HttpHandler(path="/m/user/groups/{groupId}", method="DELETE"),
        HttpHandler(path="/m/user/groups/{groupId}", method="GET"),
        HttpHandler(path="/m/user/groups/{groupId}", method="PUT"),
        HttpHandler(path="/m/user/groups/{groupId}/actions", method="POST"),
        HttpHandler(path="/m/user/groups/{groupId}/devices/available", method="GET"),
        HttpHandler(path="/m/user/groups/{groupId}/edit", method="GET"),
        HttpHandler(path="/m/user/groups/{groupId}/suggestions", method="GET"),
        HttpHandler(path="/m/user/households/", method="GET"),
        HttpHandler(path="/m/user/households/", method="POST"),
        HttpHandler(path="/m/user/households/add", method="GET"),
        HttpHandler(path="/m/user/households/current", method="POST"),
        HttpHandler(path="/m/user/households/devices-move", method="POST"),
        HttpHandler(path="/m/user/households/geosuggests", method="POST"),
        HttpHandler(path="/m/user/households/validate/name", method="POST"),
        HttpHandler(path="/m/user/households/{householdId}", method="DELETE"),
        HttpHandler(path="/m/user/households/{householdId}", method="GET"),
        HttpHandler(path="/m/user/households/{householdId}", method="PUT"),
        HttpHandler(path="/m/user/households/{householdId}/edit/name", method="GET"),
        HttpHandler(path="/m/user/households/{householdId}/groups", method="GET"),
        HttpHandler(path="/m/user/households/{householdId}/rooms", method="GET"),
        HttpHandler(path="/m/user/households/{householdId}/validate/name", method="POST"),
        HttpHandler(path="/m/user/launches/{launchId}", method="DELETE"),
        HttpHandler(path="/m/user/launches/{launchId}/edit", method="GET"),
        HttpHandler(path="/m/user/networks/", method="DELETE"),
        HttpHandler(path="/m/user/networks/", method="POST"),
        HttpHandler(path="/m/user/networks/get-info", method="POST"),
        HttpHandler(path="/m/user/networks/use", method="PUT"),
        HttpHandler(path="/m/user/rooms/", method="GET"),
        HttpHandler(path="/m/user/rooms/", method="POST"),
        HttpHandler(path="/m/user/rooms/add", method="GET"),
        HttpHandler(path="/m/user/rooms/add/devices/available", method="GET"),
        HttpHandler(path="/m/user/rooms/{roomId}", method="PUT"),
        HttpHandler(path="/m/user/rooms/{roomId}", method="DELETE"),
        HttpHandler(path="/m/user/rooms/{roomId}/devices/available", method="GET"),
        HttpHandler(path="/m/user/rooms/{roomId}/edit", method="GET"),
        HttpHandler(path="/m/user/scenarios/", method="GET"),
        HttpHandler(path="/m/user/scenarios/", method="POST"),
        HttpHandler(path="/m/user/scenarios/add", method="GET"),
        HttpHandler(path="/m/user/scenarios/device-triggers", method="GET"),
        HttpHandler(path="/m/user/scenarios/devices", method="GET"),
        HttpHandler(path="/m/user/scenarios/devices/{deviceId}/suggestions", method="GET"),
        HttpHandler(path="/m/user/scenarios/history", method="GET"),
        HttpHandler(path="/m/user/scenarios/icons", method="GET"),
        HttpHandler(path="/m/user/scenarios/triggers", method="GET"),
        HttpHandler(path="/m/user/scenarios/validate/capability", method="POST"),
        HttpHandler(path="/m/user/scenarios/validate/name", method="POST"),
        HttpHandler(path="/m/user/scenarios/validate/trigger", method="POST"),
        HttpHandler(path="/m/user/scenarios/{scenarioId}", method="PUT"),
        HttpHandler(path="/m/user/scenarios/{scenarioId}", method="DELETE"),
        HttpHandler(path="/m/user/scenarios/{scenarioId}/actions", method="POST"),
        HttpHandler(path="/m/user/scenarios/{scenarioId}/activation", method="POST"),
        HttpHandler(path="/m/user/scenarios/{scenarioId}/edit", method="GET"),
        HttpHandler(path="/m/user/settings/", method="GET"),
        HttpHandler(path="/m/user/settings/", method="POST"),
        HttpHandler(path="/m/user/skills/", method="GET"),
        HttpHandler(path="/m/user/skills/{skillId}", method="DELETE"),
        HttpHandler(path="/m/user/skills/{skillId}", method="GET"),
        HttpHandler(path="/m/user/skills/{skillId}/discovery", method="POST"),
        HttpHandler(path="/m/user/skills/{skillId}/unbind", method="POST"),
        HttpHandler(path="/m/user/speakers/capabilities/news/topics", method="GET"),
        HttpHandler(path="/m/user/storage/", method="POST"),
        HttpHandler(path="/m/user/storage/", method="DELETE"),
        HttpHandler(path="/m/user/storage/", method="GET"),
        HttpHandler(path="/m/v2/user/devices", method="GET"),
        HttpHandler(path="/m/v2/user/scenarios/", method="POST"),
        HttpHandler(path="/m/v2/user/scenarios/device-triggers", method="GET"),
        HttpHandler(path="/m/v2/user/scenarios/devices", method="GET"),
        HttpHandler(path="/m/v2/user/scenarios/validate/capability", method="POST"),
        HttpHandler(path="/m/v2/user/scenarios/validate/trigger", method="POST"),
        HttpHandler(path="/m/v2/user/scenarios/{scenarioId}", method="PUT"),
        HttpHandler(path="/m/v2/user/scenarios/{scenarioId}/edit", method="GET"),
        HttpHandler(path="/m/v3/user/devices/", method="GET"),
        HttpHandler(path="/m/v3/user/devices/stereopair/", method="POST"),
        HttpHandler(path="/m/v3/user/devices/stereopair/list-possible", method="GET"),
        HttpHandler(path="/m/v3/user/devices/stereopair/{deviceId}", method="DELETE"),
        HttpHandler(path="/m/v3/user/devices/{deviceId}/configuration/quasar", method="POST"),
        HttpHandler(path="/m/v3/user/devices/{deviceId}/ding", method="POST"),
        HttpHandler(path="/m/v3/user/launches/{launchId}/edit", method="GET"),
        HttpHandler(path="/m/v3/user/scenarios/", method="POST"),
        HttpHandler(path="/m/v3/user/scenarios/validate/capability", method="POST"),
        HttpHandler(path="/m/v3/user/scenarios/validate/trigger", method="POST"),
        HttpHandler(path="/m/v3/user/scenarios/{scenarioId}", method="PUT"),
        HttpHandler(path="/m/v3/user/scenarios/{scenarioId}/edit", method="GET"),
    ]
    CALLBACK_HANDLERS = [
        HttpHandler(path="/v1.0/callback/skills/{skillId}/discovery", method="POST"),
        HttpHandler(path="/v1.0/push/skills/{skillId}/discovery", method="POST"),
        HttpHandler(path="/v1.0/callback/skills/{skillId}/state", method="POST"),
    ]
    DIALOG_API_CALLS = [
        NamedMetric(name="getSmartHomeSkills"),
        NamedMetric(name="getSkill"),
        NamedMetric(name="getSkillCertifiedDevices"),
    ]
    BLACKBOX_API_CALLS = [
        NamedMetric(name="oauth"),
        NamedMetric(name="sessionID"),
    ]
    XIVA_API_CALLS = [
        NamedMetric(name="sendPush"),
        NamedMetric(name="getSubscriptionSign"),
        NamedMetric(name="getWebsocketURL"),
        NamedMetric(name="listSubscriptions"),
    ]
    SUP_API_CALLS = [
        NamedMetric(name="sendPush"),
    ]
    STEELIX_API_CALLS = [
        NamedMetric(name="callbackDiscovery"),
        NamedMetric(name="callbackState"),
        NamedMetric(name="pushDiscovery"),
    ]
    SOCIALISM_API_CALLS = [
        NamedMetric(name="deleteUserToken"),
        NamedMetric(name="checkUserToken"),
        NamedMetric(name="getUserToken"),
    ]
    TVM_API_CALLS = [
        NamedMetric(name="getServiceTicket"),
        NamedMetric(name="checkServiceTicket"),
        NamedMetric(name="checkUserTicket"),
    ]
    BASS_API_CALLS = [
        NamedMetric(name="sendPush"),
    ]
    BEGEMOT_API_CALLS = [
        NamedMetric(name="wizard"),
    ]
    DATASYNC_API_CALLS = [
        NamedMetric(name="getAddressesForUser"),
    ]
    GEOSUGGEST_API_CALLS = [
        NamedMetric(name="getGeosuggestFromAddress"),
    ]
    TIME_MACHINE_API_CALLS = [
        NamedMetric(name="submitTask"),
    ]
    MEMENTO_API_CALLS = [
        NamedMetric(name="getUserObjects"),
        NamedMetric(name="updateUserObjects"),
    ]
    TUYA_API_CALLS = [
        NamedMetric(name="getDevicesDiscoveryInfo"),
        NamedMetric(name="getDevicesUnderPairingToken"),
    ]
    SOLOMON_API_CALLS = [
        NamedMetric(name="fetchData"),
        NamedMetric(name="sendData"),
    ]
    NOTIFICATOR_API_CALLS = [
        NamedMetric(name="sendTypedSemanticFramePush"),
        NamedMetric(name="getDevices"),
    ]
    OAUTH_API_CALLS = [
        NamedMetric(name="issueAuthorizationCode"),
    ]
    UNIFIED_AGENT_API_CALLS = [
        NamedMetric(name="sendData"),
    ]
    NEIGHBOURS = [
        # "bass",
        # "begemot",
        # "blackbox",
        # "datasync",
        # "dialogs",
        # "geosuggest",
        # "memento",
        "notificator",
        "oauth",
        # "socialism",
        # "solomon_api",
        # "steelix",
        # "sup",
        # "time_machine",
        # "tuya",
        # "tvm",
        "unified_agent",
        "xiva",
    ]

    CODES = {
        "4xx": NamedMetric(name="4xx", color=RGBA(r=255, g=153, b=0, a=0.5)),
        "5xx": NamedMetric(name="5xx", color=RGBA(r=255, g=0, b=0)),
        "fails": NamedMetric(name="unavailable", color=RGBA(r=100, g=50, b=0)),
        "2xx": NamedMetric(name="2xx", color=RGBA(r=64, g=255, b=64, a=0.3)),
    }
    YDB_MM_INFO_METHODS = [
        "selectUser",
        "selectUserInfo"
    ]
    YDB_MM_ACTION_METHODS = [
        "selectUserDevices",
        "selectUserScenarios"
    ]
    YDB_OTHER_METHODS = [
        "selectUserDevices",
        "selectUserDevicesSimple",
        "selectDevicesSimpleByExternalIDs",
        "selectUserProviderDevicesSimple",
        "selectUserProviderArchivedDevicesSimple",
        "selectUserGroupDevices",
        "selectUserDevice",
        "selectUserDeviceSimple",
        "selectUserRoom",
        "selectUserGroup",
        "selectUserRooms",
        "selectUserGroups",
        "storeUserDevice",
        "storeDeviceState",
        "storeDevicesStates",
        "updateUserRoomName",
        "updateUserGroupName",
        "updateUserGroupNameAndAliases",
        "deleteUserRoom",
        "deleteUserGroup",
        "deleteUserDevice",
        "deleteUserDevices",
        "updateUserDeviceName",
        "updateUserDeviceNameAndAliases",
        "updateUserDeviceRoom",
        "updateUserDeviceType",
        "updateUserDeviceGroups",
        "createUserRoom",
        "createUserGroup",
        "storeUser",
        "selectUser",
        "storeExternalUser",
        "selectExternalUsers",
        "deleteExternalUser",
        "selectUserScenarios",
        "selectOnetimeScenarios",
        "selectScenario",
        "createScenario",
        "updateScenario",
        "deleteScenario",
        "deleteOnetimeScenariosScheduled",
        "selectUserInfo",
        "selectStationOwners",
        "selectUserSkills",
        "checkUserSkillExist",
        "storeUserSkill",
        "deleteUserSkill",
        "selectUserNetworks",
        "selectUserNetwork",
        "storeUserNetwork",
        "deleteUserNetwork",
        "selectAllExperiments",
    ]

    def __init__(self, oauth_token):
        ServiceGraphics.__init__(self, oauth_token)

    def update_graphics(self, project_id):
        # self.update_ui_graphics(project_id)
        # self.update_ydb_cache_graphics(project_id)
        # self.update_callback_graphics(project_id)
        # self.update_user_graphics(project_id)
        # self.update_api_graphics(project_id)
        # self.update_megamind_graphics(project_id)
        # self.update_by_dc_graphics(project_id)
        # self.update_processor_graphics(project_id)
        # self.update_ydb_graphics(project_id)
        # self.update_perf_graphics(project_id)
        self.update_neighbour_graphics(project_id)
        pass

    def update_processor_graphics(self, project_id):
        self.update(
            project_id=project_id,
            graphic_id="bulbasaur_processor_request_errors",
            name="Bulbasaur: Processor RPS ошибок",
            parameters=self.PROCESSOR_PARAMETERS,
            expressions=list(map(ServiceGraphics.ProcessorGraphics.errors_rps, self.PROCESSORS)),
            extra_data={
                "min": 0,
                "max": 5,
            }
        )
        self.update(
            project_id=project_id,
            graphic_id="bulbasaur_processor_request_total",
            name="Bulbasaur: Processor Total RPS",
            parameters=self.PROCESSOR_PARAMETERS,
            expressions=list(map(ServiceGraphics.ProcessorGraphics.processor_rps, self.PROCESSORS))
        )
        self.update(
            project_id=project_id,
            graphic_id="bulbasaur_processor_percentiles",
            name="Bulbasaur: Processor Timings, с (квантиль 99.9%)",
            parameters=self.PROCESSOR_PARAMETERS,
            expressions=list(map(ServiceGraphics.ProcessorGraphics.processor_percentile, self.PROCESSORS))
        )

    def update_user_graphics(self, project_id):
        self.update(
            project_id=project_id,
            graphic_id="bulbasaur_uniproxy_handlers_rps",
            name="Bulbasaur: Получение данных УД - RPS",
            parameters=self.PARAMETERS,
            expressions=list(map(ServiceGraphics.HttpGraphics.handler_rps, self.USER_HANDLERS))
        )
        self.update(
            project_id=project_id,
            graphic_id="bulbasaur_uniproxy_handlers_errors_rps",
            name="Bulbasaur: Получение данных УД - RPS HTTP ошибок",
            parameters=self.PARAMETERS,
            expressions=list(map(ServiceGraphics.HttpGraphics.handler_errors_rps, self.USER_HANDLERS)),
            extra_data={
                "min": 0,
                "max": 1,
            }
        )
        self.update(
            project_id=project_id,
            graphic_id="bulbasaur_uniproxy_handlers_errors_percent",
            name="Bulbasaur: Получение данных УД - % HTTP ошибок",
            parameters=self.PARAMETERS,
            expressions=list(map(ServiceGraphics.HttpGraphics.handler_errors_percent, self.USER_HANDLERS)),
            extra_data={
                "min": 0,
                "max": 110,
            }
        )
        self.update(
            project_id=project_id,
            graphic_id="bulbasaur_uniproxy_handlers_percentiles",
            name="Bulbasaur: Получение данных УД - время, с (квантиль 99.9%)",
            parameters=self.PARAMETERS,
            expressions=list(map(ServiceGraphics.HttpGraphics.handler_percentile, self.USER_HANDLERS)),
            extra_data={
                "min": 0,
                "max": 1,
            }
        )

    def update_api_graphics(self, project_id):
        self.update(
            project_id=project_id,
            graphic_id="bulbasaur_api_handlers_rps",
            name="Bulbasaur: API УД - RPS",
            parameters=self.PARAMETERS,
            expressions=list(map(ServiceGraphics.HttpGraphics.handler_rps, self.API_HANDLERS))
        )
        self.update(
            project_id=project_id,
            graphic_id="bulbasaur_api_handlers_errors_rps",
            name="Bulbasaur: API УД - RPS HTTP ошибок",
            parameters=self.PARAMETERS,
            expressions=list(map(ServiceGraphics.HttpGraphics.handler_errors_rps, self.API_HANDLERS)),
            extra_data={
                "min": 0,
                "max": 1,
            }
        )
        self.update(
            project_id=project_id,
            graphic_id="bulbasaur_api_handlers_errors_percent",
            name="Bulbasaur: API УД - % HTTP ошибок",
            parameters=self.PARAMETERS,
            expressions=list(map(ServiceGraphics.HttpGraphics.handler_errors_percent, self.API_HANDLERS)),
            extra_data={
                "min": 0,
                "max": 110,
            }
        )
        self.update(
            project_id=project_id,
            graphic_id="bulbasaur_api_handlers_percentiles",
            name="Bulbasaur: API УД - время, с (квантиль 99.9%)",
            parameters=self.PARAMETERS,
            expressions=list(map(ServiceGraphics.HttpGraphics.handler_percentile, self.API_HANDLERS)),
            extra_data={
                "min": 0,
                "max": 1,
            }
        )

    def update_megamind_graphics(self, project_id):
        self.update(
            project_id=project_id,
            graphic_id="bulbasaur_megamind_handlers_rps",
            name="Bulbasaur: Методы ММ - RPS",
            parameters=self.PARAMETERS,
            expressions=list(map(ServiceGraphics.HttpGraphics.handler_rps, self.MM_HANDLERS))
        )
        self.update(
            project_id=project_id,
            graphic_id="bulbasaur_megamind_handlers_errors_rps",
            name="Bulbasaur: Методы ММ - RPS HTTP ошибок",
            parameters=self.PARAMETERS,
            expressions=list(map(ServiceGraphics.HttpGraphics.handler_errors_rps, self.MM_HANDLERS)),
            extra_data={
                "min": 0,
                "max": 1,
            }
        )
        self.update(
            project_id=project_id,
            graphic_id="bulbasaur_megamind_handlers_errors_percent",
            name="Bulbasaur: Методы ММ - % HTTP ошибок",
            parameters=self.PARAMETERS,
            expressions=list(map(ServiceGraphics.HttpGraphics.handler_errors_percent, self.MM_HANDLERS)),
            extra_data={
                "min": 0,
                "max": 110,
            }
        )

    def update_ui_graphics(self, project_id):
        self.update(
            project_id=project_id,
            graphic_id="bulbasaur_ui_handlers_rps",
            name="Bulbasaur: Методы UI - RPS",
            parameters=self.PARAMETERS,
            expressions=list(map(ServiceGraphics.HttpGraphics.handler_rps, self.UI_HANDLERS))
        )
        self.update(
            project_id=project_id,
            graphic_id="bulbasaur_ui_handlers_errors_rps",
            name="Bulbasaur: Методы UI - RPS HTTP ошибок",
            parameters=self.PARAMETERS,
            expressions=list(map(ServiceGraphics.HttpGraphics.handler_errors_rps, self.UI_HANDLERS)),
            extra_data={
                "min": 0,
                "max": 3,
            }
        )
        self.update(
            project_id=project_id,
            graphic_id="bulbasaur_ui_handlers_errors_{period}m".format(period=5),
            name="Bulbasaur: Методы UI - Количество HTTP ошибок за {period} минут".format(period=5),
            parameters=self.PARAMETERS,
            expressions=list(
                map(
                    lambda handler: ServiceGraphics.HttpGraphics.handler_errors_in_period(
                        handler,
                        "{period}m".format(period=5),
                    ),
                    self.UI_HANDLERS,
                ),
            ),
            extra_data={
                "min": 0,
                "max": 600,
                "interpolate": "LEFT",
            }
        )
        # self.update(
        #     project_id=project_id,
        #     graphic_id="bulbasaur_ui_handlers_errors_percent",
        #     name="Bulbasaur: Методы UI - % HTTP ошибок",
        #     parameters=self.PARAMETERS,
        #     expressions=list(map(ServiceGraphics.HttpGraphics.handler_errors_percent, self.UI_HANDLERS)),
        #     extra_data={
        #         "min": 0,
        #         "max": 110,
        #     }
        # )
        # self.update(
        #     project_id=project_id,
        #     graphic_id="bulbasaur_ui_handlers_percentiles",
        #     name="Bulbasaur: Методы UI - время, с (квантиль 99.9%)",
        #     parameters=self.PARAMETERS,
        #     expressions=list(map(ServiceGraphics.HttpGraphics.handler_percentile, self.UI_HANDLERS)),
        #     extra_data={
        #         "min": 0,
        #         "max": 80,
        #     }
        # )

    def update_callback_graphics(self, project_id):
        self.update(
            project_id=project_id,
            graphic_id="bulbasaur_callback_handlers_rps",
            name="Bulbasaur: Методы Callback - RPS",
            parameters=self.PARAMETERS,
            expressions=list(map(ServiceGraphics.HttpGraphics.handler_rps, self.CALLBACK_HANDLERS))
        )
        self.update(
            project_id=project_id,
            graphic_id="bulbasaur_callback_handlers_errors_rps",
            name="Bulbasaur: Методы Callback - RPS HTTP ошибок",
            parameters=self.PARAMETERS,
            expressions=list(map(ServiceGraphics.HttpGraphics.handler_errors_rps, self.CALLBACK_HANDLERS)),
            extra_data={
                "min": 0,
                "max": 3,
            }
        )
        self.update(
            project_id=project_id,
            graphic_id="bulbasaur_callback_handlers_errors_percent",
            name="Bulbasaur: Методы Callback - % HTTP ошибок",
            parameters=self.PARAMETERS,
            expressions=list(map(ServiceGraphics.HttpGraphics.handler_errors_percent, self.CALLBACK_HANDLERS)),
            extra_data={
                "min": 0,
                "max": 110,
            }
        )
        self.update(
            project_id=project_id,
            graphic_id="bulbasaur_callback_handlers_percentiles",
            name="Bulbasaur: Методы Callback - время, с (квантиль 99.9%)",
            parameters=self.PARAMETERS,
            expressions=list(map(ServiceGraphics.HttpGraphics.handler_percentile, self.CALLBACK_HANDLERS)),
            extra_data={
                "min": 0,
                "max": 5,
            }
        )

    def update_ydb_cache_graphics(self, project_id):
        self.update(
            project_id=project_id,
            graphic_id="bulbasaur_ydb_cache_stats",
            name="Bulbasaur: YDB Cache stats - %",
            parameters=self.CACHE_PARAMETERS,
            expressions=[
                ServiceGraphics.DBGraphics.cache_stat_rps("hit"),
                ServiceGraphics.DBGraphics.cache_stat_rps("miss"),
                ServiceGraphics.DBGraphics.cache_stat_rps("not_used"),
                ServiceGraphics.DBGraphics.cache_stat_rps("ignored"),
            ],
            extra_data={
                "normalize": True,
                "scale": "LOG"
            }
        )

    def update_ydb_graphics(self, project_id):
        # Megamind UserInfo YDB Methods
        self.update(
            project_id=project_id,
            graphic_id="bulbasaur_ydb_megamind_userinfo_methods_rps",
            name="Bulbasaur: Методы YDB ММ userinfo - RPS",
            parameters=self.PARAMETERS,
            expressions=list(map(ServiceGraphics.DBGraphics.db_method_rps, self.YDB_MM_INFO_METHODS))
        )
        self.update(
            project_id=project_id,
            graphic_id="bulbasaur_ydb_megamind_userinfo_methods_errors_rps",
            name="Bulbasaur: Методы YDB ММ userinfo - RPS Ошибок",
            parameters=self.PARAMETERS,
            expressions=list(
                map(
                    lambda method: ServiceGraphics.DBGraphics.db_method_errors_rps("ydb", method),
                    self.YDB_MM_INFO_METHODS
                )
            ),
            extra_data={
                "min": 0,
                "max": 1
            }
        )
        # deprecated
        # self.update(
        #     project_id=project_id,
        #     graphic_id="bulbasaur_ydb_megamind_userinfo_methods_errors_percent",
        #     name="Bulbasaur: Методы YDB ММ userinfo - % Ошибок",
        #     parameters=self.PARAMETERS,
        #     expressions=list(map(ServiceGraphics.DBGraphics.db_method_errors_percent, self.YDB_megamind_INFO_METHODS))
        #     extra_data={
        #         "min": 0,
        #         "max": 110,
        #     }
        # )
        self.update(
            project_id=project_id,
            graphic_id="bulbasaur_ydb_megamind_userinfo_methods_percentile",
            name="Bulbasaur: Методы YDB MM userinfo - время, с (квантиль 99.9%)",
            parameters=self.PARAMETERS,
            expressions=list(map(ServiceGraphics.DBGraphics.db_method_percentile, self.YDB_MM_INFO_METHODS)),
            extra_data={
                "min": 0,
                "max": 5,
            }
        )

        # Megamind Action YDB Methods
        self.update(
            project_id=project_id,
            graphic_id="bulbasaur_ydb_megamind_action_methods_rps",
            name="Bulbasaur: Методы YDB ММ action - RPS",
            parameters=self.PARAMETERS,
            expressions=list(map(ServiceGraphics.DBGraphics.db_method_rps, self.YDB_MM_ACTION_METHODS))
        )
        self.update(
            project_id=project_id,
            graphic_id="bulbasaur_ydb_megamind_action_methods_errors_rps",
            name="Bulbasaur: Методы YDB ММ action - RPS Ошибок",
            parameters=self.PARAMETERS,
            expressions=list(
                map(
                    lambda method: ServiceGraphics.DBGraphics.db_method_errors_rps("ydb", method),
                    self.YDB_MM_ACTION_METHODS
                )
            ),
            extra_data={
                "min": 0,
                "max": 1
            }
        )
        # deprecated
        # self.update(
        #     project_id=project_id,
        #     graphic_id="bulbasaur_ydb_megamind_action_methods_errors_percent",
        #     name="Bulbasaur: Методы YDB ММ action - % Ошибок",
        #     parameters=self.PARAMETERS,
        #     expressions=list(
        #         map(ServiceGraphics.DBGraphics.db_method_errors_percent, self.YDB_megamind_ACTION_METHODS)
        #     ),
        #     extra_data={
        #         "min": 0,
        #         "max": 110,
        #     }
        # )
        self.update(
            project_id=project_id,
            graphic_id="bulbasaur_ydb_megamind_action_methods_percentile",
            name="Bulbasaur: Методы YDB MM action - время, с (квантиль 99.9%)",
            parameters=self.PARAMETERS,
            expressions=list(map(ServiceGraphics.DBGraphics.db_method_percentile, self.YDB_MM_ACTION_METHODS)),
            extra_data={
                "min": 0,
                "max": 5,
            }
        )

        # Other YDB Methods
        self.update(
            project_id=project_id,
            graphic_id="bulbasaur_ydb_other_methods_rps",
            name="Bulbasaur: Методы YDB others - RPS",
            parameters=self.PARAMETERS,
            expressions=list(map(ServiceGraphics.DBGraphics.db_method_rps, self.YDB_OTHER_METHODS))
        )
        self.update(
            project_id=project_id,
            graphic_id="bulbasaur_ydb_other_methods_errors_rps",
            name="Bulbasaur: Методы YDB others - RPS Ошибок",
            parameters=self.PARAMETERS,
            expressions=list(
                map(
                    lambda method: ServiceGraphics.DBGraphics.db_method_errors_rps("ydb", method),
                    self.YDB_OTHER_METHODS
                )
            ),
            extra_data={
                "min": 0,
                "max": 1
            }
        )
        # deprecated
        # self.update(
        #     project_id=project_id,
        #     graphic_id="bulbasaur_ydb_other_methods_errors_percent",
        #     name="Bulbasaur: Методы YDB others - % Ошибок",
        #     parameters=self.PARAMETERS,
        #     expressions=list(map(ServiceGraphics.DBGraphics.db_method_errors_percent, self.YDB_OTHER_METHODS)),
        #     extra_data={
        #         "min": 0,
        #         "max": 110,
        #     }
        # )
        self.update(
            project_id=project_id,
            graphic_id="bulbasaur_ydb_other_methods_percentile",
            name="Bulbasaur: Методы YDB others - время, с (квантиль 99.9%)",
            parameters=self.PARAMETERS,
            expressions=list(map(ServiceGraphics.DBGraphics.db_method_percentile, self.YDB_OTHER_METHODS)),
            extra_data={
                "min": 0,
                "max": 5,
            }
        )

    def update_perf_graphics(self, project_id):
        self.update_perf_general_graphics(project_id)
        self.update_perf_gc_graphics(project_id)
        self.update_perf_memory_graphics(project_id)

    def update_perf_general_graphics(self, project_id):
        self.update(
            project_id=project_id,
            graphic_id="bulbasaur_perf_goroutines_count",
            name="Bulbasaur: Горутины - Кол-во",
            parameters=self.PERF_PARAMETERS,
            expressions=list(map(ServiceGraphics.PerfGraphics.goroutines_count, self.DC))
        )
        self.update(
            project_id=project_id,
            graphic_id="bulbasaur_perf_total_sys_alloc",
            name="Bulbasaur: Total sys - alloc, Mb",
            parameters=self.PERF_PARAMETERS,
            expressions=list(map(lambda dc: ServiceGraphics.PerfGraphics.total_sys_alloc(dc, 1024 * 1024), self.DC))
        )

    def update_perf_gc_graphics(self, project_id):
        self.update(
            project_id=project_id,
            graphic_id="bulbasaur_perf_gc_count",
            name="Bulbasaur: Циклы GC - Кол-во",
            parameters=self.PERF_PARAMETERS,
            expressions=list(map(ServiceGraphics.PerfGraphics.gc_count, self.DC)),
            extra_data={
                "min": "0",
            }
        )
        self.update(
            project_id=project_id,
            graphic_id="bulbasaur_perf_gc_pause",
            name="Bulbasaur: Циклы GC - Время, мс",
            parameters=self.PERF_PARAMETERS,
            expressions=list(map(ServiceGraphics.PerfGraphics.gc_pauses, self.DC)),
            extra_data={
                "min": "0",
            }
        )

    def update_perf_memory_graphics(self, project_id):
        self.update(
            project_id=project_id,
            graphic_id="bulbasaur_perf_memory_heap_alloc",
            name="Bulbasaur: Heap - Alloc, Mb",
            parameters=self.PERF_PARAMETERS,
            expressions=list(map(lambda dc: ServiceGraphics.PerfGraphics.heap_alloc(dc, 1024 * 1024), self.DC))
        )
        self.update(
            project_id=project_id,
            graphic_id="bulbasaur_perf_memory_heap_objects",
            name="Bulbasaur: Heap objects - Кол-во",
            parameters=self.PERF_PARAMETERS,
            expressions=list(map(ServiceGraphics.PerfGraphics.heap_objects, self.DC))
        )
        self.update(
            project_id=project_id,
            graphic_id="bulbasaur_perf_memory_heap_released",
            name="Bulbasaur: Heap - Released, Mb",
            parameters=self.PERF_PARAMETERS,
            expressions=list(map(lambda dc: ServiceGraphics.PerfGraphics.heap_released(dc, 1024 * 1024), self.DC))
        )
        self.update(
            project_id=project_id,
            graphic_id="bulbasaur_perf_memory_heap_in_use",
            name="Bulbasaur: Heap - In use, Mb",
            parameters=self.PERF_PARAMETERS,
            expressions=list(
                map(lambda dc: ServiceGraphics.PerfGraphics.memtype_in_use(dc, "heap", 1024 * 1024), self.DC))
        )
        self.update(
            project_id=project_id,
            graphic_id="bulbasaur_perf_memory_stack_in_use",
            name="Bulbasaur: Stack - In use, Mb",
            parameters=self.PERF_PARAMETERS,
            expressions=list(
                map(lambda dc: ServiceGraphics.PerfGraphics.memtype_in_use(dc, "stack", 1024 * 1024), self.DC))
        )

    def update_by_dc_graphics(self, project_id):
        self.update(
            project_id=project_id,
            graphic_id="bulbasaur_mobile_rps_stacked_dc",
            name="Bulbasaur: Mobile RPS stacked by DC",
            parameters=self.BY_DC_PARAMETERS,
            expressions=list(
                map(
                    lambda dc: ServiceGraphics.ByDCGraphics.total_path_rps_in_dc(dc.with_area(), "/m/*"), self.DC
                )
            )
        )
        self.update(
            project_id=project_id,
            graphic_id="bulbasaur_mm_rps_stacked_dc",
            name="Bulbasaur: MM RPS stacked by DC",
            parameters=self.BY_DC_PARAMETERS,
            expressions=list(
                map(
                    lambda dc: ServiceGraphics.ByDCGraphics.total_path_rps_in_dc(dc.with_area(), "/v1.0/*|/megamind/*"),
                    self.DC
                )
            )
        )
        for code in ["4xx", "5xx"]:
            self.update(
                project_id=project_id,
                graphic_id="bulbasaur_mobile_{code}_rps_stacked_dc".format(code=code),
                name="Bulbasaur: Mobile {code} stacked by DC, RPS".format(code=code),
                parameters=self.BY_DC_PARAMETERS,
                expressions=list(
                    map(
                        lambda dc: ServiceGraphics.ByDCGraphics.error_code_rps_in_dc(dc, code, "/m/*"), self.DC
                    )
                ),
                extra_data={
                    "min": 0,
                    "max": 2,
                }
            )
            self.update(
                project_id=project_id,
                graphic_id="bulbasaur_mm_{code}_rps_stacked_dc".format(code=code),
                name="Bulbasaur: MM {code} stacked by DC, RPS".format(code=code),
                parameters=self.BY_DC_PARAMETERS,
                expressions=list(
                    map(
                        lambda dc: ServiceGraphics.ByDCGraphics.error_code_rps_in_dc(dc, code, "/v1.0/*|/megamind/*"),
                        self.DC
                    )
                ),
                extra_data={
                    "min": 0,
                    "max": 2,
                }
            )
            self.update(
                project_id=project_id,
                graphic_id="bulbasaur_mobile_{code}_percent_stacked_dc".format(code=code),
                name="Bulbasaur: Mobile {code} stacked by DC, %".format(code=code),
                parameters=self.BY_DC_PARAMETERS,
                expressions=list(
                    map(
                        lambda dc: ServiceGraphics.ByDCGraphics.error_code_percent_in_dc(dc, code, "/m/*"), self.DC
                    )
                ),
                extra_data={
                    "min": 0,
                    "max": 110,
                }
            )
            self.update(
                project_id=project_id,
                graphic_id="bulbasaur_mm_{code}_percent_stacked_dc".format(code=code),
                name="Bulbasaur: MM {code} stacked by DC, %".format(code=code),
                parameters=self.BY_DC_PARAMETERS,
                expressions=list(
                    map(
                        lambda dc: ServiceGraphics.ByDCGraphics.error_code_percent_in_dc(dc, code,
                                                                                         "/v1.0/*|/megamind/*"), self.DC
                    )
                ),
                extra_data={
                    "min": 0,
                    "max": 110,
                }
            )
        for dc in self.DC:
            self.update(
                project_id=project_id,
                graphic_id="bulbasaur_ui_handlers_percentiles_dc_{host}".format(host=dc.name.lower()),
                name="Bulbasaur: Методы UI - время {host}, с (квантиль 99.9%)".format(host=dc.name.upper()),
                parameters=self.BY_DC_PARAMETERS,
                expressions=list(
                    map(lambda handler: ServiceGraphics.ByDCGraphics.handler_percentile_in_dc(handler, dc.name),
                        self.UI_HANDLERS)),
                extra_data={
                    "min": 0,
                    "max": 35,
                }
            )
            self.update(
                project_id=project_id,
                graphic_id="bulbasaur_mm_handlers_percentiles_dc_{host}".format(host=dc.name.lower()),
                name="Bulbasaur: Методы MM - время {host}, с (квантиль 99.9%)".format(host=dc.name.upper()),
                parameters=self.BY_DC_PARAMETERS,
                expressions=list(
                    map(lambda handler: ServiceGraphics.ByDCGraphics.handler_percentile_in_dc(handler, dc.name),
                        self.MM_HANDLERS)),
                extra_data={
                    "min": 0,
                    "max": 5,
                }
            )

    def update_neighbour_graphics(self, project_id):
        def get_parameters(neighbour):
            parameters = self.PARAMETERS
            if neighbour == "socialism":
                parameters = self.SOCIALISM_PARAMETERS
            return parameters

        def get_calls(neighbour):
            if neighbour == "dialogs":
                return self.DIALOG_API_CALLS
            elif neighbour == "blackbox":
                return self.BLACKBOX_API_CALLS
            elif neighbour == "socialism":
                return self.SOCIALISM_API_CALLS
            elif neighbour == "tvm":
                return self.TVM_API_CALLS
            elif neighbour == "xiva":
                return self.XIVA_API_CALLS
            elif neighbour == "sup":
                return self.SUP_API_CALLS
            elif neighbour == "steelix":
                return self.STEELIX_API_CALLS
            elif neighbour == "tuya":
                return self.TUYA_API_CALLS
            elif neighbour == "time_machine":
                return self.TIME_MACHINE_API_CALLS
            elif neighbour == "bass":
                return self.BASS_API_CALLS
            elif neighbour == "begemot":
                return self.BEGEMOT_API_CALLS
            elif neighbour == "memento":
                return self.MEMENTO_API_CALLS
            elif neighbour == "datasync":
                return self.DATASYNC_API_CALLS
            elif neighbour == "geosuggest":
                return self.GEOSUGGEST_API_CALLS
            elif neighbour == "solomon_api":
                return self.SOLOMON_API_CALLS
            elif neighbour == "notificator":
                return self.NOTIFICATOR_API_CALLS
            elif neighbour == "oauth":
                return self.OAUTH_API_CALLS
            elif neighbour == "unified_agent":
                return self.UNIFIED_AGENT_API_CALLS
            else:
                return {}

        for neighbour in self.NEIGHBOURS:
            self.update(
                project_id=project_id,
                graphic_id="{neighbour}_api_calls_rps".format(neighbour=neighbour.lower()),
                name="{neighbour}: Методы API - RPS".format(neighbour=neighbour.capitalize()),
                parameters=get_parameters(neighbour),
                expressions=list(map(
                    lambda c: ServiceGraphics.NeighbourGraphics.call_rps(neighbour, c), get_calls(neighbour)
                )),
            )
            self.update(
                project_id=project_id,
                graphic_id="{neighbour}_api_errors".format(neighbour=neighbour.lower()),
                name="{neighbour}: Методы API - Ошибки, %".format(neighbour=neighbour.capitalize()),
                parameters=get_parameters(neighbour),
                expressions=[
                    ServiceGraphics.NeighbourGraphics.call_total_http_code_percent(neighbour, self.CODES["4xx"]),
                    ServiceGraphics.NeighbourGraphics.call_total_http_code_percent(neighbour, self.CODES["5xx"]),
                    ServiceGraphics.NeighbourGraphics.call_total_unavailable_percent(neighbour, self.CODES["fails"]),
                ],
                extra_data={
                    "min": 0,
                    "max": 110,
                }
            )
            for call in get_calls(neighbour):
                self.update(
                    project_id=project_id,
                    graphic_id="{neighbour}_api_call_{call}_rps".format(
                        neighbour=neighbour.lower(), call=call.name
                    ),
                    name="{neighbour}: Методы API - {call}, RPS".format(neighbour=neighbour.capitalize(),
                                                                        call=call.name),
                    parameters=get_parameters(neighbour),
                    expressions=[
                        ServiceGraphics.NeighbourGraphics.call_unavailable_rps(neighbour, call, self.CODES["fails"]),
                        ServiceGraphics.NeighbourGraphics.call_http_code_rps(neighbour, call, self.CODES["4xx"]),
                        ServiceGraphics.NeighbourGraphics.call_http_code_rps(neighbour, call, self.CODES["5xx"]),
                        ServiceGraphics.NeighbourGraphics.call_http_code_rps(neighbour, call, self.CODES["2xx"]),
                    ]
                )
                self.update(
                    project_id=project_id,
                    graphic_id="{neighbour}_api_call_{call}_percent".format(
                        neighbour=neighbour.lower(), call=call.name
                    ),
                    name="{neighbour}: Методы API - {call}, %".format(neighbour=neighbour.capitalize(), call=call.name),
                    parameters=get_parameters(neighbour),
                    expressions=[
                        ServiceGraphics.NeighbourGraphics.call_unavailable_rps(neighbour, call, self.CODES["fails"]),
                        ServiceGraphics.NeighbourGraphics.call_http_code_rps(neighbour, call, self.CODES["4xx"]),
                        ServiceGraphics.NeighbourGraphics.call_http_code_rps(neighbour, call, self.CODES["5xx"]),
                        ServiceGraphics.NeighbourGraphics.call_http_code_rps(neighbour, call, self.CODES["2xx"]),
                    ],
                    extra_data={
                        "normalize": True,
                        "min": 0,
                        "max": 110,
                    }
                )
            self.update(
                project_id=project_id,
                graphic_id="{neighbour}_api_call_percentiles".format(neighbour=neighbour.lower()),
                name="{neighbour}: Методы API - Тайминги, с (квантиль 99.9%)".format(neighbour=neighbour.capitalize()),
                parameters=get_parameters(neighbour),
                expressions=list(map(
                    lambda c: ServiceGraphics.NeighbourGraphics.call_percentile(neighbour, c), get_calls(neighbour)
                )),
                extra_data={
                    "interpolate": "RIGHT",
                }
            )


class BulbasaurDashboards(DashboardBuilder):
    PROJECT = "alice-iot"
    CLUSTER = "bulbasaur_production|bulbasaur_beta|bulbasaur_dev"
    SERVICE = "bulbasaur"

    PARAMETERS = [
        {
            "name": "project",
            "value": PROJECT,
        },
        {
            "name": "cluster",
            "value": CLUSTER,
        },
        {
            "name": "service",
            "value": SERVICE,
        },
        {
            "name": "host",
            "value": "*",
        }
    ]
    YDB_HANDLER_PARAMETERS = [
        {
            "name": "project",
            "value": PROJECT,
        },
        {
            "name": "cluster",
            "value": CLUSTER,
        },
        {
            "name": "service",
            "value": SERVICE,
        },
        {
            "name": "host",
            "value": "*",
        },
        {
            "name": "db_method",
            "value": "*",
        }
    ]
    SOCIALISM_PARAMETERS = [
        {
            "name": "project",
            "value": PROJECT,
        },
        {
            "name": "cluster",
            "value": CLUSTER,
        },
        {
            "name": "service",
            "value": SERVICE,
        },
        {
            "name": "host",
            "value": "*",
        },
        {
            "name": "skill_id",
            "value": "*",
        }
    ]
    BY_DC_PARAMETERS = [
        {
            "name": "project",
            "value": PROJECT,
        },
        {
            "name": "cluster",
            "value": "bulbasaur_production",
        },
        {
            "name": "service",
            "value": SERVICE,
        }
    ]
    PERF_PARAMETERS = [
        {
            "name": "project",
            "value": PROJECT,
        },
        {
            "name": "cluster",
            "value": CLUSTER,
        },
        {
            "name": "service",
            "value": SERVICE,
        }
    ]

    def __init__(self, oauth_token):
        DashboardBuilder.__init__(self, oauth_token)

    def update_dashboards(self, project_id):
        # self.update_service_dashboard(project_id)
        # self.update_service_by_dc_dashboard(project_id)
        # self.update_processors_dashboard(project_id)
        # self.update_ydb_dashboard(project_id)
        # self.update_ydb_handler_dashboard(project_id)
        # self.update_perf_dashboard(project_id)
        self.update_neighbours_dashboard(project_id)

    def update_service_dashboard(self, project_id):
        self.update(
            project_id=project_id,
            dashboard_id="bulbasaur_service_dashboard",
            name="Bulbasaur: Service stats",
            parameters=self.PARAMETERS,
            rows=[
                [
                    Panel(
                        title="Методы UI - RPS",
                        url=DashboardBuilder.url_query(
                            "project", project_id,
                            "cluster", "{{cluster}}",
                            "service", "{{service}}",
                            "host", "{{host}}",
                            "graph", "bulbasaur_ui_handlers_rps",
                            "legend", "off"
                        ),
                        rowspan=1,
                        colspan=6,
                    ),
                    Panel(
                        title="Методы UI - время, с (квантиль 99.9%)",
                        url=DashboardBuilder.url_query(
                            "project", project_id,
                            "cluster", "{{cluster}}",
                            "service", "{{service}}",
                            "host", "{{host}}",
                            "graph", "bulbasaur_ui_handlers_percentiles",
                            "legend", "off"
                        ),
                        rowspan=2,
                        colspan=3,
                    ),
                ],
                [
                    Panel(
                        title="Методы UI - RPS HTTP Ошибок",
                        url=DashboardBuilder.url_query(
                            "project", project_id,
                            "cluster", "{{cluster}}",
                            "service", "{{service}}",
                            "host", "{{host}}",
                            "graph", "bulbasaur_ui_handlers_errors_rps",
                            "legend", "off"
                        ),
                        rowspan=1,
                        colspan=2,
                    ),
                    Panel(
                        title="Методы UI - % HTTP Ошибок",
                        url=DashboardBuilder.url_query(
                            "project", project_id,
                            "cluster", "{{cluster}}",
                            "service", "{{service}}",
                            "host", "{{host}}",
                            "graph", "bulbasaur_ui_handlers_errors_percent",
                            "legend", "off"
                        ),
                        rowspan=1,
                        colspan=2,
                    ),
                    Panel(
                        title="Методы UI - Количество HTTP Ошибок за 5 минут",
                        url=DashboardBuilder.url_query(
                            "project", project_id,
                            "cluster", "{{cluster}}",
                            "service", "{{service}}",
                            "host", "{{host}}",
                            "graph", "bulbasaur_ui_handlers_errors_5m",
                            "legend", "off"
                        ),
                        rowspan=1,
                        colspan=2,
                    ),
                ],
                [
                    Panel(
                        title="Методы Callback - RPS",
                        url=DashboardBuilder.url_query(
                            "project", project_id,
                            "cluster", "{{cluster}}",
                            "service", "{{service}}",
                            "host", "{{host}}",
                            "graph", "bulbasaur_callback_handlers_rps",
                            "legend", "off"
                        ),
                        rowspan=1,
                        colspan=3,
                    ),
                    Panel(
                        title="Методы Callback - RPS by skill_id",
                        url=DashboardBuilder.url_query(
                            "project", project_id,
                            "cluster", "iot-prod",
                            "service", "smart-home",
                            "host", "{{host}}",
                            "graph", "provider_callbacks_per_skill_id",
                            "legend", "off"
                        ),
                        rowspan=1,
                        colspan=3,
                    ),
                    Panel(
                        title="Методы Callback - время, с (квантиль 99.9%)",
                        url=DashboardBuilder.url_query(
                            "project", project_id,
                            "cluster", "{{cluster}}",
                            "service", "{{service}}",
                            "host", "{{host}}",
                            "graph", "bulbasaur_callback_handlers_percentiles",
                            "legend", "off"
                        ),
                        rowspan=2,
                        colspan=3,
                    ),
                ],
                [
                    Panel(
                        title="Методы Callback - RPS HTTP Ошибок",
                        url=DashboardBuilder.url_query(
                            "project", project_id,
                            "cluster", "{{cluster}}",
                            "service", "{{service}}",
                            "host", "{{host}}",
                            "graph", "bulbasaur_callback_handlers_errors_rps",
                            "legend", "off"
                        ),
                        rowspan=1,
                        colspan=3,
                    ),
                    Panel(
                        title="Методы Callback - % HTTP Ошибок",
                        url=DashboardBuilder.url_query(
                            "project", project_id,
                            "cluster", "{{cluster}}",
                            "service", "{{service}}",
                            "host", "{{host}}",
                            "graph", "bulbasaur_callback_handlers_errors_percent",
                            "legend", "off"
                        ),
                        rowspan=1,
                        colspan=3,
                    ),
                ],
                [
                    Panel(
                        title="Получение пользовательской информации - RPS",
                        url=DashboardBuilder.url_query(
                            "project", project_id,
                            "cluster", "{{cluster}}",
                            "service", "{{service}}",
                            "host", "{{host}}",
                            "graph", "bulbasaur_uniproxy_handlers_rps",
                            "legend", "off"
                        ),
                        rowspan=1,
                        colspan=6,
                    ),
                    Panel(
                        title="Получение пользовательской информации - время, с (квантиль 99.9%)",
                        url=DashboardBuilder.url_query(
                            "project", project_id,
                            "cluster", "{{cluster}}",
                            "service", "{{service}}",
                            "host", "{{host}}",
                            "graph", "bulbasaur_uniproxy_handlers_percentiles",
                            "legend", "off"
                        ),
                        rowspan=1,
                        colspan=3,
                    ),
                ],
                [
                    Panel(
                        title="Получение пользовательской информации - RPS HTTP Ошибок",
                        url=DashboardBuilder.url_query(
                            "project", project_id,
                            "cluster", "{{cluster}}",
                            "service", "{{service}}",
                            "host", "{{host}}",
                            "graph", "bulbasaur_uniproxy_handlers_errors_rps",
                            "legend", "off"
                        ),
                        rowspan=1,
                        colspan=3,
                    ),
                    Panel(
                        title="Получение пользовательской информации - % HTTP Ошибок",
                        url=DashboardBuilder.url_query(
                            "project", project_id,
                            "cluster", "{{cluster}}",
                            "service", "{{service}}",
                            "host", "{{host}}",
                            "graph", "bulbasaur_uniproxy_handlers_errors_percent",
                            "legend", "off"
                        ),
                        rowspan=1,
                        colspan=3,
                    ),
                    Panel(
                        title="YDB Cache - % Stats",
                        url=DashboardBuilder.url_query(
                            "project", project_id,
                            "cluster", "{{cluster}}",
                            "service", "{{service}}",
                            "host", "{{host}}",
                            "cache", "pumpkin",
                            "graph", "bulbasaur_ydb_cache_stats",
                            "legend", "off"
                        ),
                        rowspan=1,
                        colspan=3,
                    ),
                ],
                [
                    Panel(
                        title="Методы MM - RPS",
                        url=DashboardBuilder.url_query(
                            "project", project_id,
                            "cluster", "{{cluster}}",
                            "service", "{{service}}",
                            "host", "{{host}}",
                            "graph", "bulbasaur_megamind_handlers_rps",
                            "legend", "off"
                        ),
                        rowspan=1,
                        colspan=6,
                    ),
                    Panel(
                        title="Методы MM Run - время, с (квантиль 99.9%)",
                        url=DashboardBuilder.url_query(
                            "project", project_id,
                            "cluster", "{{cluster}}",
                            "service", "{{service}}",
                            "host", "{{host}}",
                            "graph", "bulbasaur_processor_percentiles",
                            "step", "run",
                            "legend", "off"
                        ),
                        rowspan=1,
                        colspan=3,
                    )
                ],
                [
                    Panel(
                        title="Методы MM - RPS HTTP Ошибок",
                        url=DashboardBuilder.url_query(
                            "project", project_id,
                            "cluster", "{{cluster}}",
                            "service", "{{service}}",
                            "host", "{{host}}",
                            "graph", "bulbasaur_megamind_handlers_errors_rps",
                            "legend", "off"
                        ),
                        rowspan=1,
                        colspan=3,
                    ),
                    Panel(
                        title="Методы MM - % HTTP Ошибок",
                        url=DashboardBuilder.url_query(
                            "project", project_id,
                            "cluster", "{{cluster}}",
                            "service", "{{service}}",
                            "host", "{{host}}",
                            "graph", "bulbasaur_megamind_handlers_errors_percent",
                            "legend", "off"
                        ),
                        rowspan=1,
                        colspan=3,
                    ),
                    Panel(
                        title="Методы MM Apply - время, с (квантиль 99.9%)",
                        url=DashboardBuilder.url_query(
                            "project", project_id,
                            "cluster", "{{cluster}}",
                            "service", "{{service}}",
                            "host", "{{host}}",
                            "graph", "bulbasaur_processor_percentiles",
                            "step", "apply",
                            "legend", "off"
                        ),
                        rowspan=1,
                        colspan=3,
                    )
                ],
                [
                    Panel(
                        title="Методы API - RPS",
                        url=DashboardBuilder.url_query(
                            "project", project_id,
                            "cluster", "{{cluster}}",
                            "service", "{{service}}",
                            "host", "{{host}}",
                            "graph", "bulbasaur_api_handlers_rps",
                            "legend", "off"
                        ),
                        rowspan=1,
                        colspan=6,
                    ),
                    Panel(
                        title="Методы API - время, с (квантиль 99.9%)",
                        url=DashboardBuilder.url_query(
                            "project", project_id,
                            "cluster", "{{cluster}}",
                            "service", "{{service}}",
                            "host", "{{host}}",
                            "graph", "bulbasaur_api_handlers_percentiles",
                            "legend", "off"
                        ),
                        rowspan=2,
                        colspan=3,
                    ),
                ],
                [
                    Panel(
                        title="Методы API - RPS HTTP Ошибок",
                        url=DashboardBuilder.url_query(
                            "project", project_id,
                            "cluster", "{{cluster}}",
                            "service", "{{service}}",
                            "host", "{{host}}",
                            "graph", "bulbasaur_api_handlers_errors_rps",
                            "legend", "off"
                        ),
                        rowspan=1,
                        colspan=3,
                    ),
                    Panel(
                        title="Методы API - % HTTP Ошибок",
                        url=DashboardBuilder.url_query(
                            "project", project_id,
                            "cluster", "{{cluster}}",
                            "service", "{{service}}",
                            "host", "{{host}}",
                            "graph", "bulbasaur_api_handlers_errors_percent",
                            "legend", "off"
                        ),
                        rowspan=1,
                        colspan=3,
                    ),
                ],
            ]
        )

    def update_service_by_dc_dashboard(self, project_id):
        self.update(
            project_id=project_id,
            dashboard_id="bulbasaur_service_by_dc_dashboard",
            name="Bulbasaur: Service stats by DC",
            parameters=self.BY_DC_PARAMETERS,
            rows=[
                [
                    Panel(
                        title="Методы UI - RPS stacked",
                        url=DashboardBuilder.url_query(
                            "project", project_id,
                            "cluster", "{{cluster}}",
                            "service", "{{service}}",
                            "graph", "bulbasaur_mobile_rps_stacked_dc",
                            "legend", "off"
                        ),
                        rowspan=1,
                        colspan=6,
                    ),
                ],
                [
                    Panel(
                        title="Методы MM - RPS stacked",
                        url=DashboardBuilder.url_query(
                            "project", project_id,
                            "cluster", "{{cluster}}",
                            "service", "{{service}}",
                            "graph", "bulbasaur_mm_rps_stacked_dc",
                            "legend", "off"
                        ),
                        rowspan=1,
                        colspan=6,
                    ),
                ],
                [
                    Panel(
                        title="Методы UI - 4xx RPS stacked",
                        url=DashboardBuilder.url_query(
                            "project", project_id,
                            "cluster", "{{cluster}}",
                            "service", "{{service}}",
                            "graph", "bulbasaur_mobile_4xx_rps_stacked_dc",
                            "legend", "off"
                        ),
                        rowspan=1,
                        colspan=3,
                    ),
                    Panel(
                        title="Методы UI - 4xx % stacked",
                        url=DashboardBuilder.url_query(
                            "project", project_id,
                            "cluster", "{{cluster}}",
                            "service", "{{service}}",
                            "graph", "bulbasaur_mobile_4xx_percent_stacked_dc",
                            "legend", "off"
                        ),
                        rowspan=1,
                        colspan=3,
                    ),
                ],
                [
                    Panel(
                        title="Методы UI - 5xx RPS stacked",
                        url=DashboardBuilder.url_query(
                            "project", project_id,
                            "cluster", "{{cluster}}",
                            "service", "{{service}}",
                            "graph", "bulbasaur_mobile_5xx_rps_stacked_dc",
                            "legend", "off"
                        ),
                        rowspan=1,
                        colspan=3,
                    ),
                    Panel(
                        title="Методы UI - 5xx % stacked",
                        url=DashboardBuilder.url_query(
                            "project", project_id,
                            "cluster", "{{cluster}}",
                            "service", "{{service}}",
                            "graph", "bulbasaur_mobile_5xx_percent_stacked_dc",
                            "legend", "off"
                        ),
                        rowspan=1,
                        colspan=3,
                    ),
                ],
                [
                    Panel(
                        title="Методы MM - 4xx RPS stacked",
                        url=DashboardBuilder.url_query(
                            "project", project_id,
                            "cluster", "{{cluster}}",
                            "service", "{{service}}",
                            "graph", "bulbasaur_mm_4xx_rps_stacked_dc",
                            "legend", "off"
                        ),
                        rowspan=1,
                        colspan=3,
                    ),
                    Panel(
                        title="Методы MM - 4xx % stacked",
                        url=DashboardBuilder.url_query(
                            "project", project_id,
                            "cluster", "{{cluster}}",
                            "service", "{{service}}",
                            "graph", "bulbasaur_mm_4xx_percent_stacked_dc",
                            "legend", "off"
                        ),
                        rowspan=1,
                        colspan=3,
                    ),
                ],
                [
                    Panel(
                        title="Методы MM - 5xx RPS stacked",
                        url=DashboardBuilder.url_query(
                            "project", project_id,
                            "cluster", "{{cluster}}",
                            "service", "{{service}}",
                            "graph", "bulbasaur_mm_5xx_rps_stacked_dc",
                            "legend", "off"
                        ),
                        rowspan=1,
                        colspan=3,
                    ),
                    Panel(
                        title="Методы MM - 5xx % stacked",
                        url=DashboardBuilder.url_query(
                            "project", project_id,
                            "cluster", "{{cluster}}",
                            "service", "{{service}}",
                            "graph", "bulbasaur_mm_5xx_percent_stacked_dc",
                            "legend", "off"
                        ),
                        rowspan=1,
                        colspan=3,
                    ),
                ],
                [
                    Panel(
                        title="YDB Cache - % Stats MAN",
                        url=DashboardBuilder.url_query(
                            "project", project_id,
                            "cluster", "{{cluster}}",
                            "service", "{{service}}",
                            "host", "Man",
                            "cache", "pumpkin",
                            "graph", "bulbasaur_ydb_cache_stats",
                            "legend", "off"
                        ),
                        rowspan=1,
                        colspan=2,
                    ),
                    Panel(
                        title="YDB Cache - % Stats SAS",
                        url=DashboardBuilder.url_query(
                            "project", project_id,
                            "cluster", "{{cluster}}",
                            "service", "{{service}}",
                            "host", "Sas",
                            "cache", "pumpkin",
                            "graph", "bulbasaur_ydb_cache_stats",
                            "legend", "off"
                        ),
                        rowspan=1,
                        colspan=2,
                    ),
                    Panel(
                        title="YDB Cache - % Stats VLA",
                        url=DashboardBuilder.url_query(
                            "project", project_id,
                            "cluster", "{{cluster}}",
                            "service", "{{service}}",
                            "host", "Vla",
                            "cache", "pumpkin",
                            "graph", "bulbasaur_ydb_cache_stats",
                            "legend", "off"
                        ),
                        rowspan=1,
                        colspan=2,
                    )
                ],
                [
                    Panel(
                        title="Методы UI - время MAN, с (квантиль 99.9%)",
                        url=DashboardBuilder.url_query(
                            "project", project_id,
                            "cluster", "{{cluster}}",
                            "service", "{{service}}",
                            "graph", "bulbasaur_ui_handlers_percentiles_dc_man",
                            "legend", "off"
                        ),
                        rowspan=1,
                        colspan=2,
                    ),
                    Panel(
                        title="Методы UI - время SAS, с (квантиль 99.9%)",
                        url=DashboardBuilder.url_query(
                            "project", project_id,
                            "cluster", "{{cluster}}",
                            "service", "{{service}}",
                            "graph", "bulbasaur_ui_handlers_percentiles_dc_sas",
                            "legend", "off"
                        ),
                        rowspan=1,
                        colspan=2,
                    ),
                    Panel(
                        title="Методы UI - время VLA, с (квантиль 99.9%)",
                        url=DashboardBuilder.url_query(
                            "project", project_id,
                            "cluster", "{{cluster}}",
                            "service", "{{service}}",
                            "graph", "bulbasaur_ui_handlers_percentiles_dc_vla",
                            "legend", "off"
                        ),
                        rowspan=1,
                        colspan=2,
                    ),
                ],
                [
                    Panel(
                        title="Методы MM UserInfo - время MAN, с (квантиль 99.9%)",
                        url=DashboardBuilder.url_query(
                            "project", project_id,
                            "cluster", "{{cluster}}",
                            "service", "{{service}}",
                            "host", "Man",
                            "sensor", "handlers.duration_buckets",
                            "path", "/v1.0/user/info",
                            "overLinesTransform", "WEIGHTED_PERCENTILE",
                            "percentiles", "99.9",
                            "min", 0,
                            "graph", "auto",
                            "cs", "default",
                            "legend", "off"
                        ),
                        rowspan=1,
                        colspan=2,
                    ),
                    Panel(
                        title="Методы MM UserInfo - время SAS, с (квантиль 99.9%)",
                        url=DashboardBuilder.url_query(
                            "project", project_id,
                            "cluster", "{{cluster}}",
                            "service", "{{service}}",
                            "host", "Sas",
                            "sensor", "handlers.duration_buckets",
                            "path", "/v1.0/user/info",
                            "overLinesTransform", "WEIGHTED_PERCENTILE",
                            "percentiles", "99.9",
                            "min", 0,
                            "graph", "auto",
                            "cs", "default",
                            "legend", "off"
                        ),
                        rowspan=1,
                        colspan=2,
                    ),
                    Panel(
                        title="Методы MM UserInfo - время VLA, с (квантиль 99.9%)",
                        url=DashboardBuilder.url_query(
                            "project", project_id,
                            "cluster", "{{cluster}}",
                            "service", "{{service}}",
                            "host", "Vla",
                            "sensor", "handlers.duration_buckets",
                            "path", "/v1.0/user/info",
                            "overLinesTransform", "WEIGHTED_PERCENTILE",
                            "percentiles", "99.9",
                            "min", 0,
                            "graph", "auto",
                            "cs", "default",
                            "legend", "off"
                        ),
                        rowspan=1,
                        colspan=2,
                    ),
                ],
                [
                    Panel(
                        title="Методы MM Run - время MAN, с (квантиль 99.9%)",
                        url=DashboardBuilder.url_query(
                            "project", project_id,
                            "cluster", "{{cluster}}",
                            "service", "{{service}}",
                            "host", "Man",
                            "graph", "bulbasaur_processor_percentiles",
                            "step", "run",
                            "legend", "off"
                        ),
                        rowspan=1,
                        colspan=2,
                    ),
                    Panel(
                        title="Методы MM Run - время SAS, с (квантиль 99.9%)",
                        url=DashboardBuilder.url_query(
                            "project", project_id,
                            "cluster", "{{cluster}}",
                            "service", "{{service}}",
                            "host", "Sas",
                            "graph", "bulbasaur_processor_percentiles",
                            "step", "run",
                            "legend", "off"
                        ),
                        rowspan=1,
                        colspan=2,
                    ),
                    Panel(
                        title="Методы MM Run - время VLA, с (квантиль 99.9%)",
                        url=DashboardBuilder.url_query(
                            "project", project_id,
                            "cluster", "{{cluster}}",
                            "service", "{{service}}",
                            "host", "Vla",
                            "graph", "bulbasaur_processor_percentiles",
                            "step", "run",
                            "legend", "off"
                        ),
                        rowspan=1,
                        colspan=2,
                    ),
                ],
                [
                    Panel(
                        title="Методы MM Apply - время MAN, с (квантиль 99.9%)",
                        url=DashboardBuilder.url_query(
                            "project", project_id,
                            "cluster", "{{cluster}}",
                            "service", "{{service}}",
                            "host", "Man",
                            "graph", "bulbasaur_processor_percentiles",
                            "step", "apply",
                            "legend", "off"
                        ),
                        rowspan=1,
                        colspan=2,
                    ),
                    Panel(
                        title="Методы MM Apply - время SAS, с (квантиль 99.9%)",
                        url=DashboardBuilder.url_query(
                            "project", project_id,
                            "cluster", "{{cluster}}",
                            "service", "{{service}}",
                            "host", "Sas",
                            "graph", "bulbasaur_processor_percentiles",
                            "step", "apply",
                            "legend", "off"
                        ),
                        rowspan=1,
                        colspan=2,
                    ),
                    Panel(
                        title="Методы MM Apply - время VLA, с (квантиль 99.9%)",
                        url=DashboardBuilder.url_query(
                            "project", project_id,
                            "cluster", "{{cluster}}",
                            "service", "{{service}}",
                            "host", "Vla",
                            "graph", "bulbasaur_processor_percentiles",
                            "step", "apply",
                            "legend", "off"
                        ),
                        rowspan=1,
                        colspan=2,
                    ),
                ],
            ]
        )

    def update_ydb_dashboard(self, project_id):
        self.update(
            project_id=project_id,
            dashboard_id="bulbasaur_ydb_dashboard",
            name="Bulbasaur: YDB stats",
            parameters=self.PARAMETERS,
            rows=[
                [
                    Panel(
                        title="SessionPool",
                        url=DashboardBuilder.url_query(
                            "project", project_id,
                            "cluster", "{{cluster}}",
                            "service", "{{service}}",
                            "host", "{{host}}",
                            "sensor", "db.stats.*_sessions",
                            "stack", "false",
                            "graph", "auto",
                            "legend", "off"
                        ),
                        rowspan=1,
                        colspan=3,
                    ),
                    Panel(
                        title="SessionPoolBalance",
                        url=DashboardBuilder.url_query(
                            "project", project_id,
                            "cluster", "{{cluster}}",
                            "service", "{{service}}",
                            "host", "{{host}}",
                            "sensor", "db.stats.pool_balance",
                            "stack", "false",
                            "graph", "auto",
                            "legend", "off"
                        ),
                        rowspan=1,
                        colspan=3,
                    ),
                ],
                [
                    Panel(
                        title="Методы YDB MM userinfo - RPS",
                        url=DashboardBuilder.url_query(
                            "project", project_id,
                            "cluster", "{{cluster}}",
                            "service", "{{service}}",
                            "host", "{{host}}",
                            "graph", "bulbasaur_ydb_megamind_userinfo_methods_rps",
                            "legend", "off"
                        ),
                        rowspan=1,
                        colspan=2,
                    ),
                    Panel(
                        title="Методы YDB MM userinfo - RPS Ошибок",
                        url=DashboardBuilder.url_query(
                            "project", project_id,
                            "cluster", "{{cluster}}",
                            "service", "{{service}}",
                            "host", "{{host}}",
                            "graph", "bulbasaur_ydb_megamind_userinfo_methods_errors_rps",
                            "legend", "off"
                        ),
                        rowspan=1,
                        colspan=2,
                    ),
                    Panel(
                        title="Методы YDB MM userinfo - время, с (квантиль 99.9%)",
                        url=DashboardBuilder.url_query(
                            "project", project_id,
                            "cluster", "{{cluster}}",
                            "service", "{{service}}",
                            "host", "{{host}}",
                            "graph", "bulbasaur_ydb_megamind_userinfo_methods_percentile",
                            "legend", "off"
                        ),
                        rowspan=1,
                        colspan=2,
                    ),
                ],
                [
                    Panel(
                        title="Методы YDB MM action - RPS",
                        url=DashboardBuilder.url_query(
                            "project", project_id,
                            "cluster", "{{cluster}}",
                            "service", "{{service}}",
                            "host", "{{host}}",
                            "graph", "bulbasaur_ydb_megamind_action_methods_rps",
                            "legend", "off"
                        ),
                        rowspan=1,
                        colspan=2,
                    ),
                    Panel(
                        title="Методы YDB MM action - RPS Ошибок",
                        url=DashboardBuilder.url_query(
                            "project", project_id,
                            "cluster", "{{cluster}}",
                            "service", "{{service}}",
                            "host", "{{host}}",
                            "graph", "bulbasaur_ydb_megamind_action_methods_errors_rps",
                            "legend", "off"
                        ),
                        rowspan=1,
                        colspan=2,
                    ),
                    Panel(
                        title="Методы YDB MM action - время, с (квантиль 99.9%)",
                        url=DashboardBuilder.url_query(
                            "project", project_id,
                            "cluster", "{{cluster}}",
                            "service", "{{service}}",
                            "host", "{{host}}",
                            "graph", "bulbasaur_ydb_megamind_action_methods_percentile",
                            "legend", "off"
                        ),
                        rowspan=1,
                        colspan=2,
                    ),
                ],
                [
                    Panel(
                        title="Методы YDB others - RPS",
                        url=DashboardBuilder.url_query(
                            "project", project_id,
                            "cluster", "{{cluster}}",
                            "service", "{{service}}",
                            "host", "{{host}}",
                            "graph", "bulbasaur_ydb_other_methods_rps",
                            "legend", "off"
                        ),
                        rowspan=1,
                        colspan=2,
                    ),
                    Panel(
                        title="Методы YDB others - RPS Ошибок",
                        url=DashboardBuilder.url_query(
                            "project", project_id,
                            "cluster", "{{cluster}}",
                            "service", "{{service}}",
                            "host", "{{host}}",
                            "graph", "bulbasaur_ydb_other_methods_errors_rps",
                            "legend", "off"
                        ),
                        rowspan=1,
                        colspan=2,
                    ),
                    Panel(
                        title="Методы YDB others - время, с (квантиль 99.9%)",
                        url=DashboardBuilder.url_query(
                            "project", project_id,
                            "cluster", "{{cluster}}",
                            "service", "{{service}}",
                            "host", "{{host}}",
                            "graph", "bulbasaur_ydb_other_methods_percentile",
                            "legend", "off"
                        ),
                        rowspan=1,
                        colspan=2,
                    ),
                ]
            ]
        )

    def update_ydb_handler_dashboard(self, project_id):
        self.update(
            project_id=project_id,
            dashboard_id="bulbasaur_ydb_handler_dashboard",
            name="Bulbasaur: YDB Handler stats",
            parameters=self.YDB_HANDLER_PARAMETERS,
            rows=[
                [
                    Panel(
                        title="RPS",
                        url=DashboardBuilder.url_query(
                            "project", project_id,
                            "cluster", "{{cluster}}",
                            "service", "{{service}}",
                            "host", "{{host}}",
                            "db_method", "{{db_method}}",
                            "sensor", "db.total",
                            "graph", "auto",
                            "legend", "off"
                        ),
                        rowspan=1,
                        colspan=1,
                    ),
                    Panel(
                        title="RPS Ошибок",
                        url=DashboardBuilder.url_query(
                            "project", project_id,
                            "cluster", "{{cluster}}",
                            "service", "{{service}}",
                            "host", "{{host}}",
                            "db_method", "{{db_method}}",
                            "sensor", "db.fails",
                            "error_type", "operation|transport|other",
                            "graph", "auto",
                            "legend", "off"
                        ),
                        rowspan=1,
                        colspan=1,
                    ),
                ],
                [
                    Panel(
                        title="Время, с",
                        url=DashboardBuilder.url_query(
                            "project", project_id,
                            "cluster", "{{cluster}}",
                            "service", "{{service}}",
                            "host", "{{host}}",
                            "db_method", "{{db_method}}",
                            "sensor", "db.duration_buckets",
                            "overLinesTransform", "WEIGHTED_PERCENTILE",
                            "percentiles", "90%2C+95%2C+99%2C+99.9",
                            "graph", "auto",
                            "legend", "off"
                        ),
                        rowspan=1,
                        colspan=2,
                    ),
                ],
            ]
        )

    def update_perf_dashboard(self, project_id):
        self.update(
            project_id=project_id,
            dashboard_id="bulbasaur_perf_dashboard",
            name="Bulbasaur: Perf stats",
            parameters=self.PERF_PARAMETERS,
            rows=[
                [
                    Panel(
                        title="Total sys - Alloc, mb",
                        url=DashboardBuilder.url_query(
                            "project", project_id,
                            "cluster", "{{cluster}}",
                            "service", "{{service}}",
                            "graph", "bulbasaur_perf_total_sys_alloc",
                            "legend", "off"
                        ),
                        rowspan=1,
                        colspan=1,
                    ),
                    Panel(
                        title="Assumed leaks",
                        url=DashboardBuilder.url_query(
                            "project", project_id,
                            "cluster", "{{cluster}}",
                            "service", "{{service}}",
                            "host", "cluster",
                            "sensor", "perf.leaks.*",
                            "graph", "auto",
                            "legend", "off"
                        ),
                        rowspan=1,
                        colspan=1,
                    ),
                    Panel(
                        title="Горутины - Кол-во",
                        url=DashboardBuilder.url_query(
                            "project", project_id,
                            "cluster", "{{cluster}}",
                            "host", "cluster",
                            "service", "{{service}}",
                            "graph", "bulbasaur_perf_goroutines_count",
                            "legend", "off"
                        ),
                        rowspan=1,
                        colspan=1,
                    ),
                    Panel(
                        title="Циклы GC - Кол-во",
                        url=DashboardBuilder.url_query(
                            "project", project_id,
                            "cluster", "{{cluster}}",
                            "service", "{{service}}",
                            "graph", "bulbasaur_perf_gc_count",
                            "legend", "off"
                        ),
                        rowspan=1,
                        colspan=1,
                    ),
                    Panel(
                        title="Циклы GC - Время, мс",
                        url=DashboardBuilder.url_query(
                            "project", project_id,
                            "cluster", "{{cluster}}",
                            "service", "{{service}}",
                            "graph", "bulbasaur_perf_gc_pause",
                            "legend", "off"
                        ),
                        rowspan=1,
                        colspan=1,
                    ),
                ],
                [
                    Panel(
                        title="Heap - In use, mb",
                        url=DashboardBuilder.url_query(
                            "project", project_id,
                            "cluster", "{{cluster}}",
                            "service", "{{service}}",
                            "graph", "bulbasaur_perf_memory_heap_in_use",
                            "legend", "off"
                        ),
                        rowspan=1,
                        colspan=1,
                    ),
                    Panel(
                        title="Heap - Alloc, mb",
                        url=DashboardBuilder.url_query(
                            "project", project_id,
                            "cluster", "{{cluster}}",
                            "service", "{{service}}",
                            "graph", "bulbasaur_perf_memory_heap_alloc",
                            "legend", "off"
                        ),
                        rowspan=1,
                        colspan=1,
                    ),
                    Panel(
                        title="Heap - Released, mb",
                        url=DashboardBuilder.url_query(
                            "project", project_id,
                            "cluster", "{{cluster}}",
                            "service", "{{service}}",
                            "graph", "bulbasaur_perf_memory_heap_released",
                            "legend", "off"
                        ),
                        rowspan=1,
                        colspan=1,
                    ),
                    Panel(
                        title="Heap objects - Кол-во",
                        url=DashboardBuilder.url_query(
                            "project", project_id,
                            "cluster", "{{cluster}}",
                            "service", "{{service}}",
                            "graph", "bulbasaur_perf_memory_heap_objects",
                            "legend", "off"
                        ),
                        rowspan=1,
                        colspan=1,
                    ),
                    Panel(
                        title="Stack - In use, mb",
                        url=DashboardBuilder.url_query(
                            "project", project_id,
                            "cluster", "{{cluster}}",
                            "service", "{{service}}",
                            "graph", "bulbasaur_perf_memory_stack_in_use",
                            "legend", "off"
                        ),
                        rowspan=1,
                        colspan=1,
                    ),
                ],
            ]
        )

    def update_neighbours_dashboard(self, project_id):
        # self.update_dialogs_dashboards(project_id)
        # self.update_bass_dashboards(project_id)
        # self.update_begemot_dashboards(project_id)
        # self.update_datasync_dashboards(project_id)
        # self.update_geosuggest_dashboards(project_id)
        # self.update_blackbox_dashboards(project_id)
        # self.update_socialism_dashboards(project_id)
        # self.update_tvm_dashboards(project_id)
        # self.update_steelix_dashboards(project_id)
        # self.update_sup_dashboards(project_id)
        # self.update_tuya_dashboards(project_id)
        # self.update_timemachine_dashboards(project_id)
        # self.update_memento_dashboards(project_id)
        # self.update_solomon_api_dashboards(project_id)
        self.update_xiva_dashboards(project_id)
        self.update_notificator_dashboards(project_id)
        self.update_oauth_dashboards(project_id)
        self.update_unified_agent_dashboards(project_id)

    def update_dialogs_dashboards(self, project_id):
        self.update(
            project_id=project_id,
            dashboard_id="bulbasaur_dialogs_dashboard",
            name="Dialogs: Stats",
            parameters=self.PARAMETERS,
            rows=[
                [
                    Panel(
                        title="Методы API - RPS",
                        url=DashboardBuilder.url_query(
                            "project", project_id,
                            "cluster", "{{cluster}}",
                            "service", "{{service}}",
                            "host", "{{host}}",
                            "graph", "dialogs_api_calls_rps",
                            "legend", "off"
                        ),
                        rowspan=1,
                        colspan=2,
                    ),
                    Panel(
                        title="Методы API - Ошибки, %",
                        url=DashboardBuilder.url_query(
                            "project", project_id,
                            "cluster", "{{cluster}}",
                            "service", "{{service}}",
                            "host", "{{host}}",
                            "graph", "dialogs_api_errors",
                            "legend", "off"
                        ),
                        rowspan=1,
                        colspan=2,
                    ),
                    Panel(
                        title="Методы API - Тайминги, с (квантиль 99.9%)",
                        url=DashboardBuilder.url_query(
                            "project", project_id,
                            "cluster", "{{cluster}}",
                            "service", "{{service}}",
                            "host", "{{host}}",
                            "graph", "dialogs_api_call_percentiles",
                            "legend", "off"
                        ),
                        rowspan=1,
                        colspan=2,
                    ),
                ],
                [
                    Panel(
                        title="Методы API - getSkill, RPS",
                        url=DashboardBuilder.url_query(
                            "project", project_id,
                            "cluster", "{{cluster}}",
                            "service", "{{service}}",
                            "host", "{{host}}",
                            "graph", "dialogs_api_call_getSkill_rps",
                            "legend", "off"
                        ),
                        rowspan=1,
                        colspan=3,
                    ),
                    Panel(
                        title="Методы API - getSkill, %",
                        url=DashboardBuilder.url_query(
                            "project", project_id,
                            "cluster", "{{cluster}}",
                            "service", "{{service}}",
                            "host", "{{host}}",
                            "graph", "dialogs_api_call_getSkill_percent",
                            "legend", "off"
                        ),
                        rowspan=1,
                        colspan=3,
                    ),
                ],
                [
                    Panel(
                        title="Методы API - getSmartHomeSkills, RPS",
                        url=DashboardBuilder.url_query(
                            "project", project_id,
                            "cluster", "{{cluster}}",
                            "service", "{{service}}",
                            "host", "{{host}}",
                            "graph", "dialogs_api_call_getSmartHomeSkills_rps",
                            "legend", "off"
                        ),
                        rowspan=1,
                        colspan=3,
                    ),
                    Panel(
                        title="Методы API - getSmartHomeSkills, %",
                        url=DashboardBuilder.url_query(
                            "project", project_id,
                            "cluster", "{{cluster}}",
                            "service", "{{service}}",
                            "host", "{{host}}",
                            "graph", "dialogs_api_call_getSmartHomeSkills_percent",
                            "legend", "off"
                        ),
                        rowspan=1,
                        colspan=3,
                    ),
                ],
                [
                    Panel(
                        title="Методы API - getSkillCertifiedDevices, RPS",
                        url=DashboardBuilder.url_query(
                            "project", project_id,
                            "cluster", "{{cluster}}",
                            "service", "{{service}}",
                            "host", "{{host}}",
                            "graph", "dialogs_api_call_getSkillCertifiedDevices_rps",
                            "legend", "off"
                        ),
                        rowspan=1,
                        colspan=3,
                    ),
                    Panel(
                        title="Методы API - getSkillCertifiedDevices, %",
                        url=DashboardBuilder.url_query(
                            "project", project_id,
                            "cluster", "{{cluster}}",
                            "service", "{{service}}",
                            "host", "{{host}}",
                            "graph", "dialogs_api_call_getSkillCertifiedDevices_percent",
                            "legend", "off"
                        ),
                        rowspan=1,
                        colspan=3,
                    ),
                ],

            ]
        )

    def update_bass_dashboards(self, project_id):
        self.update(
            project_id=project_id,
            dashboard_id="neighbour_bass_dashboard",
            name="Bass: Stats",
            parameters=self.PARAMETERS,
            rows=[
                [
                    Panel(
                        title="Методы API - RPS",
                        url=DashboardBuilder.url_query(
                            "project", project_id,
                            "cluster", "{{cluster}}",
                            "service", "{{service}}",
                            "host", "{{host}}",
                            "graph", "bass_api_calls_rps",
                            "legend", "off"
                        ),
                        rowspan=1,
                        colspan=2,
                    ),
                    Panel(
                        title="Методы API - Ошибки, %",
                        url=DashboardBuilder.url_query(
                            "project", project_id,
                            "cluster", "{{cluster}}",
                            "service", "{{service}}",
                            "host", "{{host}}",
                            "graph", "bass_api_errors",
                            "legend", "off"
                        ),
                        rowspan=1,
                        colspan=2,
                    ),
                    Panel(
                        title="Методы API - Тайминги, с (квантиль 99.9%)",
                        url=DashboardBuilder.url_query(
                            "project", project_id,
                            "cluster", "{{cluster}}",
                            "service", "{{service}}",
                            "host", "{{host}}",
                            "graph", "bass_api_call_percentiles",
                            "legend", "off"
                        ),
                        rowspan=1,
                        colspan=2,
                    ),
                ],
                [
                    Panel(
                        title="Методы API - sendPush, RPS",
                        url=DashboardBuilder.url_query(
                            "project", project_id,
                            "cluster", "{{cluster}}",
                            "service", "{{service}}",
                            "host", "{{host}}",
                            "graph", "bass_api_call_sendPush_rps",
                            "legend", "off"
                        ),
                        rowspan=1,
                        colspan=3,
                    ),
                    Panel(
                        title="Методы API - sendPush, %",
                        url=DashboardBuilder.url_query(
                            "project", project_id,
                            "cluster", "{{cluster}}",
                            "service", "{{service}}",
                            "host", "{{host}}",
                            "graph", "bass_api_call_sendPush_percent",
                            "legend", "off"
                        ),
                        rowspan=1,
                        colspan=3,
                    ),
                ],
            ]
        )

    def update_begemot_dashboards(self, project_id):
        self.update(
            project_id=project_id,
            dashboard_id="neighbour_begemot_dashboard",
            name="Begemot: Stats",
            parameters=self.PARAMETERS,
            rows=[
                [
                    Panel(
                        title="Методы API - RPS",
                        url=DashboardBuilder.url_query(
                            "project", project_id,
                            "cluster", "{{cluster}}",
                            "service", "{{service}}",
                            "host", "{{host}}",
                            "graph", "begemot_api_calls_rps",
                            "legend", "off"
                        ),
                        rowspan=1,
                        colspan=2,
                    ),
                    Panel(
                        title="Методы API - Ошибки, %",
                        url=DashboardBuilder.url_query(
                            "project", project_id,
                            "cluster", "{{cluster}}",
                            "service", "{{service}}",
                            "host", "{{host}}",
                            "graph", "begemot_api_errors",
                            "legend", "off"
                        ),
                        rowspan=1,
                        colspan=2,
                    ),
                    Panel(
                        title="Методы API - Тайминги, с (квантиль 99.9%)",
                        url=DashboardBuilder.url_query(
                            "project", project_id,
                            "cluster", "{{cluster}}",
                            "service", "{{service}}",
                            "host", "{{host}}",
                            "graph", "begemot_api_call_percentiles",
                            "legend", "off"
                        ),
                        rowspan=1,
                        colspan=2,
                    ),
                ],
                [
                    Panel(
                        title="Методы API - wizard, RPS",
                        url=DashboardBuilder.url_query(
                            "project", project_id,
                            "cluster", "{{cluster}}",
                            "service", "{{service}}",
                            "host", "{{host}}",
                            "graph", "begemot_api_call_wizard_rps",
                            "legend", "off"
                        ),
                        rowspan=1,
                        colspan=3,
                    ),
                    Panel(
                        title="Методы API - wizard, %",
                        url=DashboardBuilder.url_query(
                            "project", project_id,
                            "cluster", "{{cluster}}",
                            "service", "{{service}}",
                            "host", "{{host}}",
                            "graph", "begemot_api_call_wizard_percent",
                            "legend", "off"
                        ),
                        rowspan=1,
                        colspan=3,
                    ),
                ],
            ]
        )

    def update_datasync_dashboards(self, project_id):
        self.update(
            project_id=project_id,
            dashboard_id="neighbour_datasync_dashboard",
            name="Datasync: Stats",
            parameters=self.PARAMETERS,
            rows=[
                [
                    Panel(
                        title="Методы API - RPS",
                        url=DashboardBuilder.url_query(
                            "project", project_id,
                            "cluster", "{{cluster}}",
                            "service", "{{service}}",
                            "host", "{{host}}",
                            "graph", "datasync_api_calls_rps",
                            "legend", "off"
                        ),
                        rowspan=1,
                        colspan=2,
                    ),
                    Panel(
                        title="Методы API - Ошибки, %",
                        url=DashboardBuilder.url_query(
                            "project", project_id,
                            "cluster", "{{cluster}}",
                            "service", "{{service}}",
                            "host", "{{host}}",
                            "graph", "datasync_api_errors",
                            "legend", "off"
                        ),
                        rowspan=1,
                        colspan=2,
                    ),
                    Panel(
                        title="Методы API - Тайминги, с (квантиль 99.9%)",
                        url=DashboardBuilder.url_query(
                            "project", project_id,
                            "cluster", "{{cluster}}",
                            "service", "{{service}}",
                            "host", "{{host}}",
                            "graph", "datasync_api_call_percentiles",
                            "legend", "off"
                        ),
                        rowspan=1,
                        colspan=2,
                    ),
                ],
                [
                    Panel(
                        title="Методы API - getAddressesForUser, RPS",
                        url=DashboardBuilder.url_query(
                            "project", project_id,
                            "cluster", "{{cluster}}",
                            "service", "{{service}}",
                            "host", "{{host}}",
                            "graph", "datasync_api_call_getAddressesForUser_rps",
                            "legend", "off"
                        ),
                        rowspan=1,
                        colspan=3,
                    ),
                    Panel(
                        title="Методы API - getAddressesForUser, %",
                        url=DashboardBuilder.url_query(
                            "project", project_id,
                            "cluster", "{{cluster}}",
                            "service", "{{service}}",
                            "host", "{{host}}",
                            "graph", "datasync_api_call_getAddressesForUser_percent",
                            "legend", "off"
                        ),
                        rowspan=1,
                        colspan=3,
                    ),
                ],
            ]
        )

    def update_geosuggest_dashboards(self, project_id):
        self.update(
            project_id=project_id,
            dashboard_id="neighbour_geosuggest_dashboard",
            name="Geosuggest: Stats",
            parameters=self.PARAMETERS,
            rows=[
                [
                    Panel(
                        title="Методы API - RPS",
                        url=DashboardBuilder.url_query(
                            "project", project_id,
                            "cluster", "{{cluster}}",
                            "service", "{{service}}",
                            "host", "{{host}}",
                            "graph", "geosuggest_api_calls_rps",
                            "legend", "off"
                        ),
                        rowspan=1,
                        colspan=2,
                    ),
                    Panel(
                        title="Методы API - Ошибки, %",
                        url=DashboardBuilder.url_query(
                            "project", project_id,
                            "cluster", "{{cluster}}",
                            "service", "{{service}}",
                            "host", "{{host}}",
                            "graph", "geosuggest_api_errors",
                            "legend", "off"
                        ),
                        rowspan=1,
                        colspan=2,
                    ),
                    Panel(
                        title="Методы API - Тайминги, с (квантиль 99.9%)",
                        url=DashboardBuilder.url_query(
                            "project", project_id,
                            "cluster", "{{cluster}}",
                            "service", "{{service}}",
                            "host", "{{host}}",
                            "graph", "geosuggest_api_call_percentiles",
                            "legend", "off"
                        ),
                        rowspan=1,
                        colspan=2,
                    ),
                ],
                [
                    Panel(
                        title="Методы API - getGeosuggestFromAddress, RPS",
                        url=DashboardBuilder.url_query(
                            "project", project_id,
                            "cluster", "{{cluster}}",
                            "service", "{{service}}",
                            "host", "{{host}}",
                            "graph", "geosuggest_api_call_getGeosuggestFromAddress_rps",
                            "legend", "off"
                        ),
                        rowspan=1,
                        colspan=3,
                    ),
                    Panel(
                        title="Методы API - getGeosuggestFromAddress, %",
                        url=DashboardBuilder.url_query(
                            "project", project_id,
                            "cluster", "{{cluster}}",
                            "service", "{{service}}",
                            "host", "{{host}}",
                            "graph", "geosuggest_api_call_getGeosuggestFromAddress_percent",
                            "legend", "off"
                        ),
                        rowspan=1,
                        colspan=3,
                    ),
                ],
            ]
        )

    def update_blackbox_dashboards(self, project_id):
        self.update(
            project_id=project_id,
            dashboard_id="bulbasaur_blackbox_dashboard",
            name="Blackbox: Stats",
            parameters=self.PARAMETERS,
            rows=[
                [
                    Panel(
                        title="Методы API - RPS",
                        url=DashboardBuilder.url_query(
                            "project", project_id,
                            "cluster", "{{cluster}}",
                            "service", "{{service}}",
                            "host", "{{host}}",
                            "graph", "blackbox_api_calls_rps",
                            "legend", "off"
                        ),
                        rowspan=1,
                        colspan=2,
                    ),
                    Panel(
                        title="Методы API - Ошибки, %",
                        url=DashboardBuilder.url_query(
                            "project", project_id,
                            "cluster", "{{cluster}}",
                            "service", "{{service}}",
                            "host", "{{host}}",
                            "graph", "blackbox_api_errors",
                            "legend", "off"
                        ),
                        rowspan=1,
                        colspan=2,
                    ),
                    Panel(
                        title="Методы API - Тайминги, с (квантиль 99.9%)",
                        url=DashboardBuilder.url_query(
                            "project", project_id,
                            "cluster", "{{cluster}}",
                            "service", "{{service}}",
                            "host", "{{host}}",
                            "graph", "blackbox_api_call_percentiles",
                            "legend", "off"
                        ),
                        rowspan=1,
                        colspan=2,
                    ),
                ],
                [
                    Panel(
                        title="Методы API - sessionID, RPS",
                        url=DashboardBuilder.url_query(
                            "project", project_id,
                            "cluster", "{{cluster}}",
                            "service", "{{service}}",
                            "host", "{{host}}",
                            "graph", "blackbox_api_call_sessionID_rps",
                            "legend", "off"
                        ),
                        rowspan=1,
                        colspan=3,
                    ),
                    Panel(
                        title="Методы API - sessionID, %",
                        url=DashboardBuilder.url_query(
                            "project", project_id,
                            "cluster", "{{cluster}}",
                            "service", "{{service}}",
                            "host", "{{host}}",
                            "graph", "blackbox_api_call_sessionID_percent",
                            "legend", "off"
                        ),
                        rowspan=1,
                        colspan=3,
                    ),
                ],
                [
                    Panel(
                        title="Методы API - oauth, RPS",
                        url=DashboardBuilder.url_query(
                            "project", project_id,
                            "cluster", "{{cluster}}",
                            "service", "{{service}}",
                            "host", "{{host}}",
                            "graph", "blackbox_api_call_oauth_rps",
                            "legend", "off"
                        ),
                        rowspan=1,
                        colspan=3,
                    ),
                    Panel(
                        title="Методы API - oauth, %",
                        url=DashboardBuilder.url_query(
                            "project", project_id,
                            "cluster", "{{cluster}}",
                            "service", "{{service}}",
                            "host", "{{host}}",
                            "graph", "blackbox_api_call_oauth_percent",
                            "legend", "off"
                        ),
                        rowspan=1,
                        colspan=3,
                    ),
                ],
            ]
        )

    def update_xiva_dashboards(self, project_id):
        self.update(
            project_id=project_id,
            dashboard_id="bulbasaur_xiva_dashboard",
            name="Xiva: Stats",
            parameters=self.PARAMETERS,
            rows=[
                [
                    Panel(
                        title="Методы API - RPS",
                        url=DashboardBuilder.url_query(
                            "project", project_id,
                            "cluster", "{{cluster}}",
                            "service", "{{service}}",
                            "host", "{{host}}",
                            "graph", "xiva_api_calls_rps",
                            "legend", "off"
                        ),
                        rowspan=1,
                        colspan=2,
                    ),
                    Panel(
                        title="Методы API - Ошибки, %",
                        url=DashboardBuilder.url_query(
                            "project", project_id,
                            "cluster", "{{cluster}}",
                            "service", "{{service}}",
                            "host", "{{host}}",
                            "graph", "xiva_api_errors",
                            "legend", "off"
                        ),
                        rowspan=1,
                        colspan=2,
                    ),
                    Panel(
                        title="Методы API - Тайминги, с (квантиль 99.9%)",
                        url=DashboardBuilder.url_query(
                            "project", project_id,
                            "cluster", "{{cluster}}",
                            "service", "{{service}}",
                            "host", "{{host}}",
                            "graph", "xiva_api_call_percentiles",
                            "legend", "off"
                        ),
                        rowspan=1,
                        colspan=2,
                    ),
                ],
                [
                    Panel(
                        title="Методы API - getWebsocketURL, RPS",
                        url=DashboardBuilder.url_query(
                            "project", project_id,
                            "cluster", "{{cluster}}",
                            "service", "{{service}}",
                            "host", "{{host}}",
                            "graph", "xiva_api_call_getWebsocketURL_rps",
                            "legend", "off"
                        ),
                        rowspan=1,
                        colspan=3,
                    ),
                    Panel(
                        title="Методы API - getWebsocketURL, %",
                        url=DashboardBuilder.url_query(
                            "project", project_id,
                            "cluster", "{{cluster}}",
                            "service", "{{service}}",
                            "host", "{{host}}",
                            "graph", "xiva_api_call_getWebsocketURL_percent",
                            "legend", "off"
                        ),
                        rowspan=1,
                        colspan=3,
                    ),
                ],
                [
                    Panel(
                        title="Методы API - sendPush, RPS",
                        url=DashboardBuilder.url_query(
                            "project", project_id,
                            "cluster", "{{cluster}}",
                            "service", "{{service}}",
                            "host", "{{host}}",
                            "graph", "xiva_api_call_sendPush_rps",
                            "legend", "off"
                        ),
                        rowspan=1,
                        colspan=3,
                    ),
                    Panel(
                        title="Методы API - sendPush, %",
                        url=DashboardBuilder.url_query(
                            "project", project_id,
                            "cluster", "{{cluster}}",
                            "service", "{{service}}",
                            "host", "{{host}}",
                            "graph", "xiva_api_call_sendPush_percent",
                            "legend", "off"
                        ),
                        rowspan=1,
                        colspan=3,
                    ),
                ],
                [
                    Panel(
                        title="Методы API - listSubscriptions, RPS",
                        url=DashboardBuilder.url_query(
                            "project", project_id,
                            "cluster", "{{cluster}}",
                            "service", "{{service}}",
                            "host", "{{host}}",
                            "graph", "xiva_api_call_listSubscriptions_rps",
                            "legend", "off"
                        ),
                        rowspan=1,
                        colspan=3,
                    ),
                    Panel(
                        title="Методы API - listSubscriptions, %",
                        url=DashboardBuilder.url_query(
                            "project", project_id,
                            "cluster", "{{cluster}}",
                            "service", "{{service}}",
                            "host", "{{host}}",
                            "graph", "xiva_api_call_listSubscriptions_percent",
                            "legend", "off"
                        ),
                        rowspan=1,
                        colspan=3,
                    ),
                ],
                [
                    Panel(
                        title="Методы API - getSubscriptionSign, RPS",
                        url=DashboardBuilder.url_query(
                            "project", project_id,
                            "cluster", "{{cluster}}",
                            "service", "{{service}}",
                            "host", "{{host}}",
                            "graph", "xiva_api_call_getSubscriptionSign_rps",
                            "legend", "off"
                        ),
                        rowspan=1,
                        colspan=3,
                    ),
                    Panel(
                        title="Методы API - getSubscriptionSign, %",
                        url=DashboardBuilder.url_query(
                            "project", project_id,
                            "cluster", "{{cluster}}",
                            "service", "{{service}}",
                            "host", "{{host}}",
                            "graph", "xiva_api_call_getSubscriptionSign_percent",
                            "legend", "off"
                        ),
                        rowspan=1,
                        colspan=3,
                    ),
                ]
            ],
        )

    def update_solomon_api_dashboards(self, project_id):
        self.update(
            project_id=project_id,
            dashboard_id="bulbasaur_solomon_api_dashboard",
            name="Solomon API: Stats",
            parameters=self.PARAMETERS,
            rows=[
                [
                    Panel(
                        title="Методы API - RPS",
                        url=DashboardBuilder.url_query(
                            "project", project_id,
                            "cluster", "{{cluster}}",
                            "service", "{{service}}",
                            "host", "{{host}}",
                            "graph", "solomon_api_api_calls_rps",
                            "legend", "off"
                        ),
                        rowspan=1,
                        colspan=2,
                    ),
                    Panel(
                        title="Методы API - Ошибки, %",
                        url=DashboardBuilder.url_query(
                            "project", project_id,
                            "cluster", "{{cluster}}",
                            "service", "{{service}}",
                            "host", "{{host}}",
                            "graph", "solomon_api_api_errors",
                            "legend", "off"
                        ),
                        rowspan=1,
                        colspan=2,
                    ),
                    Panel(
                        title="Методы API - Тайминги, с (квантиль 99.9%)",
                        url=DashboardBuilder.url_query(
                            "project", project_id,
                            "cluster", "{{cluster}}",
                            "service", "{{service}}",
                            "host", "{{host}}",
                            "graph", "solomon_api_api_call_percentiles",
                            "legend", "off"
                        ),
                        rowspan=1,
                        colspan=2,
                    ),
                ],
                [
                    Panel(
                        title="Методы API - fetchData, RPS",
                        url=DashboardBuilder.url_query(
                            "project", project_id,
                            "cluster", "{{cluster}}",
                            "service", "{{service}}",
                            "host", "{{host}}",
                            "graph", "solomon_api_api_call_fetchData_rps",
                            "legend", "off"
                        ),
                        rowspan=1,
                        colspan=3,
                    ),
                    Panel(
                        title="Методы API - fetchData, %",
                        url=DashboardBuilder.url_query(
                            "project", project_id,
                            "cluster", "{{cluster}}",
                            "service", "{{service}}",
                            "host", "{{host}}",
                            "graph", "solomon_api_api_call_fetchData_percent",
                            "legend", "off"
                        ),
                        rowspan=1,
                        colspan=3,
                    ),
                ],
                [
                    Panel(
                        title="Методы API - sendData, RPS",
                        url=DashboardBuilder.url_query(
                            "project", project_id,
                            "cluster", "{{cluster}}",
                            "service", "{{service}}",
                            "host", "{{host}}",
                            "graph", "solomon_api_api_call_sendData_rps",
                            "legend", "off"
                        ),
                        rowspan=1,
                        colspan=3,
                    ),
                    Panel(
                        title="Методы API - sendData, %",
                        url=DashboardBuilder.url_query(
                            "project", project_id,
                            "cluster", "{{cluster}}",
                            "service", "{{service}}",
                            "host", "{{host}}",
                            "graph", "solomon_api_api_call_sendData_percent",
                            "legend", "off"
                        ),
                        rowspan=1,
                        colspan=3,
                    ),
                ],
            ]
        )

    def update_unified_agent_dashboards(self, project_id):
        self.update(
            project_id=project_id,
            dashboard_id="bulbasaur_unified_agent_dashboard",
            name="Unified Agent: Stats",
            parameters=self.PARAMETERS,
            rows=[
                [
                    Panel(
                        title="Методы API - RPS",
                        url=DashboardBuilder.url_query(
                            "project", project_id,
                            "cluster", "{{cluster}}",
                            "service", "{{service}}",
                            "host", "{{host}}",
                            "graph", "unified_agent_api_calls_rps",
                            "legend", "off"
                        ),
                        rowspan=1,
                        colspan=2,
                    ),
                    Panel(
                        title="Методы API - Ошибки, %",
                        url=DashboardBuilder.url_query(
                            "project", project_id,
                            "cluster", "{{cluster}}",
                            "service", "{{service}}",
                            "host", "{{host}}",
                            "graph", "unified_agent_api_errors",
                            "legend", "off"
                        ),
                        rowspan=1,
                        colspan=2,
                    ),
                    Panel(
                        title="Методы API - Тайминги, с (квантиль 99.9%)",
                        url=DashboardBuilder.url_query(
                            "project", project_id,
                            "cluster", "{{cluster}}",
                            "service", "{{service}}",
                            "host", "{{host}}",
                            "graph", "unified_agent_api_call_percentiles",
                            "legend", "off"
                        ),
                        rowspan=1,
                        colspan=2,
                    ),
                ],
                [
                    Panel(
                        title="Методы API - sendData, RPS",
                        url=DashboardBuilder.url_query(
                            "project", project_id,
                            "cluster", "{{cluster}}",
                            "service", "{{service}}",
                            "host", "{{host}}",
                            "graph", "unified_agent_api_call_sendData_rps",
                            "legend", "off"
                        ),
                        rowspan=1,
                        colspan=3,
                    ),
                    Panel(
                        title="Методы API - sendData, %",
                        url=DashboardBuilder.url_query(
                            "project", project_id,
                            "cluster", "{{cluster}}",
                            "service", "{{service}}",
                            "host", "{{host}}",
                            "graph", "unified_agent_api_call_sendData_percent",
                            "legend", "off"
                        ),
                        rowspan=1,
                        colspan=3,
                    ),
                ],
            ]
        )

    def update_notificator_dashboards(self, project_id):
        self.update(
            project_id=project_id,
            dashboard_id="bulbasaur_notificator_dashboard",
            name="Notificator: Stats",
            parameters=self.PARAMETERS,
            rows=[
                [
                    Panel(
                        title="Методы API - RPS",
                        url=DashboardBuilder.url_query(
                            "project", project_id,
                            "cluster", "{{cluster}}",
                            "service", "{{service}}",
                            "host", "{{host}}",
                            "graph", "notificator_api_calls_rps",
                            "legend", "off"
                        ),
                        rowspan=1,
                        colspan=2,
                    ),
                    Panel(
                        title="Методы API - Ошибки, %",
                        url=DashboardBuilder.url_query(
                            "project", project_id,
                            "cluster", "{{cluster}}",
                            "service", "{{service}}",
                            "host", "{{host}}",
                            "graph", "notificator_api_errors",
                            "legend", "off"
                        ),
                        rowspan=1,
                        colspan=2,
                    ),
                    Panel(
                        title="Методы API - Тайминги, с (квантиль 99.9%)",
                        url=DashboardBuilder.url_query(
                            "project", project_id,
                            "cluster", "{{cluster}}",
                            "service", "{{service}}",
                            "host", "{{host}}",
                            "graph", "notificator_api_call_percentiles",
                            "legend", "off"
                        ),
                        rowspan=1,
                        colspan=2,
                    ),
                ],
                [
                    Panel(
                        title="Методы API - sendTypedSemanticFramePush, RPS",
                        url=DashboardBuilder.url_query(
                            "project", project_id,
                            "cluster", "{{cluster}}",
                            "service", "{{service}}",
                            "host", "{{host}}",
                            "graph", "notificator_api_call_sendTypedSemanticFramePush_rps",
                            "legend", "off"
                        ),
                        rowspan=1,
                        colspan=3,
                    ),
                    Panel(
                        title="Методы API - sendTypedSemanticFramePush, %",
                        url=DashboardBuilder.url_query(
                            "project", project_id,
                            "cluster", "{{cluster}}",
                            "service", "{{service}}",
                            "host", "{{host}}",
                            "graph", "notificator_api_call_sendTypedSemanticFramePush_percent",
                            "legend", "off"
                        ),
                        rowspan=1,
                        colspan=3,
                    ),
                ],
                [
                    Panel(
                        title="Методы API - getDevices, RPS",
                        url=DashboardBuilder.url_query(
                            "project", project_id,
                            "cluster", "{{cluster}}",
                            "service", "{{service}}",
                            "host", "{{host}}",
                            "graph", "notificator_api_call_getDevices_rps",
                            "legend", "off"
                        ),
                        rowspan=1,
                        colspan=3,
                    ),
                    Panel(
                        title="Методы API - getDevices, %",
                        url=DashboardBuilder.url_query(
                            "project", project_id,
                            "cluster", "{{cluster}}",
                            "service", "{{service}}",
                            "host", "{{host}}",
                            "graph", "notificator_api_call_getDevices_percent",
                            "legend", "off"
                        ),
                        rowspan=1,
                        colspan=3,
                    ),
                ],
            ]
        )

    def update_oauth_dashboards(self, project_id):
        self.update(
            project_id=project_id,
            dashboard_id="bulbasaur_oauth_dashboard",
            name="OAuth: Stats",
            parameters=self.PARAMETERS,
            rows=[
                [
                    Panel(
                        title="Методы API - RPS",
                        url=DashboardBuilder.url_query(
                            "project", project_id,
                            "cluster", "{{cluster}}",
                            "service", "{{service}}",
                            "host", "{{host}}",
                            "graph", "oauth_api_calls_rps",
                            "legend", "off"
                        ),
                        rowspan=1,
                        colspan=2,
                    ),
                    Panel(
                        title="Методы API - Ошибки, %",
                        url=DashboardBuilder.url_query(
                            "project", project_id,
                            "cluster", "{{cluster}}",
                            "service", "{{service}}",
                            "host", "{{host}}",
                            "graph", "oauth_api_errors",
                            "legend", "off"
                        ),
                        rowspan=1,
                        colspan=2,
                    ),
                    Panel(
                        title="Методы API - Тайминги, с (квантиль 99.9%)",
                        url=DashboardBuilder.url_query(
                            "project", project_id,
                            "cluster", "{{cluster}}",
                            "service", "{{service}}",
                            "host", "{{host}}",
                            "graph", "oauth_api_call_percentiles",
                            "legend", "off"
                        ),
                        rowspan=1,
                        colspan=2,
                    ),
                ],
                [
                    Panel(
                        title="Методы API - issueAuthorizationCode, RPS",
                        url=DashboardBuilder.url_query(
                            "project", project_id,
                            "cluster", "{{cluster}}",
                            "service", "{{service}}",
                            "host", "{{host}}",
                            "graph", "oauth_api_call_issueAuthorizationCode_rps",
                            "legend", "off"
                        ),
                        rowspan=1,
                        colspan=3,
                    ),
                    Panel(
                        title="Методы API - issueAuthorizationCode, %",
                        url=DashboardBuilder.url_query(
                            "project", project_id,
                            "cluster", "{{cluster}}",
                            "service", "{{service}}",
                            "host", "{{host}}",
                            "graph", "oauth_api_call_issueAuthorizationCode_percent",
                            "legend", "off"
                        ),
                        rowspan=1,
                        colspan=3,
                    ),
                ],
            ]
        )

    def update_tuya_dashboards(self, project_id):
        self.update(
            project_id=project_id,
            dashboard_id="bulbasaur_tuya_dashboard",
            name="Tuya: Stats",
            parameters=self.PARAMETERS,
            rows=[
                [
                    Panel(
                        title="Методы API - RPS",
                        url=DashboardBuilder.url_query(
                            "project", project_id,
                            "cluster", "{{cluster}}",
                            "service", "{{service}}",
                            "host", "{{host}}",
                            "graph", "tuya_api_calls_rps",
                            "legend", "off"
                        ),
                        rowspan=1,
                        colspan=2,
                    ),
                    Panel(
                        title="Методы API - Ошибки, %",
                        url=DashboardBuilder.url_query(
                            "project", project_id,
                            "cluster", "{{cluster}}",
                            "service", "{{service}}",
                            "host", "{{host}}",
                            "graph", "tuya_api_errors",
                            "legend", "off"
                        ),
                        rowspan=1,
                        colspan=2,
                    ),
                    Panel(
                        title="Методы API - Тайминги, с (квантиль 99.9%)",
                        url=DashboardBuilder.url_query(
                            "project", project_id,
                            "cluster", "{{cluster}}",
                            "service", "{{service}}",
                            "host", "{{host}}",
                            "graph", "tuya_api_call_percentiles",
                            "legend", "off"
                        ),
                        rowspan=1,
                        colspan=2,
                    ),
                ],
                [
                    Panel(
                        title="Методы API - getDevicesDiscoveryInfo, RPS",
                        url=DashboardBuilder.url_query(
                            "project", project_id,
                            "cluster", "{{cluster}}",
                            "service", "{{service}}",
                            "host", "{{host}}",
                            "graph", "tuya_api_call_getDevicesDiscoveryInfo_rps",
                            "legend", "off"
                        ),
                        rowspan=1,
                        colspan=3,
                    ),
                    Panel(
                        title="Методы API - getDevicesDiscoveryInfo, %",
                        url=DashboardBuilder.url_query(
                            "project", project_id,
                            "cluster", "{{cluster}}",
                            "service", "{{service}}",
                            "host", "{{host}}",
                            "graph", "tuya_api_call_getDevicesDiscoveryInfo_percent",
                            "legend", "off"
                        ),
                        rowspan=1,
                        colspan=3,
                    ),
                ],
                [
                    Panel(
                        title="Методы API - getDevicesUnderPairingToken, RPS",
                        url=DashboardBuilder.url_query(
                            "project", project_id,
                            "cluster", "{{cluster}}",
                            "service", "{{service}}",
                            "host", "{{host}}",
                            "graph", "tuya_api_call_getDevicesUnderPairingToken_rps",
                            "legend", "off"
                        ),
                        rowspan=1,
                        colspan=3,
                    ),
                    Panel(
                        title="Методы API - getDevicesUnderPairingToken, %",
                        url=DashboardBuilder.url_query(
                            "project", project_id,
                            "cluster", "{{cluster}}",
                            "service", "{{service}}",
                            "host", "{{host}}",
                            "graph", "tuya_api_call_getDevicesUnderPairingToken_percent",
                            "legend", "off"
                        ),
                        rowspan=1,
                        colspan=3,
                    ),
                ],
            ]
        )

    def update_sup_dashboards(self, project_id):
        self.update(
            project_id=project_id,
            dashboard_id="bulbasaur_sup_dashboard",
            name="Sup: Stats",
            parameters=self.PARAMETERS,
            rows=[
                [
                    Panel(
                        title="Методы API - RPS",
                        url=DashboardBuilder.url_query(
                            "project", project_id,
                            "cluster", "{{cluster}}",
                            "service", "{{service}}",
                            "host", "{{host}}",
                            "graph", "sup_api_calls_rps",
                            "legend", "off"
                        ),
                        rowspan=1,
                        colspan=2,
                    ),
                    Panel(
                        title="Методы API - Ошибки, %",
                        url=DashboardBuilder.url_query(
                            "project", project_id,
                            "cluster", "{{cluster}}",
                            "service", "{{service}}",
                            "host", "{{host}}",
                            "graph", "sup_api_errors",
                            "legend", "off"
                        ),
                        rowspan=1,
                        colspan=2,
                    ),
                    Panel(
                        title="Методы API - Тайминги, с (квантиль 99.9%)",
                        url=DashboardBuilder.url_query(
                            "project", project_id,
                            "cluster", "{{cluster}}",
                            "service", "{{service}}",
                            "host", "{{host}}",
                            "graph", "sup_api_call_percentiles",
                            "legend", "off"
                        ),
                        rowspan=1,
                        colspan=2,
                    ),
                ],
                [
                    Panel(
                        title="Методы API - sendPush, RPS",
                        url=DashboardBuilder.url_query(
                            "project", project_id,
                            "cluster", "{{cluster}}",
                            "service", "{{service}}",
                            "host", "{{host}}",
                            "graph", "sup_api_call_sendPush_rps",
                            "legend", "off"
                        ),
                        rowspan=1,
                        colspan=3,
                    ),
                    Panel(
                        title="Методы API - sendPush, %",
                        url=DashboardBuilder.url_query(
                            "project", project_id,
                            "cluster", "{{cluster}}",
                            "service", "{{service}}",
                            "host", "{{host}}",
                            "graph", "sup_api_call_sendPush_percent",
                            "legend", "off"
                        ),
                        rowspan=1,
                        colspan=3,
                    ),
                ],
            ]
        )

    def update_socialism_dashboards(self, project_id):
        self.update(
            project_id=project_id,
            dashboard_id="bulbasaur_socialism_dashboard",
            name="Socialism: Stats",
            parameters=self.SOCIALISM_PARAMETERS,
            rows=[
                [
                    Panel(
                        title="Методы API - RPS",
                        url=DashboardBuilder.url_query(
                            "project", project_id,
                            "cluster", "{{cluster}}",
                            "service", "{{service}}",
                            "host", "{{host}}",
                            "skill_id", "{{skill_id}}",
                            "graph", "socialism_api_calls_rps",
                            "legend", "off"
                        ),
                        rowspan=1,
                        colspan=2,
                    ),
                    Panel(
                        title="Методы API - Ошибки, %",
                        url=DashboardBuilder.url_query(
                            "project", project_id,
                            "cluster", "{{cluster}}",
                            "service", "{{service}}",
                            "host", "{{host}}",
                            "skill_id", "{{skill_id}}",
                            "graph", "socialism_api_errors",
                            "legend", "off"
                        ),
                        rowspan=1,
                        colspan=2,
                    ),
                    Panel(
                        title="Методы API - Тайминги, с (квантиль 99.9%)",
                        url=DashboardBuilder.url_query(
                            "project", project_id,
                            "cluster", "{{cluster}}",
                            "service", "{{service}}",
                            "host", "{{host}}",
                            "skill_id", "{{skill_id}}",
                            "graph", "socialism_api_call_percentiles",
                            "legend", "off"
                        ),
                        rowspan=1,
                        colspan=2,
                    ),
                ],
                [
                    Panel(
                        title="Методы API - getUserToken, RPS",
                        url=DashboardBuilder.url_query(
                            "project", project_id,
                            "cluster", "{{cluster}}",
                            "service", "{{service}}",
                            "host", "{{host}}",
                            "skill_id", "{{skill_id}}",
                            "graph", "socialism_api_call_getUserToken_rps",
                            "legend", "off"
                        ),
                        rowspan=1,
                        colspan=3,
                    ),
                    Panel(
                        title="Методы API - getUserToken, %",
                        url=DashboardBuilder.url_query(
                            "project", project_id,
                            "cluster", "{{cluster}}",
                            "service", "{{service}}",
                            "host", "{{host}}",
                            "skill_id", "{{skill_id}}",
                            "graph", "socialism_api_call_getUserToken_percent",
                            "legend", "off"
                        ),
                        rowspan=1,
                        colspan=3,
                    ),
                ],
                [
                    Panel(
                        title="Методы API - checkUserToken, RPS",
                        url=DashboardBuilder.url_query(
                            "project", project_id,
                            "cluster", "{{cluster}}",
                            "service", "{{service}}",
                            "host", "{{host}}",
                            "skill_id", "{{skill_id}}",
                            "graph", "socialism_api_call_checkUserToken_rps",
                            "legend", "off"
                        ),
                        rowspan=1,
                        colspan=3,
                    ),
                    Panel(
                        title="Методы API - checkUserToken, %",
                        url=DashboardBuilder.url_query(
                            "project", project_id,
                            "cluster", "{{cluster}}",
                            "service", "{{service}}",
                            "host", "{{host}}",
                            "skill_id", "{{skill_id}}",
                            "graph", "socialism_api_call_checkUserToken_percent",
                            "legend", "off"
                        ),
                        rowspan=1,
                        colspan=3,
                    ),
                ],
                [
                    Panel(
                        title="Методы API - deleteUserToken, RPS",
                        url=DashboardBuilder.url_query(
                            "project", project_id,
                            "cluster", "{{cluster}}",
                            "service", "{{service}}",
                            "host", "{{host}}",
                            "skill_id", "{{skill_id}}",
                            "graph", "socialism_api_call_deleteUserToken_rps",
                            "legend", "off"
                        ),
                        rowspan=1,
                        colspan=3,
                    ),
                    Panel(
                        title="Методы API - deleteUserToken, %",
                        url=DashboardBuilder.url_query(
                            "project", project_id,
                            "cluster", "{{cluster}}",
                            "service", "{{service}}",
                            "host", "{{host}}",
                            "skill_id", "{{skill_id}}",
                            "graph", "socialism_api_call_deleteUserToken_percent",
                            "legend", "off"
                        ),
                        rowspan=1,
                        colspan=3,
                    ),
                ],
            ]
        )

    def update_tvm_dashboards(self, project_id):
        self.update(
            project_id=project_id,
            dashboard_id="bulbasaur_tvm_dashboard",
            name="TVM: Stats",
            parameters=self.PARAMETERS,
            rows=[
                [
                    Panel(
                        title="Методы API - RPS",
                        url=DashboardBuilder.url_query(
                            "project", project_id,
                            "cluster", "{{cluster}}",
                            "service", "{{service}}",
                            "host", "{{host}}",
                            "skill_id", "{{skill_id}}",
                            "graph", "tvm_api_calls_rps",
                            "legend", "off"
                        ),
                        rowspan=1,
                        colspan=2,
                    ),
                    Panel(
                        title="Методы API - Ошибки, %",
                        url=DashboardBuilder.url_query(
                            "project", project_id,
                            "cluster", "{{cluster}}",
                            "service", "{{service}}",
                            "host", "{{host}}",
                            "skill_id", "{{skill_id}}",
                            "graph", "tvm_api_errors",
                            "legend", "off"
                        ),
                        rowspan=1,
                        colspan=2,
                    ),
                    Panel(
                        title="Методы API - Тайминги, с (квантиль 99.9%)",
                        url=DashboardBuilder.url_query(
                            "project", project_id,
                            "cluster", "{{cluster}}",
                            "service", "{{service}}",
                            "host", "{{host}}",
                            "skill_id", "{{skill_id}}",
                            "graph", "tvm_api_call_percentiles",
                            "legend", "off"
                        ),
                        rowspan=1,
                        colspan=2,
                    ),
                ],
                [
                    Panel(
                        title="Методы API - getServiceTicket, RPS",
                        url=DashboardBuilder.url_query(
                            "project", project_id,
                            "cluster", "{{cluster}}",
                            "service", "{{service}}",
                            "host", "{{host}}",
                            "skill_id", "{{skill_id}}",
                            "graph", "tvm_api_call_getServiceTicket_rps",
                            "legend", "off"
                        ),
                        rowspan=1,
                        colspan=3,
                    ),
                    Panel(
                        title="Методы API - getServiceTicket, %",
                        url=DashboardBuilder.url_query(
                            "project", project_id,
                            "cluster", "{{cluster}}",
                            "service", "{{service}}",
                            "host", "{{host}}",
                            "skill_id", "{{skill_id}}",
                            "graph", "tvm_api_call_getServiceTicket_percent",
                            "legend", "off"
                        ),
                        rowspan=1,
                        colspan=3,
                    ),
                ],
                [
                    Panel(
                        title="Методы API - checkServiceTicket, RPS",
                        url=DashboardBuilder.url_query(
                            "project", project_id,
                            "cluster", "{{cluster}}",
                            "service", "{{service}}",
                            "host", "{{host}}",
                            "skill_id", "{{skill_id}}",
                            "graph", "tvm_api_call_checkServiceTicket_rps",
                            "legend", "off"
                        ),
                        rowspan=1,
                        colspan=3,
                    ),
                    Panel(
                        title="Методы API - checkServiceTicket, %",
                        url=DashboardBuilder.url_query(
                            "project", project_id,
                            "cluster", "{{cluster}}",
                            "service", "{{service}}",
                            "host", "{{host}}",
                            "skill_id", "{{skill_id}}",
                            "graph", "tvm_api_call_checkServiceTicket_percent",
                            "legend", "off"
                        ),
                        rowspan=1,
                        colspan=3,
                    ),
                ],
                [
                    Panel(
                        title="Методы API - checkUserTicket, RPS",
                        url=DashboardBuilder.url_query(
                            "project", project_id,
                            "cluster", "{{cluster}}",
                            "service", "{{service}}",
                            "host", "{{host}}",
                            "graph", "tvm_api_call_checkUserTicket_rps",
                            "legend", "off"
                        ),
                        rowspan=1,
                        colspan=3,
                    ),
                    Panel(
                        title="Методы API - checkUserTicket, %",
                        url=DashboardBuilder.url_query(
                            "project", project_id,
                            "cluster", "{{cluster}}",
                            "service", "{{service}}",
                            "host", "{{host}}",
                            "graph", "tvm_api_call_checkUserTicket_percent",
                            "legend", "off"
                        ),
                        rowspan=1,
                        colspan=3,
                    ),
                ],
            ]
        )

    def update_steelix_dashboards(self, project_id):
        self.update(
            project_id=project_id,
            dashboard_id="bulbasaur_steelix_dashboard",
            name="Steelix: Stats",
            parameters=self.PARAMETERS,
            rows=[
                [
                    Panel(
                        title="Методы API - RPS",
                        url=DashboardBuilder.url_query(
                            "project", project_id,
                            "cluster", "{{cluster}}",
                            "service", "{{service}}",
                            "host", "{{host}}",
                            "graph", "steelix_api_calls_rps",
                            "legend", "off"
                        ),
                        rowspan=1,
                        colspan=2,
                    ),
                    Panel(
                        title="Методы API - Ошибки, %",
                        url=DashboardBuilder.url_query(
                            "project", project_id,
                            "cluster", "{{cluster}}",
                            "service", "{{service}}",
                            "host", "{{host}}",
                            "graph", "steelix_api_errors",
                            "legend", "off"
                        ),
                        rowspan=1,
                        colspan=2,
                    ),
                    Panel(
                        title="Методы API - Тайминги, с (квантиль 99.9%)",
                        url=DashboardBuilder.url_query(
                            "project", project_id,
                            "cluster", "{{cluster}}",
                            "service", "{{service}}",
                            "host", "{{host}}",
                            "graph", "steelix_api_call_percentiles",
                            "legend", "off"
                        ),
                        rowspan=1,
                        colspan=2,
                    ),
                ],
                [
                    Panel(
                        title="Методы API - callbackDiscovery, RPS",
                        url=DashboardBuilder.url_query(
                            "project", project_id,
                            "cluster", "{{cluster}}",
                            "service", "{{service}}",
                            "host", "{{host}}",
                            "graph", "steelix_api_call_callbackDiscovery_rps",
                            "legend", "off"
                        ),
                        rowspan=1,
                        colspan=3,
                    ),
                    Panel(
                        title="Методы API - callbackDiscovery, %",
                        url=DashboardBuilder.url_query(
                            "project", project_id,
                            "cluster", "{{cluster}}",
                            "service", "{{service}}",
                            "host", "{{host}}",
                            "graph", "steelix_api_call_callbackDiscovery_percent",
                            "legend", "off"
                        ),
                        rowspan=1,
                        colspan=3,
                    ),
                ],
                [
                    Panel(
                        title="Методы API - callbackState, RPS",
                        url=DashboardBuilder.url_query(
                            "project", project_id,
                            "cluster", "{{cluster}}",
                            "service", "{{service}}",
                            "host", "{{host}}",
                            "graph", "steelix_api_call_callbackState_rps",
                            "legend", "off"
                        ),
                        rowspan=1,
                        colspan=3,
                    ),
                    Panel(
                        title="Методы API - callbackState, %",
                        url=DashboardBuilder.url_query(
                            "project", project_id,
                            "cluster", "{{cluster}}",
                            "service", "{{service}}",
                            "host", "{{host}}",
                            "graph", "steelix_api_call_callbackState_percent",
                            "legend", "off"
                        ),
                        rowspan=1,
                        colspan=3,
                    ),
                ],
                [
                    Panel(
                        title="Методы API - pushDiscovery, RPS",
                        url=DashboardBuilder.url_query(
                            "project", project_id,
                            "cluster", "{{cluster}}",
                            "service", "{{service}}",
                            "host", "{{host}}",
                            "graph", "steelix_api_call_pushDiscovery_rps",
                            "legend", "off"
                        ),
                        rowspan=1,
                        colspan=3,
                    ),
                    Panel(
                        title="Методы API - pushDiscovery, %",
                        url=DashboardBuilder.url_query(
                            "project", project_id,
                            "cluster", "{{cluster}}",
                            "service", "{{service}}",
                            "host", "{{host}}",
                            "graph", "steelix_api_call_pushDiscovery_percent",
                            "legend", "off"
                        ),
                        rowspan=1,
                        colspan=3,
                    ),
                ],
            ]
        )

    def update_timemachine_dashboards(self, project_id):
        self.update(
            project_id=project_id,
            dashboard_id="neighbour_time_machine_dashboard",
            name="Timemachine: Stats",
            parameters=self.PARAMETERS,
            rows=[
                [
                    Panel(
                        title="Методы API - RPS",
                        url=DashboardBuilder.url_query(
                            "project", project_id,
                            "cluster", "{{cluster}}",
                            "service", "{{service}}",
                            "host", "{{host}}",
                            "graph", "time_machine_api_calls_rps",
                            "legend", "off"
                        ),
                        rowspan=1,
                        colspan=2,
                    ),
                    Panel(
                        title="Методы API - Ошибки, %",
                        url=DashboardBuilder.url_query(
                            "project", project_id,
                            "cluster", "{{cluster}}",
                            "service", "{{service}}",
                            "host", "{{host}}",
                            "graph", "time_machine_api_errors",
                            "legend", "off"
                        ),
                        rowspan=1,
                        colspan=2,
                    ),
                    Panel(
                        title="Методы API - Тайминги, с (квантиль 99.9%)",
                        url=DashboardBuilder.url_query(
                            "project", project_id,
                            "cluster", "{{cluster}}",
                            "service", "{{service}}",
                            "host", "{{host}}",
                            "graph", "time_machine_api_call_percentiles",
                            "legend", "off"
                        ),
                        rowspan=1,
                        colspan=2,
                    ),
                ],
                [
                    Panel(
                        title="Методы API - submitTask, RPS",
                        url=DashboardBuilder.url_query(
                            "project", project_id,
                            "cluster", "{{cluster}}",
                            "service", "{{service}}",
                            "host", "{{host}}",
                            "graph", "time_machine_api_call_submitTask_rps",
                            "legend", "off"
                        ),
                        rowspan=1,
                        colspan=3,
                    ),
                    Panel(
                        title="Методы API - submitTask, %",
                        url=DashboardBuilder.url_query(
                            "project", project_id,
                            "cluster", "{{cluster}}",
                            "service", "{{service}}",
                            "host", "{{host}}",
                            "graph", "time_machine_api_call_submitTask_percent",
                            "legend", "off"
                        ),
                        rowspan=1,
                        colspan=3,
                    ),
                ],
            ]
        )

    def update_memento_dashboards(self, project_id):
        self.update(
            project_id=project_id,
            dashboard_id="bulbasaur_memento_dashboard",
            name="Memento: Stats",
            parameters=self.PARAMETERS,
            rows=[
                [
                    Panel(
                        title="Методы API - RPS",
                        url=DashboardBuilder.url_query(
                            "project", project_id,
                            "cluster", "{{cluster}}",
                            "service", "{{service}}",
                            "host", "{{host}}",
                            "graph", "memento_api_calls_rps",
                            "legend", "off"
                        ),
                        rowspan=1,
                        colspan=2,
                    ),
                    Panel(
                        title="Методы API - Ошибки, %",
                        url=DashboardBuilder.url_query(
                            "project", project_id,
                            "cluster", "{{cluster}}",
                            "service", "{{service}}",
                            "host", "{{host}}",
                            "graph", "memento_api_errors",
                            "legend", "off"
                        ),
                        rowspan=1,
                        colspan=2,
                    ),
                    Panel(
                        title="Методы API - Тайминги, с (квантиль 99.9%)",
                        url=DashboardBuilder.url_query(
                            "project", project_id,
                            "cluster", "{{cluster}}",
                            "service", "{{service}}",
                            "host", "{{host}}",
                            "graph", "memento_api_call_percentiles",
                            "legend", "off"
                        ),
                        rowspan=1,
                        colspan=2,
                    ),
                ],
                [
                    Panel(
                        title="Методы API - getUserObjects, RPS",
                        url=DashboardBuilder.url_query(
                            "project", project_id,
                            "cluster", "{{cluster}}",
                            "service", "{{service}}",
                            "host", "{{host}}",
                            "graph", "memento_api_call_getUserObjects_rps",
                            "legend", "off"
                        ),
                        rowspan=1,
                        colspan=3,
                    ),
                    Panel(
                        title="Методы API - getUserObjects, %",
                        url=DashboardBuilder.url_query(
                            "project", project_id,
                            "cluster", "{{cluster}}",
                            "service", "{{service}}",
                            "host", "{{host}}",
                            "graph", "memento_api_call_getUserObjects_percent",
                            "legend", "off"
                        ),
                        rowspan=1,
                        colspan=3,
                    ),
                ],
                [
                    Panel(
                        title="Методы API - updateUserObjects, RPS",
                        url=DashboardBuilder.url_query(
                            "project", project_id,
                            "cluster", "{{cluster}}",
                            "service", "{{service}}",
                            "host", "{{host}}",
                            "graph", "memento_api_call_updateUserObjects_rps",
                            "legend", "off"
                        ),
                        rowspan=1,
                        colspan=3,
                    ),
                    Panel(
                        title="Методы API - updateUserObjects, %",
                        url=DashboardBuilder.url_query(
                            "project", project_id,
                            "cluster", "{{cluster}}",
                            "service", "{{service}}",
                            "host", "{{host}}",
                            "graph", "memento_api_call_updateUserObjects_percent",
                            "legend", "off"
                        ),
                        rowspan=1,
                        colspan=3,
                    ),
                ],
            ]
        )

    def update_processors_dashboard(self, project_id):
        self.update(
            project_id=project_id,
            dashboard_id="bulbasaur_processors_dashboard",
            name="Bulbasaur: Processors stats",
            parameters=self.PARAMETERS,
            rows=[
                [
                    Panel(
                        title="Processors: Run Total RPS",
                        url=DashboardBuilder.url_query(
                            "project", project_id,
                            "cluster", "{{cluster}}",
                            "service", "{{service}}",
                            "host", "{{host}}",
                            "step", "run",
                            "graph", "bulbasaur_processor_request_total",
                            "legend", "off"
                        ),
                        rowspan=1,
                        colspan=2,
                    ),
                    Panel(
                        title="Processors: Run Errors RPS",
                        url=DashboardBuilder.url_query(
                            "project", project_id,
                            "cluster", "{{cluster}}",
                            "service", "{{service}}",
                            "host", "{{host}}",
                            "graph", "bulbasaur_processor_request_errors",
                            "step", "run",
                            "legend", "off"
                        ),
                        rowspan=1,
                        colspan=2,
                    ),
                    Panel(
                        title="Processors: Run Timings, с (квантиль 99.9%)",
                        url=DashboardBuilder.url_query(
                            "project", project_id,
                            "cluster", "{{cluster}}",
                            "service", "{{service}}",
                            "host", "{{host}}",
                            "graph", "bulbasaur_processor_percentiles",
                            "step", "run",
                            "legend", "off"
                        ),
                        rowspan=1,
                        colspan=2,
                    ),
                ],
                [
                    Panel(
                        title="Processors: Apply Total RPS",
                        url=DashboardBuilder.url_query(
                            "project", project_id,
                            "cluster", "{{cluster}}",
                            "service", "{{service}}",
                            "host", "{{host}}",
                            "step", "apply",
                            "graph", "bulbasaur_processor_request_total",
                            "legend", "off"
                        ),
                        rowspan=1,
                        colspan=2,
                    ),
                    Panel(
                        title="Processors: Apply Errors RPS",
                        url=DashboardBuilder.url_query(
                            "project", project_id,
                            "cluster", "{{cluster}}",
                            "service", "{{service}}",
                            "host", "{{host}}",
                            "graph", "bulbasaur_processor_request_errors",
                            "step", "apply",
                            "legend", "off"
                        ),
                        rowspan=1,
                        colspan=2,
                    ),
                    Panel(
                        title="Processors: Apply Timings, с (квантиль 99.9%)",
                        url=DashboardBuilder.url_query(
                            "project", project_id,
                            "cluster", "{{cluster}}",
                            "service", "{{service}}",
                            "host", "{{host}}",
                            "graph", "bulbasaur_processor_percentiles",
                            "step", "apply",
                            "legend", "off"
                        ),
                        rowspan=1,
                        colspan=2,
                    ),
                ],
            ])


class BulbasaurAlerts(AlertBuilderV2):
    def __init__(self, oauth_token):
        AlertBuilderV2.__init__(self, oauth_token)

    channels = [
        {
            "id": "galecore_telegram",
            "config":
                {
                    "notifyAboutStatuses": [
                        "ALARM",
                        "OK",
                        "WARN",
                        "ERROR",
                    ]
                },
        },
        {
            "id": "iot_alerts_telegram",
            "config":
                {
                    "notifyAboutStatuses": [
                        "ALARM",
                        "OK",
                        "WARN",
                        "ERROR",
                    ]
                },
        }
    ]

    @staticmethod
    def compute_period(m=0, s=0):
        return m * 60 + s

    @staticmethod
    def annotations(*args):
        return dict(args)

    @staticmethod
    def traffic_light_annotations():
        return (
            ("trafficLight.color", "{{expression.trafficColor}}"),
            ("trafficLight.value", "{{expression.current_value}}"),
        )

    @staticmethod
    def notification_message_annotation(graph_link):
        message = '''
        Alert: {{alert.projectId}} {{alert.id}}
        {{alert.name}}

        Current status: {{status.code}}
        Current value: {{expression.current_value}}

        From time: {{fromTime}}
        To time: {{toTime}}

        Graph: ''' + graph_link
        return 'notification_message', inspect.cleandoc(message)

    def update_alerts(self, project_id):
        self.update_megamind_alerts(project_id)
        self.update_socialism_alerts(project_id)

    def update_socialism_alerts(self, project_id):
        def socialism_api_errors_alert(alert_id, alert_name):
            crit_cnt_percent_4xx_name = 'crit_cnt_percent_4xx'
            crit_cnt_percent_5xx_name = 'crit_cnt_percent_5xx'
            crit_cnt_percent_unavailable_name = 'crit_cnt_percent_unavailable'
            crit_cnt_percent_get_user_token_4xx_name = 'crit_cnt_percent_get_user_token_4xx'

            avg_percent_4xx_name = 'avg_percent_4xx'
            avg_percent_5xx_name = 'avg_percent_5xx'
            avg_percent_unavailable_name = 'avg_percent_unavailable'
            avg_percent_get_user_token_4xx_name = 'avg_percent_get_user_token_4xx'

            percent_4xx_threshold = 3
            percent_5xx_threshold = 1
            percent_unavailable_threshold = 2
            percent_get_user_token_4xx_threshold = 3

            evaluation_window = 90
            # Values are received every 15 seconds, so there are evaluation_window/15 points in each window
            crit_points_threshold = evaluation_window / 15 - 1

            sensor_common = f'neighbour=socialism, service="bulbasaur", cluster="bulbasaur_production", host="cluster", skill_id="all"'
            sensor_4xx = 'http_code="4xx"'
            sensor_5xx = 'http_code="5xx"'
            sensor_get_user_token = f'call="getUserToken"'

            # For each error count points with value higher than the corresponding threshold.
            # Alert if all points in the window have critical values.
            expression = f'let {avg_percent_4xx_name} = round(avg(replace_nan(sum(neighbours.calls{{{", ".join([sensor_common, sensor_4xx])}}}) by neighbour / sum(neighbours.total{{{sensor_common}}}) by neighbour * 100, 0)) * 100) / 100;\n\n' \
                         f'let {avg_percent_5xx_name} = round(avg(replace_nan(sum(neighbours.calls{{{", ".join([sensor_common, sensor_5xx])}}}) by neighbour / sum(neighbours.total{{{sensor_common}}}) by neighbour * 100, 0)) * 100) / 100;\n\n' \
                         f'let {avg_percent_unavailable_name} = round(avg(replace_nan(sum(neighbours.fails{{{sensor_common}}}) by neighbour / sum(neighbours.total{{{sensor_common}}}) by neighbour * 100, 0)) * 100) / 100;\n\n' \
                         f'let {avg_percent_get_user_token_4xx_name} = round(avg(replace_nan(sum(neighbours.calls{{{", ".join([sensor_common, sensor_4xx, sensor_get_user_token])}}}) by neighbour / sum(neighbours.total{{{", ".join([sensor_common, sensor_get_user_token])}}}) by neighbour * 100, 0)) * 100) / 100;\n\n' \
                         f'let {crit_cnt_percent_4xx_name} = count(drop_below(replace_nan(sum(neighbours.calls{{{", ".join([sensor_common, sensor_4xx])}}}) by neighbour / sum(neighbours.total{{{sensor_common}}}) by neighbour * 100, 0), {percent_4xx_threshold}));\n\n' \
                         f'let {crit_cnt_percent_5xx_name} = count(drop_below(replace_nan(sum(neighbours.calls{{{", ".join([sensor_common, sensor_5xx])}}}) by neighbour / sum(neighbours.total{{{sensor_common}}}) by neighbour * 100, 0), {percent_5xx_threshold}));\n\n' \
                         f'let {crit_cnt_percent_unavailable_name} = count(drop_below(replace_nan(sum(neighbours.fails{{{sensor_common}}}) by neighbour / sum(neighbours.total{{{sensor_common}}}) by neighbour * 100, 0), {percent_unavailable_threshold}));\n\n' \
                         f'let {crit_cnt_percent_get_user_token_4xx_name} = count(drop_below(replace_nan(sum(neighbours.calls{{{", ".join([sensor_common, sensor_4xx, sensor_get_user_token])}}}) by neighbour / sum(neighbours.total{{{", ".join([sensor_common, sensor_get_user_token])}}}) by neighbour * 100, 0), {percent_get_user_token_4xx_threshold}));\n\n' \
                         f'alarm_if({crit_cnt_percent_4xx_name} > {crit_points_threshold});\n' \
                         f'alarm_if({crit_cnt_percent_5xx_name} > {crit_points_threshold});\n' \
                         f'alarm_if({crit_cnt_percent_unavailable_name} > {crit_points_threshold});\n' \
                         f'alarm_if({crit_cnt_percent_get_user_token_4xx_name} > {crit_points_threshold});\n'

            notification_message = inspect.cleandoc(
                f'''
                    Errors in socialism.
                    4xx: {{{{expression.{avg_percent_4xx_name}}}}}%
                    5xx: {{{{expression.{avg_percent_5xx_name}}}}}%
                    unavailable: {{{{expression.{avg_percent_unavailable_name}}}}}%
                    4xx in getUserToken: {{{{expression.{avg_percent_get_user_token_4xx_name}}}}}%
                    Solomon stats: https://solomon.yandex-team.ru/?project=alice-iot&cluster=bulbasaur_production&skill_id=all&service=bulbasaur&host=cluster&dashboard=bulbasaur_socialism_dashboard&b=1h&e=
                '''
            )

            annotations = {
                'notification_message': notification_message
            }

            alert = Alert(
                program=expression,
                annotations = annotations
            )

            self.update(
                project_id=project_id,
                alert_id=alert_id,
                name=alert_name,
                alert=alert,
                channels=self.channels,
                window_secs=evaluation_window,
                description='https://st.yandex-team.ru/IOT-1677',
            )

        socialism_api_errors_alert('bulbasaur_socialism_api_errors', 'Bulbasaur: socialism api errors')

    def update_megamind_alerts(self, project_id):
        def path_timings_alert(path, method, graph_link):
            timings = '''{{
                service="bulbasaur", cluster="bulbasaur_production", host="cluster",
                sensor="handlers.duration_buckets", path="{path}", http_method="{method}", bin!="inf"
            }}'''.format(path=path, method=method)
            head, mid, tail = AQL.split_timeseries('timings', n_head=4, n_tail=4)
            head_avg_percentile = AQL.average(AQL.histogram_percentile(99.9, head))
            mid_avg_percentile = AQL.average(AQL.histogram_percentile(99.9, mid))
            tail_avg_percentile = AQL.average(AQL.histogram_percentile(99.9, tail))
            avg_percentile_rounded = AQL.to_fixed(AQL.average(AQL.histogram_percentile(99.9, 'timings')), 2)

            is_red = AQL.logical_and(
                AQL.gt('head_avg_percentile', 0.3), AQL.gt('mid_avg_percentile', 0.3),
                AQL.gt('tail_avg_percentile', 0.3)
            )
            is_yellow = AQL.logical_and(
                AQL.gt('head_avg_percentile', 0.2), AQL.gt('mid_avg_percentile', 0.2),
                AQL.gt('tail_avg_percentile', 0.2)
            )
            evaluation = AQL.alert_color_evaluation(is_red, is_yellow)
            return Alert(
                program='''
                let timings = {timings};

                let head_avg_percentile = {head_avg_percentile};
                let mid_avg_percentile = {mid_avg_percentile};
                let tail_avg_percentile = {tail_avg_percentile};

                let current_value = {current_value};

                {evaluation}
                '''.format(
                    timings=timings,
                    head_avg_percentile=head_avg_percentile,
                    mid_avg_percentile=mid_avg_percentile,
                    tail_avg_percentile=tail_avg_percentile,
                    current_value=avg_percentile_rounded,
                    evaluation=evaluation
                ),
                annotations=self.annotations(
                    self.notification_message_annotation(graph_link),
                    *self.traffic_light_annotations()
                ),
            )

        def path_errors_percent_alert(path, code, graph_link):
            errors = '''{{
                service="bulbasaur", cluster="bulbasaur_production", sensor="handlers.calls",
                host="cluster", path="{path}", http_code="{code}"
            }}'''.format(path=path, code=code)
            total = '''{{
                service="bulbasaur", cluster="bulbasaur_production", sensor="handlers.calls",
                host="cluster", path="{path}"
            }}'''.format(path=path)

            head, mid, tail = AQL.split_timeseries(errors, n_head=4, n_tail=4)
            head_total, mid_total, tail_total = AQL.split_timeseries(total, n_head=4, n_tail=4)

            error_calls_head = AQL.group_lines('sum', head)
            total_calls_head = AQL.group_lines('sum', head_total)

            error_calls_mid = AQL.group_lines('sum', mid)
            total_calls_mid = AQL.group_lines('sum', mid_total)

            error_calls_tail = AQL.group_lines('sum', tail)
            total_calls_tail = AQL.group_lines('sum', tail_total)

            percent_head = AQL.average(AQL.percent_expression('error_calls_head', 'total_calls_head'))
            percent_mid = AQL.average(AQL.percent_expression('error_calls_mid', 'total_calls_mid'))
            percent_tail = AQL.average(AQL.percent_expression('error_calls_tail', 'total_calls_tail'))
            percent_rounded = AQL.to_fixed(AQL.arithmetic_average('percent_head', 'percent_mid', 'percent_tail'), 2)

            is_red = AQL.logical_and(
                AQL.gt('percent_head', 1), AQL.gt('percent_mid', 1), AQL.gt('percent_tail', 1)
            )
            is_yellow = AQL.logical_and(
                AQL.gt('percent_head', 0.5), AQL.gt('percent_mid', 0.5), AQL.gt('percent_tail', 0.5)
            )
            evaluation = AQL.alert_color_evaluation(is_red, is_yellow)
            return Alert(
                program='''
                let error_calls_head = {error_calls_head};
                let total_calls_head = {total_calls_head};

                let error_calls_mid = {error_calls_mid};
                let total_calls_mid = {total_calls_mid};

                let error_calls_tail = {error_calls_tail};
                let total_calls_tail = {total_calls_tail};

                let percent_head = {percent_head};
                let percent_mid = {percent_mid};
                let percent_tail = {percent_tail};
                let current_value = {current_value};

                {evaluation}
                '''.format(
                    error_calls_head=error_calls_head,
                    total_calls_head=total_calls_head,
                    error_calls_mid=error_calls_mid,
                    total_calls_mid=total_calls_mid,
                    error_calls_tail=error_calls_tail,
                    total_calls_tail=total_calls_tail,
                    percent_head=percent_head,
                    percent_mid=percent_mid,
                    percent_tail=percent_tail,
                    current_value=percent_rounded,
                    evaluation=evaluation
                ),
                annotations=self.annotations(
                    self.notification_message_annotation(graph_link),
                    *self.traffic_light_annotations()
                ),
            )

        mm_timings_graph_link = "https://solomon.yandex-team.ru/?project=alice-iot&" \
                                "cluster=bulbasaur_production&service=bulbasaur&host=cluster&" \
                                "graph=bulbasaur_megamind_handlers_percentiles"
        mm_4xx_errors_graph_link = "https://solomon.yandex-team.ru/?project=alice-iot&" \
                                   "cluster=bulbasaur_production&service=bulbasaur&" \
                                   "graph=bulbasaur_mm_4xx_percent_stacked_dc"
        mm_5xx_errors_graph_link = "https://solomon.yandex-team.ru/?project=alice-iot&" \
                                   "cluster=bulbasaur_production&service=bulbasaur&" \
                                   "graph=bulbasaur_mm_5xx_percent_stacked_dc"
        # self.update(
        #     project_id=project_id,
        #     alert_id="bulbasaur_mm_userinfo_timings",
        #     name="Bulbasaur: MM userinfo timings",
        #     alert=path_timings_alert("/v1.0/user/info", "GET", mm_timings_graph_link),
        #     channels=self.channels,
        #     window_secs=self.compute_period(m=4),
        # )
        # self.update(
        #     project_id=project_id,
        #     alert_id="bulbasaur_mm_actions_timings",
        #     name="Bulbasaur: MM actions timings",
        #     alert=path_timings_alert("/v1.0/user/devices/actions", "POST", mm_timings_graph_link),
        #     channels=self.channels,
        #     window_secs=self.compute_period(m=4),
        # )
        self.update(
            project_id=project_id,
            alert_id="bulbasaur_mm_http_errors_4xx_percent",
            name="Bulbasaur: MM HTTP 4xx Errors %",
            alert=path_errors_percent_alert("/v1.0/*", "4xx", mm_4xx_errors_graph_link),
            channels=self.channels,
            window_secs=self.compute_period(m=4),
        )
        self.update(
            project_id=project_id,
            alert_id="bulbasaur_mm_http_errors_5xx_percent",
            name="Bulbasaur: MM HTTP 5xx Errors %",
            alert=path_errors_percent_alert("/v1.0/*", "5xx", mm_5xx_errors_graph_link),
            channels=self.channels,
            window_secs=self.compute_period(m=4),
        )
