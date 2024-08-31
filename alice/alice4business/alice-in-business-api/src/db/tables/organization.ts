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
import { PromoCodeInstance } from './promoCode';
import { DeviceInstance } from './device';
import { PermissionInstance } from './permission';
import { ConnectOrganizationInstance } from './connectOrganization';
import { RoomInstance } from './room';

export interface OrganizationSchema {
    id: string;
    connectOrgId: number | null;
    name: string;
    templateUrl: string | null;
    preactivatedSkillIds: string[];
    usesRooms: boolean | null;
    imageUrl: string | null;
    infoUrl: string | null;
    infoTitle: string | null;
    infoSubtitle: string | null;
    maxStationVolume: number | null;

    createdAt: Date;
    updatedAt: Date;
}

@Table({
    tableName: 'organizations',
    modelName: 'organization',
    underscored: true,
})
export class OrganizationInstance extends Model<OrganizationInstance>
    implements OrganizationSchema {
    @PrimaryKey @Default(DataType.UUIDV4) @Column(DataType.UUID) readonly id!: string;
    @Column(DataType.TEXT) readonly name!: string;
    @CreatedAt readonly createdAt!: Date;
    @UpdatedAt readonly updatedAt!: Date;
    @Column(DataType.TEXT) readonly templateUrl!: string | null;
    @Column(DataType.INTEGER) readonly connectOrgId!: number | null;
    @Default([])
    @Column(DataType.ARRAY(DataType.STRING))
    readonly preactivatedSkillIds!: string[];
    @Column(DataType.BOOLEAN)
    readonly usesRooms!: boolean | null;
    @Column(DataType.TEXT) readonly imageUrl!: string | null;
    @Column(DataType.TEXT) readonly infoUrl!: string | null;
    @Column(DataType.TEXT) readonly infoTitle!: string | null;
    @Column(DataType.TEXT) readonly infoSubtitle!: string | null;
    @Column(DataType.INTEGER) readonly maxStationVolume!:  number | null;

    readonly devices?: DeviceInstance[];
    readonly promoCodes?: PromoCodeInstance[];
    readonly permissions?: PermissionInstance[];
    readonly connectOrganization?: ConnectOrganizationInstance;
    readonly rooms?: RoomInstance[];
}
