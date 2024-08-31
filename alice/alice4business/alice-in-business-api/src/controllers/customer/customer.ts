import { asyncJsonResponse, notFound } from '../utils';
import { ActivationCode, ActivationLink, Device, Operation, Room, sequelize } from '../../db';
import { resetDeviceImplement } from '../../lib/implements/reset';
import {
    serializeOperationScope,
    serializeAnonimousOperationScope,
} from '../../lib/utils';
import RestApiError from '../../lib/errors/restApi';
import {
    Status as OperationStatus,
    Type as OperationType,
} from '../../db/tables/operation';
import { isEmptyResultError } from '../../lib/errors/utils';
import config from '../../lib/config';
import { activateDevice4CustomerImplement } from '../../lib/implements/activate4customer';
import {
    activatePromoCodeForDevice,
    activatePromoCodeForUser,
} from '../../lib/implements/promo-activate';
import log from '../../lib/log';
import { activateImplement } from '../../lib/implements/activate';
import { Op } from 'sequelize';
import { DeviceInstance, Platform, Status } from '../../db/tables/device';
import { Request } from 'express';
import { ActivationLinkInstance } from '../../db/tables/activationLink';
import * as solomon from '../../services/solomon';

export const getDevicesHandler = asyncJsonResponse(
    async (req) => {
        const devices = await Device.findAll({
            attributes: ['deviceId', 'platform'],
            include: [
                {
                    required: false,
                    model: Operation,
                    attributes: ['id'],
                    where: {
                        type: OperationType.Reset,
                        status: OperationStatus.Pending,
                    },
                },
            ],
            where: {
                ownerId: req.user.uid.toString(10),
            },
        });

        return devices.map((device) => ({
            deviceId: device.deviceId,
            platform: device.platform,
            pendingOperation: device.operations?.[0]?.id,
        }));
    },
    {
        wrapResult: true,
    },
);

export const resetDeviceHandler = asyncJsonResponse(
    async (req) => {
        const { deviceId, platform } = req.body;

        if (
            !(
                deviceId &&
                typeof deviceId === 'string' &&
                platform &&
                typeof platform === 'string'
            )
        ) {
            throw new RestApiError('deviceId and platforms parameters are required', 400);
        }

        const device = (await Device.findOne({
            where: {
                deviceId,
                platform,
                ownerId: req.user.uid.toString(10),
            },
            include: [{ model: Room, required: false }],
            rejectOnEmpty: true,
        }).catch(notFound('Device')))!;

        return await resetDeviceImplement(
            device,
            req.ip,
            req.headers,
            await serializeOperationScope(req, 'customer'),
        );
    },
    {
        wrapResult: true,
    },
);

const getDeviceByActivationCode = async (code: any, invalidCodeErrorText?: string) => {
    if (!(code && typeof code === 'string')) {
        throw new RestApiError('Activation code is required', 400, {
            payload: {
                text: invalidCodeErrorText ?? 'Код введён с ошибкой. Повторите попытку',
            },
        });
    }

    const activationCode = (await ActivationCode.findOne({
        where: {
            code,
        },
        include: [{ model: Device, required: true, include: [{ model: Room, required: false }] }],
        rejectOnEmpty: true,
    }).catch((error: any) => {
        if (isEmptyResultError(error)) {
            throw new RestApiError('Bad activation code', 422, {
                payload: {
                    text:
                        invalidCodeErrorText ?? 'Код введён с ошибкой. Повторите попытку',
                },
            });
        }
        throw error;
    }))!;

    if (
        activationCode.createdAt.getTime() +
        config.app.customerActivationOperation.codeTTL <
        Date.now()
    ) {
        await activationCode.destroy().catch(() => { });
        throw new RestApiError('Activation code expired', 422, {
            payload: {
                text: 'Код устарел. Используйте другой код',
            },
        });
    }
    if (activationCode.device!.room) {
        await activationCode.device!.room!.reload({ include: [{ model: Device }] });
    }
    return [activationCode.device!, activationCode] as const;
};

export const getDeviceByCodeHandler = asyncJsonResponse(
    async (req) => {
        const { code } = req.params;

        const [device] = await getDeviceByActivationCode(code);

        return {
            note: device.note,
            deviceId: device.deviceId,
            platform: device.platform,
            pendingOperation: device.operations?.[0]?.id,
        };
    },
    { wrapResult: true },
);

const getActivation = async (id: string): Promise<ActivationLinkInstance> => {
    const now = new Date();
        const activation = await ActivationLink.findOne({
            where: {
                id,
                enabled: true,
                activeSince: { [Op.lt]: now },
                activeTill: { [Op.gt]: now },
            },
            include: [
                {
                    model: Room,
                    include: [
                        {
                            model: Device
                        }
                    ]
                },
                {
                    model: Device
                }
            ],
            rejectOnEmpty: true,
        }).catch(notFound('activation'));
        const devices = activation.device ? [activation.device!] : activation.room?.devices || [];
        if (devices.length === 0) {
            throw new RestApiError('activation not found', 404);
        }
        return activation;
}

export const getDevicesByActivationIdHandler = asyncJsonResponse(
    async (req) => {
        const { id } = req.params;
        const activation = await getActivation(id);
        const devices = activation.device ? [activation.device!] : activation.room?.devices || [];
        return {
            room: activation.room?.name || null,
            totalDevices: devices.length,
            devices: devices
                .filter(device => req.user || device.status !== Status.Active)
                .map(device => ({
                    note: device.note,
                    deviceId: device.deviceId,
                    platform: device.platform,
                })),
        };
    },
    { wrapResult: true },
);

