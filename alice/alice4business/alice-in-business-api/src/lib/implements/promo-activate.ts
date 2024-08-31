import { Op } from 'sequelize';
import RestApiError from '../errors/restApi';
import { sequelize, Operation, PromoCode, Device } from '../../db';
import { DeviceInstance, Status } from '../../db/tables/device';
import {
    OperationSchema,
    Scope as OperationScope,
    Status as OperationStatus,
    Type as OperationType,
} from '../../db/tables/operation';
import config from '../config';
import log from '../log';
import { plusRequest, PlusResponseStatus } from '../../services/media/ya-plus';
import * as solomon from '../../services/solomon';
import { sleep } from '../utils';
import { serializeError } from 'serialize-error';
import { trimPrivateFields } from '../errors/utils';
import { notifyStateChange, sendPush } from '../../services/push/send';
import { PromoCodeInstance, PromoCodeSchema } from '../../db/tables/promoCode';
import { User } from '../../lib/acl';
import { RoomInstance } from '../../db/tables/room';
import { createOperation } from './operations';

const tryActivate = async (code: string, clientIp: string, userId: string) => {
    const res = await plusRequest({
        promoCode: code,
        clientIp,
        userId,
    });
    log.debug(`promocode activating request: ${res.result.status}`, {
        mediaResponse: res,
    });
    if (res.result.status !== PlusResponseStatus.Success) {
        log.error(`promocode activating request (rejected): ${res.result.status}`, {
            mediaResponse: res,
        });
    }
    return res.result.status;
};

const ensureActivatePromo = async (code: string, clientIp: string, userId: string) => {
    let tryCounter = 0;
    while (tryCounter++ < config.mediabilling.retries) {
        const status = await tryActivate(code, clientIp, userId);
        if (status !== PlusResponseStatus.FailedToCreatePayment) {
            return status;
        }
        await sleep(200);
    }
    return;
};

export const activatePromoCodeForUser = async (
    device: DeviceInstance,
    clientIp: string,
    scope: OperationScope,
    user: User,
) => {
    const operation = await Operation.create({
        type: OperationType.PromoActivate,
        status: OperationStatus.Resolved,
        payload: {
            uid: String(user.uid),
            login: await user.login,
        },
        devicePk: device.id,
        scope,
    } as OperationSchema);

    try {
        if (device.status !== Status.Active) {
            throw new RestApiError('Device is not active', 400, {
                payload: { text: 'Активируйте устройство, чтобы применить промокод' },
            });
        }

        await activatePromoCodeImpl({
            userId: String(user.uid),
            clientIp,
            organizationId: device.organizationId,
            operationId: operation.id,
        });
        sendPush({
            topic: device.organizationId,
            event: 'device-state',
            payload: device.id,
        });
        sendPush({
            topic: device.organizationId,
            event: 'organization-info',
            payload: null,
        });
    } catch (e) {
        await operation.update({
            status: OperationStatus.Rejected,
            error: trimPrivateFields(serializeError(e)),
        } as OperationSchema);
        log.error('promocode activating error', { operation, e });
        throw e;
    }
};

export const activatePromoCodeForRoom = async (
    room: RoomInstance,
    clientIp: string,
    scope: OperationScope
) => {
    const operation = await Operation.create({
        type: OperationType.PromoActivate,
        status: OperationStatus.Resolved,
        roomPk: room.id,
        scope,
    } as OperationSchema);
    if (!room.devices) {
        await room.reload({ include: [{ model: Device }] });
    }
    try {
        let kolonkish: string | undefined;
        room.devices!.filter(d => d.status === Status.Active).forEach(device => {
            if (!device.kolonkishId) {
                throw new RestApiError('One of room\'s devices is activated to personal account', 400, {
                    payload: {
                        text:
                            'Одно из устройств было активировано на персональный аккаунт посетителя',
                    },
                });
            }
            if (device.status !== Status.Active) {
                throw new RestApiError('Device is not active', 400, {
                    payload: { text: 'Активируйте все устройства комнаты, чтобы применить промокод' },
                });
            }
            if (!kolonkish) {
                kolonkish = device.kolonkishId;
            } else {
                if (kolonkish !== device.kolonkishId) {
                    throw new RestApiError('Different accounts for room', 400, {
                        payload: { text: 'Устройства комнаты активированы на разные аккаунты' },
                    });
                }
            }
        });
        if (!kolonkish) {
            throw new RestApiError('No kolonkish for room found', 400);
        }
        await activatePromoCodeImpl({
            userId: kolonkish,
            clientIp,
            organizationId: room.organizationId,
            operationId: operation.id,
        });
        notifyStateChange(room);
        sendPush({
            topic: room.organizationId,
            event: 'organization-info',
            payload: null,
        });
    } catch (e) {
        await operation.update({
            status: OperationStatus.Rejected,
            error: trimPrivateFields(serializeError(e)),
        } as OperationSchema);
        log.error('promocode activating error', { operation, e });
        throw e;
    }
};

