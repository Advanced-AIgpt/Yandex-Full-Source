import RestApiError from '../../lib/errors/restApi';
import {
    isEmptyResultError,
    isSequelizeValidationError,
    renameValidationErrorFields,
} from '../../lib/errors/utils';
import { asyncJsonResponse, notFound } from '../utils';
import { Device, Operation, PromoCode, Room } from '../../db';
import { activateImplement } from '../../lib/implements/activate';
import { activatePromoCodeForDevice } from '../../lib/implements/promo-activate';
import { resetDeviceImplement } from '../../lib/implements/reset';
import { serializeOperationScope } from '../../lib/utils';
import { syncDevice, syncDeviceList } from '../../lib/sync/quasar';
import * as ACL from '../../lib/acl';
import { PlusResponseStatus } from '../../services/media/ya-plus';
import express from 'express';
import { getAgreement } from '../../lib/implements/agreement';
import log from '../../lib/log';
import { getUserDevices } from '../../services/bulbasaur/device-list';
import { sendPush } from '../../services/push/send';
import { Status as OperationStatus } from '../../db/tables/operation';
import { DeviceInstance, DeviceSchema, Platform, Status } from '../../db/tables/device';
import { resetActivationLinks } from '../public/activations';

const throwRestApiError = (error: Error) => {
    if (isSequelizeValidationError(error)) {
        renameValidationErrorFields(error, {
            organization_id: 'organizationId',
            device_id: 'deviceId',
            external_device_id: 'externalDeviceId',
            kolonkish_id: 'kolonkishId',
        });
    }

    throw RestApiError.fromError(error);
};

const _extractDeviceInfo = (device: DeviceInstance) => ({
    id: device.id,
    organizationId: device.organizationId,
    platform: device.platform,
    deviceId: device.deviceId,
    externalDeviceId: device.externalDeviceId,
    note: device.note,
    status: device.status,
    online: device.online,
    agreementAccepted: device.agreementAccepted,
    isActivatedByCustomer:
        device.status === Status.Active && device.kolonkishId !== device.ownerId,
    hasPromo: Boolean(device.promoCodes?.length),
    pendingOperation: device.operations?.[0]?.id,
    room: device.room ? { id: device!.room.id, name: device!.room.name } : null
});

export const getDeviceList = asyncJsonResponse(
    async (req, res) => {
        const { organizationId } = req.params;
        const { roomId } = req.query;
        const list = await ACL.getOrganizationDevices(req.user, [], organizationId, roomId, {
            include: [
                {
                    required: false,
                    model: PromoCode, // TODO: разобрать
                    attributes: ['id'],
                    where: {
                        status: PlusResponseStatus.Success,
                    },
                },
                {
                    required: false,
                    model: Operation,
                    attributes: ['id'],
                    where: {
                        status: OperationStatus.Pending,
                    },
                },
                {
                    required: false,
                    model: Room,
                },
            ],
        }).catch(notFound('Organization'));
        setTimeout(async () => {
            await syncDeviceList(list);
        }, 0);
        return list.map(_extractDeviceInfo);
    },
    { wrapResult: true },
);

export const getDeviceHandler = asyncJsonResponse(
    async (req) => {
        const { id } = req.params;
        const device = await ACL.getDevice(req.user, [], { id }, { include: ['room'] }).catch(
            notFound('Device'),
        );
        await syncDevice(device);
        await device.reload({
            include: [
                {
                    required: false,
                    model: PromoCode, // TODO: разобрать
                    attributes: ['id'],
                    where: {
                        status: PlusResponseStatus.Success,
                    },
                },
                {
                    required: false,
                    model: Operation,
                    attributes: ['id'],
                    where: {
                        status: OperationStatus.Pending,
                    },
                },
            ],
        });

        return _extractDeviceInfo(device);
    },
    {
        wrapResult: true,
    },
);

export const createDeviceHandler = asyncJsonResponse(
    async (req, res) => {
        try {
            const { organizationId } = req.body;
            await ACL.getOrganization(req.user, ['edit'], { id: organizationId }).catch(
                notFound('Organization'),
            );


            const fields = req.body.device as Pick<
                DeviceSchema,
                'platform' | 'deviceId' | 'externalDeviceId' | 'note' | 'roomId'
            >;
            try {
                fields.platform = fields.platform.replace(/\s+/g, '') as Platform;
                if (!fields.platform) {
                    delete fields.platform;
                }

                fields.deviceId = fields.deviceId.replace(/\s+/g, '');
                if (!fields.deviceId) {
                    delete fields.deviceId;
                }

                fields.externalDeviceId = (fields.externalDeviceId || '').trim();
                if (!fields.externalDeviceId) {
                    delete fields.externalDeviceId;
                }

                fields.note = (fields.note || '').trim();
                if (!fields.note) {
                    delete fields.note;
                }

                fields.roomId = (fields.roomId || '').trim();
                if (!fields.roomId) {
                    delete fields.roomId;
                }
            } catch (origError) {
                throw new RestApiError('Invalid req.body.device format', 400, {
                    origError,
                });
            }

            const device = await Device.create({
                ...fields,
                organizationId,
            } as DeviceSchema);

            const pendingOperation = await resetDeviceImplement(
                device,
                req.ip,
                req.headers,
                await serializeOperationScope(req, 'int'),
            ).catch((error) =>
                log.warn('Failed to reset device after create', { error }),
            );

            await device.reload({ include: ['room'] });

            return {
                ..._extractDeviceInfo(device),
                pendingOperation,
            };
        } catch (error) {
            return throwRestApiError(error);
        }
    },
    { wrapResult: true },
);

