import { pushGot } from './utils';
import config from '../../lib/config';
import log from '../../lib/log';
import { DeviceInstance } from '../../db/tables/device';
import { RoomInstance } from '../../db/tables/room';
import { OrganizationInstance } from '../../db/tables/organization';

type RequireOnlyOne<T, Keys extends keyof T = keyof T> = Pick<T, Exclude<keyof T, Keys>> &
    {
        [K in Keys]-?: Required<Pick<T, K>> &
            Partial<Record<Exclude<Keys, K>, undefined>>;
    }[Keys];

interface AllSendParams {
    user: string;
    topic: string;
    event: string;

    payload: any;
    tags?: string[];
    keys?: string[];
}

type SendParams = RequireOnlyOne<AllSendParams, 'user' | 'topic'>;

export const notifyStateChange = (entity: DeviceInstance | RoomInstance | OrganizationInstance, ignoreRoom?: boolean): void => {
    if (entity instanceof RoomInstance) {
        sendPush({
            topic: entity.organizationId,
            event: 'room-state',
            payload: entity.id
        });
        entity.devices!.map(device => notifyStateChange(device, true));
    } else if (entity instanceof DeviceInstance) {
        sendPush({
            topic: entity.organizationId,
            event: 'device-state',
            payload: entity.id
        });
        if (entity.roomId && !ignoreRoom) {
            sendPush({
                topic: entity.organizationId,
                event: 'room-state',
                payload: entity.roomId
            }); 
        }
    } else if (entity instanceof OrganizationInstance) {
        sendPush({
            topic: entity.id,
            event: 'organization-info',
            payload: {}
        });
    }
}

export const sendPush = (params: SendParams): void => {
    setImmediate(async () => {
        try {
            await pushGot('/v2/send', {
                method: 'post',
                query: {
                    user: params.user,
                    topic: config.push.topicPrefix + params.topic,
                    event: params.event,
                },
                body: {
                    payload: params.payload,
                    tags: params.tags,
                    keys: params.keys,
                },
            });
        } catch (error) {
            log.warn('Failed to send notification', { error });
        }
    });
};
