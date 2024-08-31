import { Column, DataType, Model, PrimaryKey, Table } from 'sequelize-typescript';

export const Types = {
    view: 'Просмотр списка устройств',
    edit: 'Добавление и удаление устройств',
    status: 'Управление статусом устройств',
    promocode: 'Использование промо-кодов',
};
const types = Object.values(Types) as (keyof typeof Types)[];

export enum Source {
    Native = 'native',
    Connect = 'connect',
}
const sources = Object.values(Source) as Source[];

export interface PermissionSchema {
    organizationId: string;
    uid: number;
    type: keyof typeof Types;
    source: Source;
}

@Table({
    tableName: 'permissions',
    modelName: 'permission',
    underscored: true,
    timestamps: false,
})
export class PermissionInstance extends Model<PermissionInstance>
    implements PermissionSchema {
    @PrimaryKey @Column(DataType.UUID) readonly organizationId!: string;
    @PrimaryKey @Column(DataType.BIGINT) readonly uid!: number;
    @PrimaryKey @Column(DataType.ENUM(...types)) readonly type!: keyof typeof Types;
    @PrimaryKey @Column(DataType.ENUM(...sources)) readonly source!: Source;
}
