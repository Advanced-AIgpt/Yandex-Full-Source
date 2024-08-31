import { bulbasaurGotWithTvm } from './utils';
import { Platform } from '../../db/tables/device';

interface UserDevice {
    deviceId: string;
    platform: Platform;
    note?: string;
}

export const getUserDevices = async (userTicket: string) => {
    interface UserInfoResponse {
        request_id: string;
        payload: {
            devices: {
                name: string;
                type: string;
                quasar_info: {
                    device_id: string;
                    platform: Platform;
                };
            }[];
        };
    }
    const userInfo = await bulbasaurGotWithTvm<UserInfoResponse>('/v1.0/user/info', {
        method: 'get',
        headers: {
            'X-Ya-User-Ticket': userTicket,
        },
    });

    const result = [] as UserDevice[];
    for (const device of userInfo.payload.devices) {
        if (
            device.quasar_info &&
            (device.type === 'devices.types.smart_speaker.yandex.station' ||
                device.type === 'devices.types.smart_speaker.yandex.station.mini')
        ) {
            result.push({
                deviceId: device.quasar_info.device_id,
                platform: device.quasar_info.platform,
                note: device.name,
            });
        }
    }

    return result;
};
