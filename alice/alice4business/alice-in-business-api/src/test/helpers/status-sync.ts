import { DeviceInstance, Status } from '../../db/tables/device';

export function syncDeviceStub(props?: {
    status?: Status;
    online?: boolean;
    ownerId?: string;
}) {
    return async (device: DeviceInstance, logContext?: object) => {
        Object.assign(device, props);
    };
}
