const deviceOperations = [
    {
        value: 'device:create',
        content: 'Создать устройство'
    },
    {
        value: 'device:change',
        content: 'Редактировать устройство'
    },
    {
        value: 'device:add-puid',
        content: 'Добавить устройству puid умного дома'
    },
]

const organizationOperations = [
    {
        value: 'organization:create',
        content: 'Создать организацию'
    },
    {
        value: 'organization:change',
        content: 'Редактировать организацию'
    },
    {
        value: 'organization:set-max-station-volume',
        content: 'Установить максимальную громкость для станций организации'
    },
]

const promocodeOperations = [
    {
        value: 'promocode:add',
        content: 'Добавить промокоды'
    },
    {
        value: 'promocode:add-to-organization',
        content: 'Добавить промокоды в организацию'
    },
]

const roomOperations = [
    {
        value: 'room:create',
        content: 'Создать комнату'
    },
]

const userOperations = [
    {
        value: 'users:create',
        content: 'Создать пользователя'
    },
    {
        value: 'users:bind',
        content: 'Прикрепить пользователя к организации'
    },
]

const operations = [
    ...deviceOperations,
    ...organizationOperations,
    ...promocodeOperations,
    ...roomOperations,
    ...userOperations
]

enum platforms {
    YandexStation = 'yandexstation',
    YandexMini = 'yandexmini',
    YandexStation2 = 'yandexstation_2',
    YandexLight = 'yandexmicro',
    YandexMini2 = 'yandexmini_2',
    CVTE351 = 'yandex_tv_hisi351_cvte',
    RTK2861 = 'yandex_tv_rt2861_hikeen',
    RTK2871 = 'yandex_tv_rt2871_hikeen',
    CV9632 = 'yandex_tv_mt9632_cv',
    Module = 'yandexmodule_2',
}

export type RequiredInputType = 'inputNumber' | 'inputString' | 'selectCheck';
export type ParamNameType = Partial<'deviceId' | 'orgDeviceIdsCheckbox' | 'puid' | 'primaryId' | 'platform' | 'externalDeviceId' | 'note' | 'status'
    | 'organizationId' | 'kolonkishId' | 'kolonkishLogin' | 'name' | 'connectOrgId' | 'login' | 'organizationIdsCheckbox'
    | 'templateURL' | 'imageUrl' | 'infoUrl' | 'infoTitle' | 'infoSubtitle' | 'ticketKey' | 'userIdsCheckbox'
    | 'usesRooms' | 'newMaxStationVolume' | 'maxStationVolume' | 'codes' | 'codesNum' | 'externalRoomId'>;