export const applyPromocodeForUser = asyncJsonResponse(
    async (req) => {
        const user = req.user;

        if (await user.hasPlus) {
            throw new RestApiError('User already in plus', 403, {
                payload: { text: 'Вы уже в плюсе! Активация не требуется' },
            });
        }

        const device = (await Device.findOne({
            where: {
                ownerId: user.uid.toString(10),
            },
            rejectOnEmpty: true,
        }).catch((error) => {
            if (isEmptyResultError(error)) {
                throw new RestApiError(`Device not found`, 404, {
                    payload: { text: 'Упс. Кажется вы не еще активировали устройство' },
                });
            }

            throw error;
        }))!;

        const scope = await serializeOperationScope(req, 'customer');

        await activatePromoCodeForUser(device, req.ip, scope, user);

        return 'ok';
    },
    {
        wrapResult: true,
    },
);

const activateGuestByCode = async (code: string, req: Request): Promise<[string, string]> => {
    const [device, activationCode] = await getDeviceByActivationCode(code);
    const entityToActivate = device.room ? device.room! : device;
    log.debug('Guest activation start')
    const [, operationId] = await Promise.all([
        activationCode.destroy().catch(() => { }),
        activateImplement(entityToActivate, serializeAnonimousOperationScope(req)),
    ]);

    await device.reload();

    // Set hardcoded ip address for guest activations https://st.yandex-team.ru/PASKILLS-8269
    const static_ip = '93.158.190.208';
    log.debug(`Guest activation continue: ${operationId}`)

    await activatePromoCodeForDevice(
        device,
        static_ip,
        serializeAnonimousOperationScope(req),
        true,
    ).catch((error) => {
        log.warn('Failed to apply promocode after activation', { error });
    });

    log.debug(`Guest activation finish: ${operationId}`)
    return [operationId, device.kolonkishId || ''];
}

const activateCustomerByCode = async (code: string, req: Request): Promise<string> => {
    const ts = Date.now();
    const [device, activationCode] = await getDeviceByActivationCode(
        code,
        'Код активации устарел. Пожалуйста, обновите страницу и попробуйте еще раз.',
    );
    solomon.addSwitchMetric('get_device_timing', Date.now() - ts);
    const entityToActivate = device.room ? device.room! : device;
    const [, operationId] = await Promise.all([
        activationCode.destroy().catch(() => { }),
        activateDevice4CustomerImplement(
            entityToActivate,
            req.user,
            req.headers,
            await serializeOperationScope(req, 'customer'),
        ),
    ]);
    return operationId;
}

const activateGuestById = async (id: string, req: Request): Promise<[string, string]> => {

    const activation = await getActivation(id);
    const devices = (activation.device ? [activation.device!] : activation.room?.devices || []).filter(device => device.status !== Status.Active);
    const entityToActivate = activation.device || activation.room;
    if (devices.length === 0 || !entityToActivate) {
        throw new RestApiError('activation not found', 404);
    }
    const operationId = await activateImplement(entityToActivate, serializeAnonimousOperationScope(req));
    const firstDevice =  devices[0];
    if (!activation.promoActivated) {
        await firstDevice.reload();
        await activatePromoCodeForDevice(
            firstDevice,
            req.ip,
            serializeAnonimousOperationScope(req),
            true,
        ).catch((error) => {
            log.warn('Failed to apply promocode after activation', { error });
            return [operationId, activation.device?.kolonkishId || ''];
        });
        await activation.update({
            promoActivated: true
        }).catch((error) => {
            log.warn('Failed to mark promocode activation as completed', { error });
            return [operationId, activation.device?.kolonkishId || ''];
        });
    }

    return [operationId, activation.device?.kolonkishId || ''];
}

const activateCustomerById = async (id: string, req: Request): Promise<string> => {
    const now = new Date();
    const activation = await ActivationLink.findOne({
        where: {
            id,
            enabled: true,
            activeSince: { [Op.lt]: now },
            activeTill: { [Op.gt]: now },
        },
        include: [
            {
                model: Room,
                include: [
                    {
                        model: Device
                    }
                ]
            },
            {
                model: Device
            }
        ],
        rejectOnEmpty: true,
    }).catch(notFound('activation'));

    const entityToActivate = activation.device || activation.room;
    if (!entityToActivate) {
        throw new RestApiError('activation not found', 404);
    }
    const operationId = await activateDevice4CustomerImplement(
            entityToActivate,
            req.user,
            req.headers,
            await serializeOperationScope(req, 'customer'),
        );
    return operationId;
}


export const activateForGuestHandler = asyncJsonResponse(
    async (req) => {
        const { code, activationId } = req.body;
        if (code && activationId) {
            throw new RestApiError('Single activation method expected', 400);
        }
        if (code) {
            return await activateGuestByCode(code, req);
        }
        if (activationId) {
            return await activateGuestById(activationId, req);
        }
        throw new RestApiError('Single activation method expected', 400);

    },
    {
        wrapResult: true,
    },
);

export const activateForCustomerHandler = asyncJsonResponse(
    async (req) => {
        const { code, activationId } = req.body;

        if (code && activationId) {
            throw new RestApiError('Single activation method expected', 400);
        }
        if (code) {
            return await activateCustomerByCode(code, req);
        }
        if (activationId) {
            return await activateCustomerById(activationId, req);
        }
        throw new RestApiError('Single activation method expected', 400);
    },
    {
        wrapResult: true,
    },
);
