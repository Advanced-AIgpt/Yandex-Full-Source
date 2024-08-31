import { asyncJsonResponse, notFound } from "../utils";
import * as ACL from '../../lib/acl';
import { RoomInstance, RoomSchema } from "../../db/tables/room";
import { resetRoomImplement } from "../../lib/implements/reset";
import { serializeOperationScope } from "../../lib/utils";
import { Device, Operation, PromoCode, Room, sequelize } from "../../db";
import { Status as OperationStatus } from '../../db/tables/operation';
import { activateImplement } from "../../lib/implements/activate";
import { isEmptyResultError, isSequelizeValidationError, renameValidationErrorFields } from "../../lib/errors/utils";
import RestApiError from "../../lib/errors/restApi";
import { DeviceInstance, DeviceSchema } from "../../db/tables/device";
import { sendPush } from "../../services/push/send";
import { syncRoom } from "../../lib/sync/quasar";
import { activatePromoCodeForRoom } from "../../lib/implements/promo-activate";
import { PlusResponseStatus } from "../../services/media/ya-plus";
import { resetActivationLinks } from "../public/activations";

const throwRestApiError = (error: Error) => {
    if (isSequelizeValidationError(error)) {
        renameValidationErrorFields(error, {
            organization_id: 'organizationId',
            room_id: 'roomId',
            external_room_id: 'externalId',
        });
    }
    throw RestApiError.fromError(error);
};

function _extractRoomInfo(room: RoomInstance) {
    return {
        id: room.id,
        name: room.name,
        externalRoomId: room.externalRoomId,
        status: room.getStatus(),
        onlineStatus: room.getOnlineStatus(),
        promoStatus: room.getPromoStatus(),
        numDevices: room.devices?.length,
        pendingOperation: room.operations?.[0]?.id,
    }
}


export const getRoomHandler = asyncJsonResponse(
    async (req) => {
        const { id } = req.params;
        const room = await ACL.getRoom(req.user, [], { id },
            {
                include: [
                    {
                        model: Device
                    },
                    {
                        model: Operation,
                        required: false,
                        where: {
                            status: OperationStatus.Pending
                        }
                    }
                ]
            }
        ).catch(notFound('Room'));
        return _extractRoomInfo(room);
    },
    { wrapResult: true }
);

export const getRoomList = asyncJsonResponse(
    async (req, res) => {
        const { organizationId } = req.params;
        const list = await ACL.getOrganizationRooms(req.user, [], organizationId,
            {
                include: [
                    {
                        model: Device,
                        include: [
                            {
                                required: false,
                                model: PromoCode,
                                attributes: ['id'],
                                where: {
                                    status: PlusResponseStatus.Success,
                                },
                            },
                        ]
                    },
                    {
                        model: Operation,
                        required: false,
                        where: {
                            status: OperationStatus.Pending
                        }
                    }
                ]
            }
        ).catch(notFound('Organization'));
        return list.map(_extractRoomInfo);
    },
    { wrapResult: true }
);

export const deleteRoomHandler = asyncJsonResponse(
    async (req) => {
        const { id } = req.params;
        try {
            const room = await ACL.getRoom(req.user, [], { id }).catch(notFound('Room'));
            const organizationId = room.organizationId;
            const deviceIds: string[] = [];
            await sequelize.transaction<void>(async (transaction) => {
                await Device.update({
                    roomId: null
                } as DeviceSchema,
                    {
                        where: {
                            roomId: id
                        },
                        transaction,
                        returning: true,

                    }).then(([, devices]: [number, DeviceInstance[]]) => {
                        devices.forEach(device => deviceIds.push(device.id));
                    });
                await room.destroy({ transaction });
            });
            deviceIds.forEach(device => sendPush({
                topic: organizationId,
                event: 'device-state',
                payload: device,
            }));
            sendPush({
                topic: organizationId,
                event: 'room-state',
                payload: id
            });
            return { status: 'ok' };
        } catch (error) {
            if (isEmptyResultError(error)) {
                return { status: 'ok' };
            }
            throw error;
        }
    },
    { wrapResult: true }
);