export interface CmdValueType {
    cmd: string | string[];
    paramName: ParamNameType;
    important: boolean;
}
const scenarios: Record<string, Array<Record<string, CmdValueType>>> = {
    "device:add-puid": [
        {'Выберите устройство':
            {
                cmd: 'deviceId',
                paramName: 'deviceId',
                important: true
            }
        },
        {'Введите puid умного дома':
            {
                cmd: 'inputNumber',
                paramName: 'puid',
                important: true
            }
        },
    ],
    "device:create": [
        {'Выберите платформу':
            {
                cmd: Object.values(platforms),
                paramName: 'platform',
                important: true
            }
        },
        {'Device id':
            {
                cmd: 'inputString',
                paramName: 'deviceId',
                important: true
            }
        },
        {'External device id':
            {
                cmd: 'inputString',
                paramName: 'externalDeviceId',
                important: false
            }
        },
        {'Пометка':
            {
                cmd: 'inputString',
                paramName: 'note',
                important: false
            }
        },
        {'Статус':
            {
                cmd: ['reset', 'inactive', 'active'],
                paramName: 'status',
                important: true
            }
        },
        {'Выберите организацию':
            {
                cmd: 'organizationId',
                paramName: 'organizationId',
                important: true
            }
        },
        {'Kolonkish ID':
            {
                cmd: 'inputString',
                paramName: 'kolonkishId',
                important: false
            }
        },
        {'Kolonkish Login':
            {
                cmd: 'inputString',
                paramName: 'kolonkishLogin',
                important: false
            }
        },
    ],
    "device:change": [
        {'Выберите устройство':
                {
                    cmd: 'deviceId',
                    paramName: 'primaryId',
                    important: true
                }
        },
        {'Выберите платформу':
                {
                    cmd: Object.values(platforms),
                    paramName: 'platform',
                    important: true
                }
        },
        {'Device id':
                {
                    cmd: 'inputString',
                    paramName: 'deviceId',
                    important: true
                }
        },
        {'External device id':
                {
                    cmd: 'inputString',
                    paramName: 'externalDeviceId',
                    important: false
                }
        },
        {'Пометка':
                {
                    cmd: 'inputString',
                    paramName: 'note',
                    important: false
                }
        },
        {'Статус':
                {
                    cmd: ['reset', 'inactive', 'active'],
                    paramName: 'status',
                    important: true
                }
        },
        {'Выберите организацию':
                {
                    cmd: 'organizationId',
                    paramName: 'organizationId',
                    important: true
                }
        },
        {'Kolonkish ID':
                {
                    cmd: 'inputString',
                    paramName: 'kolonkishId',
                    important: false
                }
        },
        {'Kolonkish Login':
                {
                    cmd: 'inputString',
                    paramName: 'kolonkishLogin',
                    important: false
                }
        },
    ],
    "organization:change": [
        {'Выберите организацию':
                {
                    cmd: 'organizationId',
                    paramName: 'organizationId',
                    important: true
                }
        },
        {'Название организации':
                {
                    cmd: 'inputString',
                    paramName: 'name',
                    important: true
                }
        },
        {'Connect organization id':
                {
                    cmd: 'connectOrgId',
                    paramName: 'connectOrgId',
                    important: false
                }
        },
        {'URL шаблона соглашения':
                {
                    cmd: 'inputString',
                    paramName: 'templateURL',
                    important: false
                }
        },
        {'URL картинки для превью организации на ТВ':
                {
                    cmd: 'inputString',
                    paramName: 'imageUrl',
                    important: false
                }
        },
        {'URL на страницу организации':
                {
                    cmd: 'inputString',
                    paramName: 'infoUrl',
                    important: false
                }
        },
        {'Заголовок иконки информации об организации':
                {
                    cmd: 'inputString',
                    paramName: 'infoTitle',
                    important: false
                }
        },
        {'Подзаголовок иконки информации об организации':
                {
                    cmd: 'inputString',
                    paramName: 'infoSubtitle',
                    important: false
                }
        },
        {'Использовать фичу комнат':
                {
                    cmd: ['Да', 'Нет'],
                    paramName: 'usesRooms',
                    important: false
                }
        },
        {'Максимальная громкость':
                {
                    cmd: ['0','1','2','3','4','5','6','7','8','9','10'],
                    paramName: 'maxStationVolume',
                    important: false
                }
        },
    ],
    "organization:create": [
        {'Название организации':
                {
                    cmd: 'inputString',
                    paramName: 'name',
                    important: true
                }
        },
        {'Connect organization id':
                {
                    cmd: 'connectOrgId',
                    paramName: 'connectOrgId',
                    important: false
                }
        },
        {'URL шаблона соглашения':
                {
                    cmd: 'inputString',
                    paramName: 'templateURL',
                    important: false
                }
        },
        {'URL картинки для превью организации на ТВ':
                {
                    cmd: 'inputString',
                    paramName: 'imageUrl',
                    important: false
                }
        },
        {'URL на страницу организации':
                {
                    cmd: 'inputString',
                    paramName: 'infoUrl',
                    important: false
                }
        },
        {'Заголовок иконки информации об организации':
                {
                    cmd: 'inputString',
                    paramName: 'infoTitle',
                    important: false
                }
        },
        {'Подзаголовок иконки информации об организации':
                {
                    cmd: 'inputString',
                    paramName: 'infoSubtitle',
                    important: false
                }
        },
        {'Использовать фичу комнат':
                {
                    cmd: ['Да', 'Нет'],
                    paramName: 'usesRooms',
                    important: false
                }
        },
    ],
    "organization:set-max-station-volume": [
        {'Выберите организацию':
                {
                    cmd: 'organizationId',
                    paramName: 'organizationId',
                    important: true
                }
        },
        {'Максимальная громкость':
                {
                    cmd: ['0','1','2','3','4','5','6','7','8','9','10'],
                    paramName: 'newMaxStationVolume',
                    important: false
                }
        },
    ],
    "promocode:add":[
        {'Выберите организацию':
                {
                    cmd: 'organizationId',
                    paramName: 'organizationId',
                    important: true
                }
        },
        {'Ticket key':
                {
                    cmd: 'inputString',
                    paramName: 'ticketKey',
                    important: true
                }
        },
        {'Введите промокоды, разделяя их ";", "\\n", "\\s" или ";"':
                {
                    cmd: 'inputString',
                    paramName: 'codes',
                    important: true
                }
        },
    ],
    "promocode:add-to-organization":[
        {'Выберите организацию':
                {
                    cmd: 'organizationId',
                    paramName: 'organizationId',
                    important: true
                }
        },
        {'Количество промокодов':
                {
                    cmd: 'inputNumber',
                    paramName: 'codesNum',
                    important: true
                }
        },
    ],
    "room:create": [
        {'Выберите организацию':
                {
                    cmd: 'organizationId',
                    paramName: 'organizationId',
                    important: true
                }
        },
        {'Название комнаты':
                {
                    cmd: 'inputString',
                    paramName: 'name',
                    important: true
                }
        },
        {'Room External Id':
                {
                    cmd: 'inputString',
                    paramName: 'externalRoomId',
                    important: false
                }
        },
        {'Устройства в комнате':
                {
                    cmd: 'orgDeviceIdsCheckbox',
                    paramName: 'orgDeviceIdsCheckbox',
                    important: false
                }
        },
    ],
    "users:create": [
        {'Введите логин':
                {
                    cmd: 'inputString',
                    paramName: 'login',
                    important: true
                }
        },
        {'Выберите организации':
                {
                    cmd: 'organizationIdsCheckbox',
                    paramName: 'organizationIdsCheckbox',
                    important: true
                }
        },    ],
    "users:bind": [
        {'Выберите пользователей':
                {
                    cmd: 'userIdsCheckbox',
                    paramName: 'userIdsCheckbox',
                    important: true
                }
        },
        {'Выберите организации':
                {
                    cmd: 'organizationIdsCheckbox',
                    paramName: 'organizationIdsCheckbox',
                    important: true
                }
        },
    ]
}

export default {
    scenarios,
    operations
}
