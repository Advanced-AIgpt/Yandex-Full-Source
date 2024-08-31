import { Op } from 'sequelize';

import { asyncJsonResponse } from '../utils';
import { Device } from '../../db';
import { DeviceSchema } from '../../db/tables/device';
import { logSupportOperation } from './utils';

const OK_STATUS = 'ok'
const NOT_OK_STATUS = 'not ok'

export const getAllDevice = asyncJsonResponse(
    async () => {
        const devices = await Device.findAll(
            {
                where: {
                    deletedAt: { [Op.eq]: null },
                },
            }
        );
        return devices.map((device) => ({
            value: device.id,
            name: `${device.platform} (${device.deviceId})`,
        }));
    },
    { wrapResult: true },
);

export const addPuid = asyncJsonResponse(
    async (req) => {
        const operationType = 'device:add-puid';
        const logPuid = req.user.uid.toString();

        try {
            const { deviceId, puid } = req.body.params;

            await Device.update({smartHomeUid: Number(puid)}, {where: {id: deviceId}});
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
);

export const createDevice = asyncJsonResponse(
    async (req) => {
        const operationType = 'device:create';
        const logPuid = req.user.uid.toString();

        try {
            const {
                externalDeviceId,
                note,
                status,
                organizationId,
                deviceId,
                platform,
                kolonkishId,
                kolonkishLogin
            } = req.body.params;

            await Device.create({
                externalDeviceId,
                note,
                status,
                organizationId,
                deviceId,
                platform,
                kolonkishId,
                kolonkishLogin
            } as DeviceSchema);

            await logSupportOperation(operationType, true, logPuid)
            return {
                status: OK_STATUS
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
);

export const changeDevice = asyncJsonResponse(
    async (req) => {
        const operationType = 'device:change';
        const logPuid = req.user.uid.toString();

        try {
            const params = req.body.params;

            const device = await Device.findByPk(params.primaryId, {
                rejectOnEmpty: true,
            });

            await device!.update({...params} as unknown as DeviceSchema);

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
    { wrapResult: true }
);

export const getDefaults = asyncJsonResponse(
    async (req) => {
        const {id} = req.params;
        const {
            platform,
            deviceId,
            externalDeviceId,
            note,
            status,
            organizationId,
            kolonkishId,
            kolonkishLogin
        } = await Device.findByPk(id, {
            rejectOnEmpty: true,
        });

        return {
            platform,
            deviceId,
            externalDeviceId,
            note,
            status,
            organizationId,
            kolonkishId,
            kolonkishLogin
        }
    },
    { wrapResult: true }
);
