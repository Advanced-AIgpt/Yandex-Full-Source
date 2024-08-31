import anyTest, { TestInterface } from 'ava';
import sinon from 'sinon';
import {
    callConsoleApi,
    createCredentials,
    responds,
    respondsWithError,
} from '../../helpers/api';
import * as bulbasaurUtils from '../../../services/bulbasaur/utils';
import { wipeDatabase } from '../../helpers/db';

const routeMethod = 'get';
const route = '/devices/my';

interface Context {
    bulbasaurStub: sinon.SinonStub;
}

const test = anyTest as TestInterface<Context>;

test.beforeEach(async (t) => {
    await wipeDatabase();
    await createCredentials();
    t.context.bulbasaurStub = sinon
        .stub(bulbasaurUtils, 'bulbasaurGot')
        .rejects('unexpected route');
});

test.afterEach.always(() => {
    sinon.restore();
});

test('bulbasaur сломан', async (t) => {
    t.context.bulbasaurStub.withArgs('/v1.0/user/info').rejects('broken');

    respondsWithError({
        t,
        res: await callConsoleApi(routeMethod, route),
        code: 500,
        message: 'Unexpected error',
    });

    t.true(t.context.bulbasaurStub.calledOnce);
});

test('bulbasaur вернул пустой список', async (t) => {
    t.context.bulbasaurStub.withArgs('/v1.0/user/info').resolves({
        body: {
            status: 'ok',
            payload: {
                devices: [],
                rooms: [],
                scenarios: [],
            },
        },
    });

    responds({
        t,
        res: await callConsoleApi(routeMethod, route),
        expect: [],
    });

    t.true(t.context.bulbasaurStub.calledOnce);
});

test('bulbasaur не вернул станций', async (t) => {
    t.context.bulbasaurStub.withArgs('/v1.0/user/info').resolves({
        body: {
            status: 'ok',
            payload: {
                devices: [
                    {
                        type: 'devices.types.lamp',
                    },
                    {
                        // не хватает quasar_info
                        type: 'devices.types.smart_speaker.yandex.station',
                    },
                    {
                        // такие колонки мы не обслуживаем
                        type: 'devices.types.smart_speaker.yandex.station.elari',
                        quasar_info: {
                            platform: 'elarimini',
                        },
                    },
                ],
                rooms: [],
                scenarios: [],
            },
        },
    });

    responds({
        t,
        res: await callConsoleApi(routeMethod, route),
        expect: [],
    });

    t.true(t.context.bulbasaurStub.calledOnce);
});

test('успешный сценарий', async (t) => {
    t.context.bulbasaurStub.withArgs('/v1.0/user/info').resolves({
        body: {
            status: 'ok',
            payload: {
                devices: [
                    {
                        type: 'devices.types.lamp',
                    },
                    {
                        // такие колонки мы не обслуживаем
                        type: 'devices.types.smart_speaker.yandex.station.elari',
                        quasar_info: {
                            platform: 'elarimini',
                        },
                    },
                    {
                        type: 'devices.types.smart_speaker.yandex.station',
                        quasar_info: {
                            platform: 'yandexstation',
                            device_id: 'яАппаратныйИдентификатор',
                        },
                    },
                    {
                        type: 'devices.types.smart_speaker.yandex.station.mini',
                        name: 'Моя Мини',
                        quasar_info: {
                            platform: 'yandexmini',
                            device_id: 'яУникальныйНомерПроцессора',
                        },
                    },
                ],
                rooms: [],
                scenarios: [],
            },
        },
    });

    responds({
        t,
        res: await callConsoleApi(routeMethod, route),
        expect: [
            {
                deviceId: 'яАппаратныйИдентификатор',
                platform: 'yandexstation',
            },
            {
                deviceId: 'яУникальныйНомерПроцессора',
                platform: 'yandexmini',
                note: 'Моя Мини',
            },
        ],
    });

    t.true(t.context.bulbasaurStub.calledOnce);
});
