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
import { DeviceInstance } from './device';
import { OperationInstance } from './operation';
import { OrganizationInstance } from './organization';
import { PermissionInstance } from './permission';


export interface RoomSchema {
    id: string;
    organizationId: string;
    externalRoomId: string | null;
    name: string | null;
    createdAt: Date;
    updatedAt: Date;
}

export enum Status {
    Active = 'active',
    Inactive = 'inactive',
    Reset = 'reset',
    Mixed = 'mixed'
};

export enum OnlineStatus {
    Online = 'online',
    Offline = 'offline',
    Mixed = 'mixed'
};

export enum PromoStatus {
    Applied = 'applied',
    Available = 'available',
    NotAvailable = 'not_available'
}

@Table({
    tableName: 'rooms',
    modelName: 'room',
    underscored: true,
    paranoid: true,
})
export class RoomInstance extends Model<RoomInstance> implements RoomSchema {
    @PrimaryKey @Default(DataType.UUIDV4) @Column(DataType.UUID) readonly id!: string;
    @Column(DataType.UUID) readonly organizationId!: string;
    @Column(DataType.TEXT) readonly externalRoomId!: string | null;
    @Column(DataType.TEXT) readonly name!: string | null;
    @CreatedAt readonly createdAt!: Date;
    @UpdatedAt readonly updatedAt!: Date;
    @DeletedAt readonly deletedAt!: Date | null;

    readonly operations?: OperationInstance[];
    readonly organization?: OrganizationInstance;
    readonly devices?: DeviceInstance[];
    readonly permissions?: PermissionInstance[];

    public getOnlineStatus(): OnlineStatus {
        if (!this.devices) {
            return OnlineStatus.Offline;
        }
        if (this.devices.every(device => device.online)) {
            return OnlineStatus.Online;
        } else if (this.devices.every(device => !device.online)) {
            return OnlineStatus.Offline;
        } else {
            return OnlineStatus.Mixed;
        }
    }

    public getStatus(): Status {
        if (!this.devices) {
            return Status.Reset;
        }
        const statuses = new Set(this.devices.map(device => device.status));
        if (statuses.size === 1) {
            return statuses.values().next().value;
        } else {
            return Status.Mixed;
        }
    }

    public getPromoStatus(): PromoStatus {
        if (this.devices) {
            const activeDevices = this.devices!.filter(d => d.status === Status.Active)
            if (activeDevices.length === 0) {
                return PromoStatus.NotAvailable;
            }
            if (activeDevices.every(device => device.promoCodes && device.promoCodes.length > 0)) {
                return PromoStatus.Applied;
            }
            if (activeDevices.every(device => device.ownerId === device.kolonkishId)) {
                const firstOwner = this.devices[0].ownerId;
                if (activeDevices.every(device => device.ownerId === firstOwner)) {
                    return PromoStatus.Available;
                }
            }
        } 
        return PromoStatus.NotAvailable;
    }
}