import { IOperation } from './operation';
import { IDeviceRoom } from './room';

export enum StationPlatform {
    YandexStation = 'yandexstation',
    YandexMini = 'yandexmini',
    YandexStation2 = 'yandexstation_2',
    YandexLight = 'yandexmicro',
    YandexMini2 = 'yandexmini_2',
    YandexMidi = 'yandexmidi',
}

export enum TvPlatform {
    CVTE351 = 'yandex_tv_hisi351_cvte',
    RTK2861 = 'yandex_tv_rt2861_hikeen',
    RTK2871 = 'yandex_tv_rt2871_hikeen',
    CV9632 = 'yandex_tv_mt9632_cv',
    Module = 'yandexmodule_2',
}

export type Platform = StationPlatform | TvPlatform;

export interface IDevice {
    id: string;
    organizationId: string;
    platform: Platform;
    deviceId: string;
    externalDeviceId?: string;
    note?: string;
    status: Status;
    agreementAccepted: boolean;
    online: boolean;
    isActivatedByCustomer?: boolean;
    hasPromo: boolean;
    pendingOperation?: string;
    room?: IDeviceRoom;
}

interface IRoomId {
    roomId?: string;
}

export type IDeviceCreation = Pick<IDevice, 'platform' | 'deviceId' | 'externalDeviceId' | 'note'> &
    IRoomId;

export type IMyDevice = Pick<IDevice, 'platform' | 'deviceId' | 'note'>;

export enum Status {
    Active = 'active',
    Inactive = 'inactive',
    Reset = 'reset',
    Mixed = 'mixed',
}

export const statusLabels: Record<Status, string> = {
    [Status.Active]: 'Активна',
    [Status.Inactive]: 'Сброшена',
    [Status.Reset]: 'Сбрасывается',
    [Status.Mixed]: 'По-разному',
};

export interface Kolonkish {
    login?: string;
    uid: string;
}

export interface History {
    operations: IOperation[];
    next: string;
}
