import test from 'ava';
import sinon from 'sinon';
import { createDevice, wipeDatabase } from '../../helpers/db';
import {
    callPublicApi,
    createCredentials,
    responds,
    respondsWithError,
} from '../../helpers/api';
import * as activateImplement from '../../../lib/implements/activate';
import data from '../../helpers/data';

test.beforeEach(async (t) => {
    await wipeDatabase();
    await createCredentials();
});
test.afterEach.always(async (t) => {
    sinon.restore();
});

test('Возвращает id операции', async (t) => {
    const operationId = data.uniqueString();
    sinon.stub(activateImplement, 'activateImplement').resolves(operationId);
    const { externalDeviceId } = await createDevice();
    const res = await callPublicApi('post', '/devices/activate', {
        body: { external_id: externalDeviceId },
    });
    responds({
        wrapResult: false,
        expect: { operationId, status: 'ok' },
        code: 200,
        res,
        t,
    });
});

test('Не передан external_id', async (t) => {
    const res = await callPublicApi('post', '/devices/activate');
    respondsWithError({
        code: 400,
        message: 'external_id parameter is required',
        res,
        t,
    });
});

test('Невалидный external_id', async (t) => {
    const res = await callPublicApi('post', '/devices/activate', {
        body: { external_id: data.uniqueString() },
    });
    respondsWithError({
        code: 404,
        message: 'Device not found',
        res,
        t,
    });
});
