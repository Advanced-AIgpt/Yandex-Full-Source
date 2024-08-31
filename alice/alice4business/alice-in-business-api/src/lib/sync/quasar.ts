import { DeviceInstance, DeviceSchema } from '../../db/tables/device';
import { getDeviceInfo } from '../../services/quasar/info';
import { Device } from '../../db';
import { Op, WhereValue } from 'sequelize';
import log from '../log';
import config from '../config';
import { sendPush } from '../../services/push/send';
import { RoomInstance } from '../../db/tables/room';

export const syncRoom = async (
    room: RoomInstance,
    logContext?: object
): Promise<void> => {
    if (room.devices) {
        await Promise.all(room.devices!.map(async device => syncDevice(device, logContext)));
    }
}

export const syncDevice = async (
    device: DeviceInstance,
    logContext?: object,
): Promise<void> => {
    logContext = {
        ...logContext,
        syncDevice: {
            id: device.id,
        },
    };

    log.debug('sync device status: start', logContext);
    const syncStart = Date.now();
    const initialDeviceState = await Device.update(
        {
            lastSyncStart: new Date(syncStart),
        } as DeviceSchema,
        {
            where: {
                id: device.id,
                lastSyncStart: {
                    [Op.lte]: new Date(syncStart - config.quasar.retryInterval),
                },
            },
            returning: true,
            silent: true, // не менять updatedAt
        },
    ).then(([count, updatedDevices]) => {
        if (count === 1) {
            return updatedDevices[0];
        } else {
            return;
        }
    });

    if (initialDeviceState) {
        log.debug('sync device status: device locked', logContext);
        const deviceInfo = await getDeviceInfo(device.deviceId, device.platform);

        log.debug('sync device status: got device status', {
            ...logContext,
            deviceState: {
                online: deviceInfo.device.status === 'online',
                ownerId: deviceInfo.device.owner_uid,
            },
        });
        const syncUpdate = Date.now();
        const newDeviceState = await Device.update(
            {
                lastSyncUpdate: new Date(syncUpdate),
                online: deviceInfo.device.status === 'online',
                ownerId: deviceInfo.device.owner_uid,

                updatedAt: new Date(syncUpdate),
            } as DeviceSchema,
            {
                where: {
                    id: device.id,
                    lastSyncStart: new Date(syncStart) as WhereValue, // условие блокировки
                },
                silent: true, // использовать updatedAt из values
                returning: true,
            },
        ).then(([count, updatedDevices]) => {
            if (count === 1) {
                return updatedDevices[0];
            } else {
                return;
            }
        });

        if (newDeviceState) {
            device.set(newDeviceState.get(), {
                raw: true,
                reset: true,
            });
            if (
                newDeviceState.online !== initialDeviceState.online ||
                newDeviceState.ownerId !== initialDeviceState.ownerId
            ) {
                sendPush({
                    topic: device.organizationId,
                    event: 'device-state',
                    payload: device.id,
                });
            }
            log.info('sync device status: done', logContext);
        } else {
            log.info("sync device status: can't update device", logContext);
        }
    } else {
        await device.reload();
        log.info('sync device status: device is up to date, skip', logContext);
    }
};

export const syncDeviceList = async (deviceList: DeviceInstance[], logContext?: object) =>
    Promise.all(
        deviceList.map((device) =>
            syncDevice(device, logContext).catch((e) => {
                log.error('sync device status: failed ' + e.toString(), {
                    ...logContext,
                    syncDevice: {
                        id: device.id,
                    },
                });
            }),
        ),
    );
