import { quasarGotWithTvm } from './utils';
import { Platform } from '../../db/tables/device';

export const switchDeviceUser = (deviceId: string, platform: Platform, xCode: string) => {
    return quasarGotWithTvm('/admin/switch_device_user', {
        body: { device_id: deviceId, platform, x_code: xCode },
        method: 'post',
    });
};
