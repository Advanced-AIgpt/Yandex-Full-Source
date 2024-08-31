import test from 'ava';
import sinon from 'sinon';
import {
    createDevice,
    createOrganization,
    createPromoCode,
    getLastOperation,
    wipeDatabase,
} from '../../helpers/db';
import { activatePromoCodeForDevice } from '../../../lib/implements/promo-activate';
import data from '../../helpers/data';
import RestApiError from '../../../lib/errors/restApi';
import * as mediaService from '../../../services/media/ya-plus';
import * as Send from '../../../services/push/send';
import config from '../../../lib/config';
import {
    Status as OperationStatus,
    Type as OperationType,
} from '../../../db/tables/operation';
import { Status } from '../../../db/tables/device';

test.beforeEach(async (t) => {
    await wipeDatabase();
    await createOrganization();
});

test.afterEach.always(async (t) => {
    sinon.restore();
});

const success = data.response.plusRequest(mediaService.PlusResponseStatus.Success);
const failedToCreatePayment = data.response.plusRequest(
    mediaService.PlusResponseStatus.FailedToCreatePayment,
);

test('Промокод уже использован', async (t) => {
    const sendPush = sinon.stub(Send, 'sendPush').returns();

    const device = await createDevice({
        status: Status.Active,
        online: true,
        agreementAccepted: true,
    });
    await createPromoCode({
        id: data.uuid(),
        userId: undefined,
        status: undefined,
        code: data.uniqueString(),
    });
    const promoCode = await createPromoCode({
        id: data.uuid(),
        userId: device.kolonkishId!,
        status: mediaService.PlusResponseStatus.Success,
    });

    await t.throwsAsync(
        activatePromoCodeForDevice(device, data.user.ip, data.operation.scope!),
        {
            instanceOf: RestApiError,
            message: /has already been redeemed/,
        },
    );

    const oldPromoCodeData = promoCode.get({ plain: true, clone: true });
    await promoCode.reload();
    t.deepEqual(promoCode.get({ plain: true, clone: true }), oldPromoCodeData);

    const op = await getLastOperation(device.id);
    t.truthy(op.type === OperationType.PromoActivate);
    t.truthy(op.status === OperationStatus.Rejected);
    t.deepEqual(op.scope, data.operation.scope);

    t.is(sendPush.callCount, 0);
});

test('Промокоды закончились', async (t) => {
    const sendPush = sinon.stub(Send, 'sendPush').returns();

    const device = await createDevice({
        status: Status.Active,
        online: true,
        agreementAccepted: true,
    });
    const promoCode = await createPromoCode({
        id: data.uuid(),
        userId: data.uniqueString(),
        status: mediaService.PlusResponseStatus.CodeNotExists,
    });

    await t.throwsAsync(
        activatePromoCodeForDevice(device, data.user.ip, data.operation.scope!),
        {
            instanceOf: RestApiError,
            message: /No more promo codes/,
        },
    );

    const oldPromoCodeData = promoCode.get({ plain: true, clone: true });
    await promoCode.reload();
    t.deepEqual(promoCode.get({ plain: true, clone: true }), oldPromoCodeData);

    const op = await getLastOperation(device.id);
    t.is(op.type, OperationType.PromoActivate);
    t.is(op.status, OperationStatus.Rejected);
    t.deepEqual(op.scope, data.operation.scope);

    t.is(sendPush.callCount, 0);
});

test('Успешная активация промокода', async (t) => {
    const sendPush = sinon.stub(Send, 'sendPush').returns();

    const device = await createDevice({
        status: Status.Active,
        online: true,
        agreementAccepted: true,
    });
    sinon.stub(mediaService, 'plusRequest').resolves(success);
    const promoCode = await createPromoCode({
        userId: undefined,
        status: undefined,
    });

    await t.notThrowsAsync(
        activatePromoCodeForDevice(device, data.user.ip, data.operation.scope!),
    );

    await promoCode.reload();
    t.is(promoCode.userId, device.kolonkishId);
    t.is(promoCode.status, mediaService.PlusResponseStatus.Success);
    t.truthy(promoCode.operationId);

    const op = await promoCode.getOperation();
    t.is(op.devicePk, device.id);
    t.is(op.type, OperationType.PromoActivate);
    t.is(op.status, OperationStatus.Resolved);
    t.deepEqual(op.scope, data.operation.scope);

    t.is(sendPush.callCount, 2);
});