export const activatePromoCodeForDevice = async (
    device: DeviceInstance,
    clientIp: string,
    scope: OperationScope,
    shouldSkipAgreement: boolean = false,
) => {
    const operation = await Operation.create({
        type: OperationType.PromoActivate,
        status: OperationStatus.Resolved,
        payload: device.kolonkishId
            ? {
                kolonkishLogin: device.kolonkishLogin!,
                kolonkishUid: device.kolonkishId,
            }
            : null,
        devicePk: device.id,
        scope,
    } as OperationSchema);

    try {
        if (!device.kolonkishId) {
            throw new RestApiError('Device is activated to personal account', 400, {
                payload: {
                    text:
                        'Устройство было активировано на персональный аккаунт посетителя',
                },
            });
        }
        if (!shouldSkipAgreement && !device.agreementAccepted) {
            throw new RestApiError('Agreement is not accepted', 400, {
                payload: {
                    text: 'Для применения промо-кода требуется подписать Соглашение',
                },
            });
        }
        if (device.status !== Status.Active) {
            throw new RestApiError('Device is not active', 400, {
                payload: { text: 'Активируйте устройство, чтобы применить промокод' },
            });
        }

        await activatePromoCodeImpl({
            userId: String(device.kolonkishId),
            clientIp,
            organizationId: device.organizationId,
            operationId: operation.id,
        });
        sendPush({
            topic: device.organizationId,
            event: 'device-state',
            payload: device.id,
        });
        sendPush({
            topic: device.organizationId,
            event: 'organization-info',
            payload: null,
        });
    } catch (e) {
        await operation.update({
            status: OperationStatus.Rejected,
            error: trimPrivateFields(serializeError(e)),
        } as OperationSchema);
        log.error('promocode activating error', { operation, e });
        throw e;
    }
};

interface ActivatePromoCodeProps {
    userId: string;
    clientIp: string;
    organizationId: string;
    operationId: string;
}

export const activatePromoCodeImpl = async ({
    userId,
    clientIp,
    organizationId,
    operationId,
}: ActivatePromoCodeProps) => {
    const expiredPeriodMs = 1000 * 60 * 60 * 24 * 7; // One week
    const beenRedeemed =
        (await PromoCode.count({
            where: {
                userId,
                status: { [Op.or]: [PlusResponseStatus.Success, 'pending'] },
                updatedAt: {
                    [Op.gte]: new Date(Date.now() - expiredPeriodMs),
                },
            },
        })) > 0;
    if (beenRedeemed) {
        throw new RestApiError('Promo code has already been redeemed', 400, {
            payload: { text: 'Промокод для устройства уже активирован' },
        });
    }
    try {
        const promoCode = await sequelize.transaction<PromoCodeInstance>(
            async (transaction) => {
                const lockedPromoCode = await PromoCode.findOne({
                    where: {
                        organizationId,
                        status: null,
                        operationId: null,
                    },
                    transaction,
                    lock: transaction.LOCK.UPDATE,
                    order: ['created_at'],
                });

                if (!lockedPromoCode) {
                    throw new RestApiError('No more promo codes', 400, {
                        payload: { text: 'Промокоды закончились' },
                    });
                }
                return lockedPromoCode.update(
                    {
                        userId,
                        status: 'pending',
                        operationId,
                    } as PromoCodeSchema,
                    {
                        transaction,
                    });
            });

        const status = await ensureActivatePromo(promoCode.code, clientIp, promoCode.userId!);

        switch (status) {
            case PlusResponseStatus.Success:
                await promoCode.update({ status } as PromoCodeSchema);
                break;

            case PlusResponseStatus.UserTemporaryBanned:
            case PlusResponseStatus.UserHasTemporaryCampaignRestrictions:
            case PlusResponseStatus.CodeOnlyForNewUsers:
                // проблема не в коде, его нужно вернуть в список доступных
                await promoCode.update({
                    userId: null,
                    status: null,
                    operationId: null,
                } as PromoCodeSchema);
                throw new RestApiError('Promocode activating error', 400, {
                    payload: {
                        text: 'Ошибка во время активации промокода',
                        status,
                        promoCode: promoCode.id,
                    },
                });

            default:
                // код испорчен, так и запишем
                await promoCode.update({ status } as PromoCodeSchema);
                throw new RestApiError('Promocode activating error', 400, {
                    payload: {
                        text: 'Ошибка во время активации промокода',
                        status,
                        promoCode: promoCode.id,
                    },
                });
        }
    } catch (e) {
        solomon.incCounter(`promocode__activate_error`);
        throw e;
    }
    solomon.incCounter('promocode__activate_success');
};