export const resetRoomHandler = asyncJsonResponse(
    async (req, res) => {
        const { id } = req.params;
        const room = await ACL.getRoom(req.user, ['status'], { id }, {
            include: [
                {
                    model: Device
                }]
        }).catch(notFound('Room'));
        const operationId = await resetRoomImplement(
            room,
            req.ip,
            req.headers,
            await serializeOperationScope(req, 'int'),
            true
        );
        setImmediate(async () => await resetActivationLinks(room));
        return { status: 'ok', operationId };

    },
    { wrapResult: true }
);

export const activateRoomHandler = asyncJsonResponse(
    async (req, res) => {
        const { id } = req.params;
        const room = await ACL.getRoom(req.user, ['status'], { id }, {
            include: [
                {
                    model: Device,
                }]
        }).catch(notFound('Room'));
        const operationId = await activateImplement(
            room,
            await serializeOperationScope(req, 'int'),
        );
        return { status: 'ok', operationId };

    },
    { wrapResult: true }
);

export const createRoomHandler = asyncJsonResponse(
    async (req, res) => {
        try {
            const { organizationId } = req.body;
            await ACL.getOrganization(req.user, ['edit'], { id: organizationId }).catch(
                notFound('Organization'),
            );
            const fields = req.body.room as Pick<RoomSchema, 'name' | 'externalRoomId'>;
            try {
                fields.name = (fields.name || '').trim();
                if (!fields.name) {
                    delete fields.name;
                }
                fields.externalRoomId = (fields.externalRoomId || '').trim();
                if (!fields.externalRoomId) {
                    delete fields.externalRoomId;
                }
            } catch (origError) {
                throw new RestApiError('Invalid req.body.room format', 400, {
                    origError,
                });
            }
            const room = await Room.create({
                ...fields,
                organizationId
            } as RoomSchema);

            return _extractRoomInfo(room);
        } catch (error) {
            return throwRestApiError(error);
        }
    },
    { wrapResult: true }
);

export const renameRoomHandler = asyncJsonResponse(
    async (req) => {
        const { id } = req.params;
        const { name } = req.body;
        try {
            const room = await ACL.getRoom(req.user, [], { id }, {
                include: [
                    {
                        model: Device
                    }]
            }).catch(notFound('Room'));
            await room.update({
                name: name?.trim() || null,
            });
            room.devices?.forEach(device => sendPush({
                topic: room.organizationId,
                event: 'device-state',
                payload: device.id
            }));
            sendPush({
                topic: room.organizationId,
                event: 'room-state',
                payload: id
            });
            return { status: 'ok' };
        } catch (error) {
            return throwRestApiError(error);
        }
    },
    { wrapResult: true }
);

export const editExternalRoomIdHandler = asyncJsonResponse(
    async (req) => {
        const { id } = req.params;
        const { externalRoomId } = req.body;
        try {
            const room = await ACL.getRoom(req.user, [], { id }).catch(notFound('Room'));
            await room.update({
                externalRoomId: externalRoomId?.trim() || null,
            });
            sendPush({
                topic: room.organizationId,
                event: 'room-state',
                payload: id
            });
            return { status: 'ok' };
        } catch (error) {
            return throwRestApiError(error);
        }
    },
    { wrapResult: true }
);


export const activatePromoCodeHandler = asyncJsonResponse(
    async (req) => {
        const { id } = req.params;
        const room = await ACL.getRoom(req.user, ['promocode'], { id },
            {
                include: [
                    {
                        model: Device
                    }
                ]
            }).catch(notFound('Room'));
        await syncRoom(room);
        await activatePromoCodeForRoom(
            room,
            req.ip,
            await serializeOperationScope(req, 'int'),
        );
        return { status: 'ok' };
    });