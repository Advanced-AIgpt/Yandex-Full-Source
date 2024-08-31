import { Sequelize } from 'sequelize-typescript';
import config from '../lib/config';
import { ActivationCodeInstance } from './tables/activationCode';
import { DeviceInstance } from './tables/device';
import { OrganizationInstance } from './tables/organization';
import { PermissionInstance } from './tables/permission';
import { UserInstance } from './tables/user';
import { PromoCodeInstance } from './tables/promoCode';
import { OperationInstance } from './tables/operation';
import { ConnectOrganizationInstance } from './tables/connectOrganization';
import { RoomInstance } from './tables/room';
import { ActivationLinkInstance } from './tables/activationLink';
import { RoleInstance } from './tables/role';
import { SupportOperationsInstance } from './tables/supportOperation';

export const sequelize = new Sequelize(config.db.uri, {
    ...config.db.options,
    repositoryMode: true,
});

sequelize.addModels([
    ActivationCodeInstance,
    DeviceInstance,
    OrganizationInstance,
    PermissionInstance,
    UserInstance,
    PromoCodeInstance,
    OperationInstance,
    ConnectOrganizationInstance,
    RoomInstance,
    ActivationLinkInstance,
    RoleInstance,
    SupportOperationsInstance
]);

export const ActivationCode = sequelize.getRepository(ActivationCodeInstance);
export const Device = sequelize.getRepository(DeviceInstance);
export const User = sequelize.getRepository(UserInstance);
export const Organization = sequelize.getRepository(OrganizationInstance);
export const Permission = sequelize.getRepository(PermissionInstance);
export const PromoCode = sequelize.getRepository(PromoCodeInstance);
export const Operation = sequelize.getRepository(OperationInstance);
export const ConnectOrganization = sequelize.getRepository(ConnectOrganizationInstance);
export const Room = sequelize.getRepository(RoomInstance);
export const ActivationLink = sequelize.getRepository(ActivationLinkInstance);
export const Role = sequelize.getRepository(RoleInstance);
export const SupportOperation = sequelize.getRepository(SupportOperationsInstance);

// *–Permission
Device.hasMany(Permission, { foreignKey: 'organizationId', sourceKey: 'organizationId' });
Organization.hasMany(Permission, { foreignKey: 'organizationId' });
Room.hasMany(Permission, { foreignKey: 'organizationId', sourceKey: 'organizationId' });

// ActivationCode–Device
ActivationCode.belongsTo(Device, { foreignKey: 'deviceId', targetKey: 'id' });
Device.hasMany(ActivationCode, { foreignKey: 'deviceId', sourceKey: 'id' });

// Device–Organization
Device.belongsTo(Organization, { foreignKey: 'organizationId', targetKey: 'id' });
Organization.hasMany(Device, { foreignKey: 'organizationId', sourceKey: 'id' });

// Operation—Device
Operation.belongsTo(Device, { foreignKey: 'devicePk', targetKey: 'id' });
Device.hasMany(Operation, { foreignKey: 'devicePk', sourceKey: 'id' });

// Operation-Room
Operation.belongsTo(Room, { foreignKey: 'roomPk', targetKey: 'id' });
Room.hasMany(Operation, { foreignKey: 'roomPk', sourceKey: 'id' });

// PromoCode—Organization
PromoCode.belongsTo(Organization, { foreignKey: 'organizationId', targetKey: 'id' });
Organization.hasMany(PromoCode, { foreignKey: 'organizationId', sourceKey: 'id' });

// PromoCode–Device TODO разобрать
Device.hasMany(PromoCode, { foreignKey: 'userId', sourceKey: 'kolonkishId' });

// PromoCode—Operation
PromoCode.belongsTo(Operation, { foreignKey: 'operationId', targetKey: 'id' });

// Organization–ConnectOrganization
Organization.belongsTo(ConnectOrganization, {
    foreignKey: 'connectOrgId',
    targetKey: 'id',
});
ConnectOrganization.hasMany(Organization, {
    foreignKey: 'connectOrgId',
    sourceKey: 'id',
});

// Room-Organization
Room.belongsTo(Organization, { foreignKey: 'organizationId', targetKey: 'id' });
Organization.hasMany(Room, { foreignKey: 'organizationId', sourceKey: 'id' });

// Device-Room
Device.belongsTo(Room, { foreignKey: 'roomId', targetKey: 'id' });
Room.hasMany(Device, { foreignKey: 'roomId', sourceKey: 'id' });

// Operation-parent operation
Operation.belongsTo(Operation, { as: 'parent', foreignKey: 'parentId' });
Operation.hasMany(Operation, { as: 'children', foreignKey: 'parentId' });

// Activation link to room and device
ActivationLink.belongsTo(Room, { foreignKey: 'roomId', targetKey: 'id' });
ActivationLink.belongsTo(Device, { foreignKey: 'deviceId', targetKey: 'id' });
Device.hasMany(ActivationLink, { foreignKey: 'deviceId', sourceKey: 'id' });
Room.hasMany(ActivationLink, { foreignKey: 'roomId', sourceKey: 'id' });

// GlobalPermission-User
Role.belongsTo(User, { foreignKey: 'uid', targetKey: 'id' });
