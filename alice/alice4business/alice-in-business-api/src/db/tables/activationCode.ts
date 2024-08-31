import {
    Column,
    CreatedAt,
    DataType,
    Model,
    PrimaryKey,
    Table,
} from 'sequelize-typescript';
import { DeviceInstance } from './device';

export interface ActivationCodeSchema {
    code: string;
    deviceId: string;

    createdAt: Date;
}

@Table({
    tableName: 'activation_codes',
    modelName: 'activationCode',
    underscored: true,
    updatedAt: false,
})
export class ActivationCodeInstance extends Model implements ActivationCodeSchema {
    @PrimaryKey @Column(DataType.STRING) readonly code!: string;
    @Column(DataType.UUID) readonly deviceId!: string;
    @CreatedAt readonly createdAt!: Date;

    readonly device?: DeviceInstance;
}