export const editDeviceNote = asyncJsonResponse(
    async (req, res) => {
        const { note } = req.body;
        const { id } = req.params;
        try {
            const device = await ACL.getDevice(req.user, ['edit'], { id }).catch(
                notFound('Device'),
            );
            await device.update({ note: note?.trim() || null } as DeviceSchema);
            sendPush({
                topic: device.organizationId,
                event: 'device-state',
                payload: device.id,
            });
            return { status: 'ok' };
        } catch (error) {
            return throwRestApiError(error);
        }
    },
    { wrapResult: true },
);

export const editDeviceExternalDeviceId = asyncJsonResponse(
    async (req, res) => {
        const { externalDeviceId } = req.body;
        const { id } = req.params;
        try {
            const device = await ACL.getDevice(req.user, ['edit'], { id }).catch(
                notFound('Device'),
            );
            await device.update({
                externalDeviceId: externalDeviceId?.trim() || null,
            } as DeviceSchema);
            sendPush({
                topic: device.organizationId,
                event: 'device-state',
                payload: device.id,
            });
            return { status: 'ok' };
        } catch (error) {
            return throwRestApiError(error);
        }
    },
    { wrapResult: true },
);

export const editDeviceRoomId = asyncJsonResponse(
    async (req, res) => {
        const { roomId } = req.body;
        const { id } = req.params;
        try {
            const device = await ACL.getDevice(req.user, ['edit'], { id }).catch(
                notFound('Device'),
            );
            await device.update({
                roomId: roomId?.trim() || null,
            } as DeviceSchema);
            sendPush({
                topic: device.organizationId,
                event: 'device-state',
                payload: device.id,
            });
            return { status: 'ok' };
        } catch (error) {
            return throwRestApiError(error);
        }
    },
    { wrapResult: true },
);

export const deleteDeviceHandler = asyncJsonResponse(async (req, res) => {
    const { id } = req.params;
    try {
        const device = await ACL.getDevice(req.user, ['edit'], { id });
        await device.destroy();
        sendPush({
            topic: device.organizationId,
            event: 'device-state',
            payload: device.id,
        });
        return { status: 'ok' };
    } catch (error) {
        if (isEmptyResultError(error)) {
            return { status: 'ok' };
        }
        throw error;
    }
});

export const activateDeviceHandler = asyncJsonResponse(
    async (req, res) => {
        const { id } = req.params;
        const device = await ACL.getDevice(req.user, ['status'], { id }).catch(
            notFound('Device'),
        );
        const operationId = await activateImplement(
            device,
            await serializeOperationScope(req, 'int'),
        );
        return { status: 'ok', operationId };
    },
    { wrapResult: true },
);

export const resetDeviceHandler = asyncJsonResponse(
    async (req, res) => {
        const { id } = req.params;
        const device = await ACL.getDevice(req.user, ['status'], { id }, {
            include: [{ model: Room, required: false }]
        }).catch(
            notFound('Device'),
        );
        const operationId = await resetDeviceImplement(
            device,
            req.ip,
            req.headers,
            await serializeOperationScope(req, 'int'),
        );
        setImmediate(async () => resetActivationLinks(device));
        return { status: 'ok', operationId };
    },
    { wrapResult: true },
);

export const getDeviceAgreement: express.RequestHandler = (req, res, next) => {
    (async () => {
        const { id } = req.params;
        const device = await ACL.getDevice((req as express.Request).user, [], {
            id,
        }).catch(notFound('Device'));

        const agreementBuffer = await getAgreement(device);
        const headers = {
            'Content-Type': 'application/pdf',
            'Content-Disposition': `inline; filename=agreement-${device.kolonkishLogin}.pdf`,
        };
        res.header(headers).send(agreementBuffer);
    })().catch(next);
};

export const activatePromoCodeHandler = asyncJsonResponse(async (req, res) => {
    const { id } = req.params;
    const device = await ACL.getDevice(req.user, ['promocode'], { id }).catch(
        notFound('Device'),
    );
    await syncDevice(device);
    await activatePromoCodeForDevice(
        device,
        req.ip,
        await serializeOperationScope(req, 'int'),
    );
    return { status: 'ok' };
});

export const getMyDeviceList = asyncJsonResponse(
    (req) => getUserDevices(req.header('X-Ya-User-Ticket')!),
    {
        wrapResult: true,
    },
);
