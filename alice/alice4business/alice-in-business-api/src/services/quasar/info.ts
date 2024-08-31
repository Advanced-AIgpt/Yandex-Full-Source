import { Platform } from '../../db/tables/device';
import { quasarGotWithTvm } from './utils';
import config from '../../lib/config';

export interface DeviceInfo {
    status: 'ok';
    device: {
        status: 'offline' | 'online' | 'unknown';
        owner_uid: string;
    };
}

export const getDeviceInfo = (deviceId: string, platform: Platform) =>
    quasarGotWithTvm<DeviceInfo>('/admin/device_info', {
        query: { device_id: deviceId, platform },
        timeout: config.quasar.timeout,
    });
