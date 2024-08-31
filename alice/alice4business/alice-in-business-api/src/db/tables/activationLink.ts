import { Column, CreatedAt, DataType, Default, Model, PrimaryKey, Table } from "sequelize-typescript";
import { DeviceInstance } from "./device";
import { RoomInstance } from "./room";

export interface ActivationLinkSchema {
    id: string;
    deviceId: string | null;
    roomId: string | null;
    enabled: boolean;
    activeSince: Date
    activeTill: Date;
    createdAt: Date;
}

@Table({
    tableName: 'activation_links',
    modelName: 'activationLink',
    underscored: true,
    updatedAt: false
})
export class ActivationLinkInstance extends Model implements ActivationLinkSchema {
    @PrimaryKey @Default(DataType.UUIDV4) @Column(DataType.UUID) readonly id!: string;
    @Column(DataType.UUID) readonly deviceId!: string | null;
    @Column(DataType.UUID) readonly roomId!: string | null;
    @Column(DataType.BOOLEAN) enabled!: boolean;
    @Column(DataType.BOOLEAN) promoActivated!: boolean;
    @Column(DataType.DATE) activeSince!: Date;
    @Column(DataType.DATE) activeTill!: Date;
    
    @CreatedAt readonly createdAt!: Date;

    readonly device?: DeviceInstance;
    readonly room?: RoomInstance;
}