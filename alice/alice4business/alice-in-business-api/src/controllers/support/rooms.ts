import { asyncJsonResponse } from '../utils';
import { Device, Room } from '../../db';
import { RoomSchema } from '../../db/tables/room';
import { DeviceSchema } from '../../db/tables/device';

import { logSupportOperation } from './utils';

const OK_STATUS = 'ok'
const NOT_OK_STATUS = 'not ok'

interface CreateRoomParams {
    organizationId: string;
    name: string;
    externalRoomId: string | null;
    orgDeviceIdsCheckbox: string[]
}

export const createRoom = asyncJsonResponse(
    async (req) => {
        const operationType = 'rooms:create';
        const logPuid = req.user.uid.toString();

        try {
            const { organizationId, name, externalRoomId, orgDeviceIdsCheckbox }: CreateRoomParams = req.body.params

            const r = await Room.create({
                externalRoomId: externalRoomId || null,
                name,
                organizationId
            } as RoomSchema);

            const updates = [] as Promise<any>[];
            for (const deviceId of orgDeviceIdsCheckbox) {
                updates.push(Device.update({roomId: r.id} as DeviceSchema, {where: {id: deviceId}}));
            }
            await Promise.all(updates)

            await logSupportOperation(operationType, true, logPuid)
            return {
                status: OK_STATUS,
            }
        } catch (e) {
            const id = await logSupportOperation(operationType, false, logPuid, e.message)
            return {
                status: NOT_OK_STATUS,
                message: `Operation failed with id ${id}`
            }
        }
    },
    { wrapResult: true },
)
