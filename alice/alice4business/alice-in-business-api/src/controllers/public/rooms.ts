import RestApiError from "../../lib/errors/restApi";
import * as ACL from '../../lib/acl';
import log from '../../lib/log';
import { asyncJsonResponse, notFound } from "../utils";
import { createActivation, resetActivationLinks } from "./activations";
import { PromoStatus, RoomInstance } from "../../db/tables/room";
import { Device, Operation, PromoCode } from "../../db";
import { Status as OperationStatus } from "../../db/tables/operation";
import { syncRoom } from "../../lib/sync/quasar";
import { resetRoomImplement } from "../../lib/implements/reset";
import { serializeOperationScope } from "../../lib/utils";
import { activateImplement } from "../../lib/implements/activate";
import { activatePromoCodeForRoom } from "../../lib/implements/promo-activate";
import { Status } from "../../db/tables/device";

export const getRoomInfoHandler = asyncJsonResponse(
    async req => {
        const roomId = req.query.room_id;
        const externalRoomId = req.query.external_room_id;
        let room: RoomInstance
        if (roomId && typeof roomId === "string") {
            room = await ACL.getRoom(req.user, [], { id: roomId }, {
                include: [
                    {
                        required: true,
                        model: Device,
                        include: [
                            {
                                required: false,
                                model: Operation,
                                attributes: ["id"],
                                where: {
                                    status: OperationStatus.Pending,
                                }
                            },
                            {
                                model: PromoCode
                            }
                        ]
                    },
                    {
                        required: false,
                        model: Operation,
                        attributes: ["id"],
                        where: {
                            status: OperationStatus.Pending
                        }
                    }
                ]
            }).catch(
                notFound("Room"),
            );
        } else if (externalRoomId && typeof externalRoomId === 'string') {
            room = await ACL.getRoom(req.user, [], { externalRoomId }, {
                include: [
                    {
                        required: true,
                        model: Device,
                        include: [
                            {
                                required: false,
                                model: Operation,
                                attributes: ["id"],
                                where: {
                                    status: OperationStatus.Pending,
                                }
                            },
                            { 
                                model: PromoCode 
                            }
                        ]
                    },
                    {
                        required: false,
                        model: Operation,
                        attributes: ["id"],
                        where: {
                            status: OperationStatus.Pending
                        }
                    }
                ]
            }).catch(
                notFound("Room"),
            );
        } else {
            throw new RestApiError("room_id or external_room_id parameter is required", 400);
        }
        await syncRoom(room);

        return {
            status: "ok",
            external_room_id: room.externalRoomId,
            info: {
                id: room.id,
                name: room.name,
                external_room_id: room.externalRoomId,
                online: room.getOnlineStatus(),
                status: room.getStatus(),
                promo_status: room.getPromoStatus(),
                in_progress: room.operations ? room.operations.length > 0 : false,
                devices: 
                    room.devices?.map(device => ({
                        id: device.id,
                        external_id: device.externalDeviceId,
                        online: device.online,
                        status: device.status,
                        device_id: device.deviceId,
                        note: device.note,
                        in_progress: device.operations ? device.operations.length > 0 : false,
                        has_plus: device.promoCodes && device.promoCodes.length > 0 && device.status ===  Status.Active,
                        has_custom_user: device.ownerId != null && device.kolonkishId !== device.ownerId
                    }))
            }
        }

    }
);

export const activateRoomHandler = asyncJsonResponse(
    async req => {
        const externalRoomId = req.body.external_room_id;
        if (!externalRoomId || typeof externalRoomId !== "string") {
            throw new RestApiError('external_room_id parameter is required', 400);
        }
        const applyPromoCode = req.body.with_promocode === true;

        const room = await ACL.getRoom(req.user, ["status"], { externalRoomId }, {
            include: [
                {
                    required: true,
                    model: Device,
                    include: [
                        { model: PromoCode }
                    ]
                }]
        }).catch(notFound('Room'));
        const operationId = await activateImplement(
            room,
            await serializeOperationScope(req, 'ext'),
        );

        if (applyPromoCode) {
            setImmediate(async () => {
                await syncRoom(room);
                await room.reload();
                if (room.getPromoStatus() === PromoStatus.Available) {
                    await activatePromoCodeForRoom(
                        room,
                        req.ip,
                        await serializeOperationScope(req, 'ext'),
                    ).catch((error) => {
                        log.warn('Failed to apply promocode after activation', { error });
                    });
                }
            });
        }
        return { status: 'ok', operationId };
    },
    {
        putPostStatus: 200,
    },
);

export const resetRoomHandler = asyncJsonResponse(
    async req => {
        const externalRoomId = req.body.external_room_id;
        if (!externalRoomId || typeof externalRoomId !== "string") {
            throw new RestApiError("external_room_id parameter is required", 400);
        }
        const partial = req.body.partial;
        if (partial !== undefined && typeof partial !== "boolean") {
            throw new RestApiError("partial should be boolean", 400);
        }
        const room = await ACL.getRoom(req.user, ['status'], { externalRoomId }, {
            include: [
                {
                    required: true,
                    model: Device,
                }]
        }).catch(notFound('Room'));

        const operationId = await resetRoomImplement(
            room, req.ip, req.headers, await serializeOperationScope(req, "ext"), !partial
        );
        setImmediate(async () => resetActivationLinks(room));
        return { status: 'ok', operationId };
    }
);

export const activatePromoCodeHandler = asyncJsonResponse(
    async req => {
        const externalRoomId = req.body.external_room_id;
        if (!externalRoomId || typeof externalRoomId !== 'string') {
            throw new RestApiError('external_id parameter is required', 400);
        }
        const room = await ACL.getRoom(req.user, ['status'], { externalRoomId }, {
            include: [
                {
                    required: true,
                    model: Device,
                    include: [
                        { model: PromoCode }
                    ]
                }]
        }).catch(notFound('Room'));

        await syncRoom(room);
        if (room.getPromoStatus() !== PromoStatus.Available) {
            throw new RestApiError('promo cannot be applied to room', 400);
        }
        await activatePromoCodeForRoom(
            room,
            req.ip,
            await serializeOperationScope(req, 'ext'),
        );
        return { status: 'ok' };
    }
);

export const createRoomActivationHandler = asyncJsonResponse(
    async (req, res) => {
        const externalRoomId = req.body.external_id;
        if (!externalRoomId || typeof externalRoomId !== 'string') {
            throw new RestApiError('external_id parameter is required', 400);
        }
        const firstActivationDate = new Date(req.body.first_activation_date);
        const lastActivationDate = new Date(req.body.last_activation_date);
        const room = await ACL.getRoom(req.user, ['status'], { externalRoomId }, {
            include: [
                {
                    required: true,
                    model: Device
                }]
        }).catch(notFound('Room'));
        const activation = await createActivation(room, firstActivationDate, lastActivationDate)
        return {
            status: 'ok',
            id: activation.id,
        }
    }
);