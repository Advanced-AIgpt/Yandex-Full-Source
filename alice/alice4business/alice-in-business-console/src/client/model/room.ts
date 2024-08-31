import { Status } from './device';

export enum OnlineStatus {
    Online = 'online',
    Offline = 'offline',
    Mixed = 'mixed',
}

export enum PromoStatus {
    Applied = 'applied',
    Available = 'available',
    NotAvailable = 'not_available',
}

export interface IRoom {
    id: string;
    organizationId: string;
    name: string;
    externalRoomId?: string;
    status: Status;
    onlineStatus: OnlineStatus;
    numDevices: number;
    pendingOperation?: string;
    promoStatus: PromoStatus;
}

export type IDeviceRoom = Pick<IRoom, 'id' | 'name'>;

export type IRoomCreation = Pick<IRoom, 'name' | 'externalRoomId'>;

export const onlineStatusLabel: Record<OnlineStatus, string> = {
    [OnlineStatus.Online]: 'В сети',
    [OnlineStatus.Offline]: 'Не в сети',
    [OnlineStatus.Mixed]: 'По-разному',
};
