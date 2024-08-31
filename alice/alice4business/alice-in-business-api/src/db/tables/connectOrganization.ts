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

export interface ConnectOrganizationSchema {
    id: number;
    name: string;
    active: boolean;

    lastSync: Date | null;

    createdAt: Date;
    updatedAt: Date;
}

@Table({
    tableName: 'connect_organizations',
    modelName: 'connectOrganization',
    underscored: true,
})
export class ConnectOrganizationInstance extends Model
    implements ConnectOrganizationSchema {
    @PrimaryKey @Column(DataType.INTEGER) readonly id!: number;
    @Default('') @Column(DataType.TEXT) readonly name!: string;
    @Default(true) @Column(DataType.BOOLEAN) readonly active!: boolean;
    @Column(DataType.DATE) readonly lastSync!: Date | null;
    @CreatedAt readonly createdAt!: Date;
    @UpdatedAt readonly updatedAt!: Date;
}
