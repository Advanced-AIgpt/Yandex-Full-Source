import RestApiError from '../../lib/errors/restApi';
import { asyncJsonResponse, notFound } from '../utils';
import { activateImplement } from '../../lib/implements/activate';
import { resetDeviceImplement } from '../../lib/implements/reset';
import { syncDevice } from '../../lib/sync/quasar';
import { activatePromoCodeForDevice } from '../../lib/implements/promo-activate';
import { serializeOperationScope } from '../../lib/utils';
import * as ACL from '../../lib/acl';
import log from '../../lib/log';
import { DeviceInstance } from '../../db/tables/device';
import { Operation } from '../../db';
import { Status as OperationStatus } from '../../db/tables/operation';
import { createActivation, resetActivationLinks } from './activations';
import { getUserHasPlus } from '../../services/blackbox';

export const activateDeviceHandler = asyncJsonResponse(
    async (req, res) => {
        log.info('public/device/activate body params:', {
            body: {
                external_id: req.body.external_id,
                with_promocode: req.body.with_promocode,
            },
        });

        const externalDeviceId = req.body.external_id;
        if (!externalDeviceId || typeof externalDeviceId !== 'string') {
            throw new RestApiError('external_id parameter is required', 400);
        }
        const applyPromoCode = req.body.with_promocode === true;

        const device = await ACL.getDevice(
            req.user,
            applyPromoCode ? ['status', 'promocode'] : ['status'],
            { externalDeviceId },
        ).catch(notFound('Device'));
        const operationId = await activateImplement(
            device,
            await serializeOperationScope(req, 'ext'),
        );

        if (applyPromoCode) {
            setImmediate(async () => {
                await device.reload();
                await activatePromoCodeForDevice(
                    device,
                    req.ip,
                    await serializeOperationScope(req, 'ext'),
                    true,
                ).catch((error) => {
                    log.warn('Failed to apply promocode after activation', { error });
                });
            });
        }

        return { status: 'ok', operationId };
    },
    {
        putPostStatus: 200,
    },
);

export const resetDeviceHandler = asyncJsonResponse(async (req, res) => {
    const externalDeviceId = req.body.external_id;
    if (!externalDeviceId || typeof externalDeviceId !== 'string') {
        throw new RestApiError('external_id parameter is required', 400);
    }
    const device = await ACL.getDevice(req.user, ['status'], { externalDeviceId }).catch(
        notFound('Device'),
    );

    const operationId = await resetDeviceImplement(
        device,
        req.ip,
        req.headers,
        await serializeOperationScope(req, 'ext'),
    );
    setImmediate(async () => resetActivationLinks(device));
    return { status: 'ok', operationId };
});

export const getDeviceInfoHandler = asyncJsonResponse(async (req, res) => {
    const deviceId = req.query.device_id;
    const externalDeviceId = req.query.external_id;
    let device: DeviceInstance;

    if (deviceId && typeof deviceId === 'string') {
        device = await ACL.getDevice(req.user, [], { deviceId }, {
            include: [
                {
                    required: false,
                    model: Operation,
                    attributes: ['id'],
                    where: {
                        status: OperationStatus.Pending,
                    },
                },
            ]
        }).catch(
            notFound('Device'),
        );
    } else if (externalDeviceId && typeof externalDeviceId === 'string') {
        device = await ACL.getDevice(req.user, [], { externalDeviceId }, {
            include: [
                {
                    required: false,
                    model: Operation,
                    attributes: ['id'],
                    where: {
                        status: OperationStatus.Pending,
                    },
                },
            ]
        }).catch(
            notFound('Device'),
        );
    } else {
        throw new RestApiError('device_id or external_id parameter is required', 400);
    }

    await syncDevice(device);
    const hasPlus = device.ownerId != null ? await getUserHasPlus(device.ownerId, req.ip) : null

    return {
        status: 'ok',
        external_id: device.externalDeviceId,
        info: {
            external_id: device.externalDeviceId,
            online: device.online,
            status: device.status,
            device_id: device.deviceId,
            note: device.note,
            in_progress: device.operations ? device.operations.length > 0 : false,
            has_plus: hasPlus,
            has_custom_user: device.ownerId != null && device.kolonkishId !== device.ownerId
        },
    };
});

export const activatePromoCodeHandler = asyncJsonResponse(async (req, res) => {
    const externalDeviceId = req.body.external_id;
    if (!externalDeviceId || typeof externalDeviceId !== 'string') {
        throw new RestApiError('external_id parameter is required', 400);
    }
    const device = await ACL.getDevice(req.user, ['promocode'], {
        externalDeviceId,
    }).catch(notFound('Device'));
    await syncDevice(device);

    await activatePromoCodeForDevice(
        device,
        req.ip,
        await serializeOperationScope(req, 'ext'),
        true,
    );
    return { status: 'ok' };
});

export const createDeviceActivationHandler = asyncJsonResponse(
    async (req, res) => {
        const externalDeviceId = req.body.external_id;
        if (!externalDeviceId || typeof externalDeviceId !== 'string') {
            throw new RestApiError('external_id parameter is required', 400);
        }
        const firstActivationDate = new Date(req.body.first_activation_date);
        const lastActivationDate = new Date(req.body.last_activation_date);
        const device = await ACL.getDevice(req.user, ['status'], { externalDeviceId })
            .catch(notFound('Device'));
        const activation = await createActivation(device, firstActivationDate, lastActivationDate)
        return {
            status: 'ok',
            id: activation.id,
        }
    }
);