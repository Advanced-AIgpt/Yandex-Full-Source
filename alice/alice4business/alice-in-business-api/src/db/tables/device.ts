import {
    Column,
    CreatedAt,
    DataType,
    Default,
    DeletedAt,
    Model,
    PrimaryKey,
    Table,
    UpdatedAt,
} from 'sequelize-typescript';
import { OrganizationInstance } from './organization';
import { PromoCodeInstance } from './promoCode';
import { OperationInstance } from './operation';
import { PermissionInstance } from './permission';
import { RoomInstance } from './room';

export enum Status {
    Active = 'active',
    Inactive = 'inactive',
    Reset = 'reset',
}
export const statuses = Object.values(Status) as Status[];


export enum Platform {
    YandexStation = 'yandexstation',
    YandexMini = 'yandexmini',
    YandexStation2 = 'yandexstation_2',
    YandexLight = 'yandexmicro',
    YandexMini2 = 'yandexmini_2',
    CVTE351 = 'yandex_tv_hisi351_cvte',
    RTK2861 = 'yandex_tv_rt2861_hikeen',
    RTK2871 = 'yandex_tv_rt2871_hikeen',
    CV9632 = 'yandex_tv_mt9632_cv',
    Module = 'yandexmodule_2',
}

export interface DeviceSchema {
    id: string;
    organizationId: string;
    externalDeviceId: string | null;
    roomId: string | null;
    platform: Platform;
    deviceId: string;

    status: Status;
    kolonkishId: string | null;
    kolonkishLogin: string | null;
    agreementAccepted: boolean;

    lastSyncStart: Date;
    lastSyncUpdate: Date | null;
    online: boolean;
    ownerId: string | null;

    note: string | null;
    smartHomeUid: number | null;
    preactivatedSkillIds: string[];

    createdAt: Date;
    updatedAt: Date;
}

@Table({
    tableName: 'devices',
    modelName: 'device',
    underscored: true,
    paranoid: true,
})
export class DeviceInstance extends Model<DeviceInstance> implements DeviceSchema {
    @PrimaryKey @Default(DataType.UUIDV4) @Column(DataType.UUID) readonly id!: string;
    @Column(DataType.UUID) readonly organizationId!: string;
    @Column(DataType.UUID) readonly roomId!: string | null;
    @Column(DataType.TEXT) readonly platform!: Platform;
    @Column(DataType.TEXT) readonly deviceId!: string;
    @Column(DataType.TEXT) readonly externalDeviceId!: string | null;
    @Column(DataType.TEXT) readonly kolonkishId!: string | null;
    @Column(DataType.TEXT) readonly kolonkishLogin!: string | null;
    @Column(DataType.TEXT) readonly note!: string | null;
    @Default(Status.Reset) @Column(DataType.ENUM(...statuses)) readonly status!: Status;
    @CreatedAt readonly createdAt!: Date;
    @UpdatedAt readonly updatedAt!: Date;
    @Column(DataType.DATE) readonly lastSyncUpdate!: Date | null;
    @Default(false) @Column(DataType.BOOLEAN) readonly online!: boolean;
    @Column(DataType.TEXT) readonly ownerId!: string | null;
    @Default(0) @Column(DataType.DATE) readonly lastSyncStart!: Date;
    @DeletedAt readonly deletedAt!: Date | null;
    @Default(false) @Column(DataType.BOOLEAN) readonly agreementAccepted!: boolean;
    @Column(DataType.BIGINT) readonly smartHomeUid!: number | null;
    @Default([])
    @Column(DataType.ARRAY(DataType.STRING))
    readonly preactivatedSkillIds!: string[];

    readonly operations?: OperationInstance[];
    readonly organization?: OrganizationInstance;
    readonly permissions?: PermissionInstance[];
    readonly promoCodes?: PromoCodeInstance[];
    readonly room?: RoomInstance;

    public isTv(): boolean {
        return this.platform.startsWith('yandex_tv_') || this.platform === 'yandexmodule_2';      
    }

    public isStation(): boolean {
        return !this.isTv();
    }
}
