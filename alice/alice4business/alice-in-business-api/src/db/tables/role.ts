import { Column, DataType, Model, PrimaryKey, Table } from 'sequelize-typescript';

export const Roles = {
    admin: 'Администратор',
    support: 'Саппорт',
};

export const Permissions = {
    view: 'Просмотр списка устройств',
    edit: 'Добавление и удаление устройств',
    status: 'Управление статусом устройств',
    promocode: 'Использование промо-кодов',
    support: 'Взаимодействие с БД через веб интерфейс',
};

export type Role = keyof typeof Roles
export type Permission = keyof typeof Permissions
export type Permissions = Permission[]

export const getPermissionsByRoles = (roles: Role[]) => {
    const finalRoles = roles.reduce<Permissions>((arr, role) => {
        switch (role){
            case 'admin':
                arr = arr.concat(Object.keys(Permissions) as Permissions)
                break;
            case 'support':
                arr = arr.concat(['view', 'status', 'support'] as Permissions)
                break;
            default:
                return arr;
        }
        return arr;
    }, [])
    return new Set(finalRoles);
};

export interface RolesSchema {
    uid: string;
    role: keyof typeof Roles;
}

@Table({
    tableName: 'roles',
    modelName: 'role',
    underscored: true,
    timestamps: false,
})
export class RoleInstance extends Model<RoleInstance> implements RolesSchema {
    @PrimaryKey @Column(DataType.TEXT) readonly uid!: string;
    @PrimaryKey @Column(DataType.ENUM(...Object.keys(Roles))) readonly role!: keyof typeof Roles;
}