test('Retries на FailedToCreatePayment', async (t) => {
    const sendPush = sinon.stub(Send, 'sendPush').returns();

    const fakeFn = sinon.stub();
    fakeFn.returns(failedToCreatePayment);
    fakeFn.onCall(config.mediabilling.retries - 1).returns(success);
    sinon.stub(mediaService, 'plusRequest').value(fakeFn);

    const device = await createDevice({
        status: Status.Active,
        online: true,
        agreementAccepted: true,
    });
    const promoCode = await createPromoCode({
        userId: undefined,
        status: undefined,
    });

    await t.notThrowsAsync(
        activatePromoCodeForDevice(device, data.user.ip, data.operation.scope!),
    );

    await promoCode.reload();
    t.is(promoCode.userId, device.kolonkishId);
    t.is(promoCode.status, mediaService.PlusResponseStatus.Success);
    t.truthy(promoCode.operationId);

    const op = await promoCode.getOperation();
    t.is(op.devicePk, device.id);
    t.is(op.type, OperationType.PromoActivate);
    t.is(op.status, OperationStatus.Resolved);
    t.deepEqual(op.scope, data.operation.scope);

    t.is(sendPush.callCount, 2);
});

test('Ошибка при активации промокода до печати соглашения', async (t) => {
    const sendPush = sinon.stub(Send, 'sendPush').returns();

    sinon.stub(mediaService, 'plusRequest').resolves(success);
    const device = await createDevice({
        status: Status.Active,
        online: true,
        agreementAccepted: false,
    });
    const promoCode = await createPromoCode({
        userId: undefined,
        status: undefined,
    });

    await t.throwsAsync(
        activatePromoCodeForDevice(device, data.user.ip, data.operation.scope!),
        {
            instanceOf: RestApiError,
            message: 'Agreement is not accepted',
        },
    );

    const oldPromoCodeData = promoCode.get({ plain: true, clone: true });
    await promoCode.reload();
    t.deepEqual(promoCode.get({ plain: true, clone: true }), oldPromoCodeData);

    const op = await getLastOperation(device.id);
    t.is(op.type, OperationType.PromoActivate);
    t.is(op.status, OperationStatus.Rejected);
    t.deepEqual(op.scope, data.operation.scope);

    t.is(sendPush.callCount, 0);
});

test('Ошибка при активации промокода на сброшенное устройство', async (t) => {
    const sendPush = sinon.stub(Send, 'sendPush').returns();

    sinon.stub(mediaService, 'plusRequest').resolves(success);
    const device = await createDevice({
        status: Status.Reset,
        online: true,
        agreementAccepted: true,
    });
    const promoCode = await createPromoCode({
        userId: undefined,
        status: undefined,
    });

    await t.throwsAsync(
        activatePromoCodeForDevice(device, data.user.ip, data.operation.scope!),
        {
            instanceOf: RestApiError,
            message: 'Device is not active',
        },
    );

    const oldPromoCodeData = promoCode.get({ plain: true, clone: true });
    await promoCode.reload();
    t.deepEqual(promoCode.get({ plain: true, clone: true }), oldPromoCodeData);

    const op = await getLastOperation(device.id);
    t.is(op.type, OperationType.PromoActivate);
    t.is(op.status, OperationStatus.Rejected);
    t.deepEqual(op.scope, data.operation.scope);

    t.is(sendPush.callCount, 0);
});

test('Ошибка при активации промокода на аккаунт посетителя', async (t) => {
    const sendPush = sinon.stub(Send, 'sendPush').returns();

    sinon.stub(mediaService, 'plusRequest').resolves(success);
    const device = await createDevice({
        status: Status.Active,
        online: false,
        agreementAccepted: true,
        kolonkishId: undefined,
        kolonkishLogin: undefined,
    });
    const promoCode = await createPromoCode({
        userId: undefined,
        status: undefined,
    });

    await t.throwsAsync(
        activatePromoCodeForDevice(device, data.user.ip, data.operation.scope!),
        {
            instanceOf: RestApiError,
            message: 'Device is activated to personal account',
        },
    );

    const oldPromoCodeData = promoCode.get({ plain: true, clone: true });
    await promoCode.reload();
    t.deepEqual(promoCode.get({ plain: true, clone: true }), oldPromoCodeData);

    const op = await getLastOperation(device.id);
    t.is(op.type, OperationType.PromoActivate);
    t.is(op.status, OperationStatus.Rejected);
    t.deepEqual(op.scope, data.operation.scope);

    t.is(sendPush.callCount, 0);
});
