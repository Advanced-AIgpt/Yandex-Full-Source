import {
    Column,
    CreatedAt,
    DataType,
    Default,
    Model,
    PrimaryKey,
    Table,
    UpdatedAt,
} from 'sequelize-typescript';
import { DeviceInstance, DeviceSchema } from './device';
import { RoomInstance } from './room';

export interface UserScope {
    method?: string;
    body?: any;
    params?: any;
    url?: string;
    context: 'ext' | 'int' | 'customer';
    userLogin: string;
    userId: string;
}

export interface AnonymousUserScope {
    method?: string;
    body?: any;
    params?: any;
    url?: string;
    context: 'customer';
    userLogin: null;
    userId: null;
}

export type Scope = UserScope | AnonymousUserScope;

export interface ActivatePayload {
    kolonkishLogin: string;
    kolonkishUid: string;
    shouldActivatePromoCode?: boolean;
    clientIp: string;
}

export interface ResetPayload {
    kolonkishLogin: string;
    kolonkishUid: string;
}

export interface ActivateForUserPayload {
    login: string;
    uid: string;
}

export enum Type {
    Activate = 'activate',
    Reset = 'reset',
    PromoActivate = 'promo-activate',
    ActivateRoom = 'activate-room',
    ResetRoom = 'reset-room'
}

export enum Status {
    Pending = 'pending',
    Resolved = 'resolved',
    Rejected = 'rejected',
}

export interface OperationSchema {
    id: string;
    type: Type;
    status: Status;
    payload: ActivatePayload | ResetPayload | ActivateForUserPayload | null;
    devicePk: string | null;
    roomPk: string | null;
    error: any | null;
    scope: Scope | null;
    lastHandling: Date;
    parentId: string | null;

    createdAt: Date;
    updatedAt: Date;
}

@Table({
    tableName: 'operations',
    modelName: 'operation',
    underscored: true,
})
export class OperationInstance extends Model<OperationInstance>
    implements OperationSchema {
    @PrimaryKey @Default(DataType.UUIDV4) @Column(DataType.UUID) readonly id!: string;
    @Column(DataType.STRING) readonly type!: Type;
    @Column(DataType.UUID) readonly devicePk!: string | null;
    @Column(DataType.UUID) readonly roomPk!: string | null;
    @Column(DataType.JSONB) readonly error!: any | null;
    @Default(Status.Pending) @Column(DataType.STRING) readonly status!: Status;
    @Column(DataType.JSONB) readonly payload!: ActivatePayload | ResetPayload | null;
    @CreatedAt readonly createdAt!: Date;
    @UpdatedAt readonly updatedAt!: Date;
    @Column(DataType.JSONB) readonly scope!: Scope | null;
    @Default(DataType.NOW) @Column(DataType.DATE) readonly lastHandling!: Date;
    @Column(DataType.UUID) readonly parentId!: string | null;

    readonly device?: DeviceInstance;
    readonly room?: RoomInstance;
    readonly parent?: OperationInstance;
    readonly children?: OperationInstance[];
}
