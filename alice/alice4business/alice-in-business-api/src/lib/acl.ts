import { Device, Role, Operation, Organization, Permission, Room } from '../db';
import { DeviceInstance } from '../db/tables/device';
import { OperationInstance } from '../db/tables/operation';
import { OrganizationInstance } from '../db/tables/organization';
import { Types as PermissionTypes } from '../db/tables/permission';
import { getPermissionsByRoles, Permissions as RolePermissions, Permission as RolePermission } from '../db/tables/role';
import { EmptyResultError, Includeable } from 'sequelize';
import log from './log';
import RestApiError from './errors/restApi';
import { getUserInfo } from '../services/blackbox';
import { RoomInstance } from '../db/tables/room';
import { isEmptyResultError } from './errors/utils';

type Permission = keyof typeof PermissionTypes | RolePermission;
type Permissions = Permission[] | RolePermissions;

const checkPermission = (requiredPermissions: Permissions, actualPermissions: Set<Permission> | Permissions) => {
    for (const permission of requiredPermissions) {
        const condition =
            actualPermissions instanceof Set
                ? !actualPermissions.has(permission)
                : !actualPermissions.includes(permission);
        if (condition) {
            return false;
        }
    }
    return true;
};

interface UserEntity {
    login: string;
    attributes: Record<string, any>;
}

export { ACLUser as User };
class ACLUser {
    constructor(uid: number, ip: string) {
        this.uid = uid;
        this.ip = ip;
        this.userInfo = getUserInfo({
            uid: this.uid,
            userip: this.ip,
            // проверка на плюс
            // https://doc.yandex-team.ru/Passport/AuthDevGuide/concepts/DB_About.html#DB_About__db-attributes
            attributes: '1015',
        })
            .then((res) => res.body.users[0])
            // не выбрасываю ошибку из catch, чтобы не было преждевременного unhandled promise rejection
            .catch((e) => {
                log.warn('Failed to get user info', {
                    uid: this.uid,
                    error: e,
                });

                return e as Error;
            });
    }

    private userInfo: Promise<UserEntity | Error>;
    public readonly uid: number;
    public readonly ip: string;
    public get login(): Promise<string> {
        // lazy get user login from BlackBox

        if (this._login === undefined) {
            this._login = this.userInfo.then((user) => {
                if (user instanceof Error) {
                    log.warn('Failed to get user login', {
                        uid: this.uid,
                        error: user,
                    });

                    this._login = undefined;

                    throw user;
                }

                return user.login;
            });
        }

        return this._login!;
    }
    public get hasPlus() {
        if (this._hasPlus === undefined) {
            this._hasPlus = this.userInfo.then((user) => {
                if (user instanceof Error) {
                    log.warn('Failed to check for plus', {
                        uid: this.uid,
                        error: user,
                    });

                    throw user;
                }

                return Boolean(user.attributes && user.attributes['1015']);
            });
        }

        return this._hasPlus;
    }

    private _login?: Promise<string>;
    private _hasPlus?: Promise<boolean>;
}

export const getDevice = async (
    user: ACLUser,
    requiredPermissions: Permissions,
    where: { id: string } | { deviceId: string } | { externalDeviceId: string },
    options: {
        include?: Includeable[];
        paranoid?: boolean;
    } = {},
): Promise<DeviceInstance> => {
    const roles = (await Role.findAll({ where: { uid: user.uid.toString() }}))!.map((role) => role.role)
    const permissions = getPermissionsByRoles(roles);

    if (permissions.size && checkPermission(requiredPermissions, permissions)) {
        return (await Device.findOne({
            where,
            include: [...(options.include || [])],
            paranoid: options.paranoid,
            rejectOnEmpty: true,
        }))!;
    }

    const device = (await Device.findOne({
        where,
        include: [
            {
                model: Permission,
                attributes: ['type'],
                where: { uid: user.uid },
                required: true,
            },
            ...(options.include || []),
        ],
        paranoid: options.paranoid,
        rejectOnEmpty: true,
    }))!;

    const actualPermissions = new Set(device.permissions!.map((item) => item.type));
    if (!checkPermission(requiredPermissions, actualPermissions)) {
        throw new RestApiError('Permission denied', 403, {
            payload: { requiredPermissions },
        });
    }

    return device;
};

export const getRoom = async (
    user: ACLUser,
    requiredPermissions: Permissions,
    where: { id: string } | { externalRoomId: string },
    options: {
        include?: Includeable[];
        paranoid?: boolean;
    } = {},
): Promise<RoomInstance> => {
    const roles = (await Role.findAll({ where: { uid: user.uid.toString() }}))!.map((role) => role.role)
    const permissions = getPermissionsByRoles(roles);

    if (permissions.size && checkPermission(requiredPermissions, permissions)) {
        return (await Room.findOne({
            where,
            include: [...(options.include || [])],
            paranoid: options.paranoid,
            rejectOnEmpty: true,
        }))!;
    }

    const room = (await Room.findOne({
        where,
        include: [
            {
                model: Permission,
                attributes: ['type'],
                where: { uid: user.uid },
                required: true,
            },
            ...(options.include || []),
        ],
        paranoid: options.paranoid,
        rejectOnEmpty: true,
    }))!;

    const actualPermissions = new Set(room.permissions!.map((item) => item.type));
    if (!checkPermission(requiredPermissions, actualPermissions)) {
        throw new RestApiError('Permission denied', 403, {
            payload: { requiredPermissions },
        });
    }

    return room;
};

export const getOrganization = async (
    user: ACLUser,
    requiredPermissions: Permissions,
    where: { id: string },
    options: {
        include: Includeable[];
    } = { include: [] },
): Promise<OrganizationInstance> => {
    const roles = (await Role.findAll({ where: { uid: user.uid.toString() }}))!.map((role) => role.role)
    const permissions = getPermissionsByRoles(roles);

    if (permissions.size && checkPermission(requiredPermissions, permissions)) {
        return (await Organization.findOne({
            where,
            include: [...options.include],
            rejectOnEmpty: true,
        }))!;
    }
    const organization = (await Organization.findOne({
        where,
        include: [
            {
                model: Permission,
                attributes: ['type'],
                where: { uid: user.uid },
                required: true,
            },
            ...options.include,
        ],
        rejectOnEmpty: true,
    }))!;

    const actualPermissions = new Set(organization.permissions!.map((item) => item.type));
    if (!checkPermission(requiredPermissions, actualPermissions)) {
        throw new RestApiError('Permission denied', 403, {
            payload: { requiredPermissions },
        });
    }

    return organization;
};

export const getOrganizationDevices = async (
    user: ACLUser,
    requiredPermissions: Permissions,
    organizationId: string,
    roomId?: string | null,
    options: {
        include?: Includeable[];
        paranoid?: boolean;
    } = {},
): Promise<DeviceInstance[]> => {
    const roles = (await Role.findAll({ where: { uid: user.uid.toString() }}))!.map((role) => role.role)
    const permissions = getPermissionsByRoles(roles);

    if (permissions.size && checkPermission(requiredPermissions, permissions)) {
        return (await Organization.findOne({
            where: { id: organizationId },
            include: [
                {
                    model: Device,
                    where: roomId ? { roomId: roomId! } : {},
                    required: false,
                    include: options.include || [],
                    paranoid: options.paranoid,
                },
            ],
            rejectOnEmpty: true,
        }))!.devices!;
    }

    const organization = (await Organization.findOne({
        where: { id: organizationId },
        include: [
            {
                model: Permission,
                attributes: ['type'],
                where: { uid: user.uid },
                required: true,
            },
            {
                model: Device,
                where: roomId ? { roomId: roomId! } : {},
                required: false,
                include: options.include || [],
                paranoid: options.paranoid,
            },
        ],
        rejectOnEmpty: true,
    }))!;

    const actualPermissions = new Set(organization.permissions!.map((item) => item.type));
    if (!checkPermission(requiredPermissions, actualPermissions)) {
        throw new RestApiError('Permission denied', 403, {
            payload: { requiredPermissions },
        });
    }

    return organization.devices!;
};

export const getOrganizationRooms = async (
    user: ACLUser,
    requiredPermissions: Permissions,
    organizationId: string,
    options: {
        include?: Includeable[];
    } = {},
): Promise<RoomInstance[]> => {
    const roles = (await Role.findAll({ where: { uid: user.uid.toString() }}))!.map((role) => role.role)
    const permissions = getPermissionsByRoles(roles);

    if (permissions.size && checkPermission(requiredPermissions, permissions)) {
        return (
            await Organization.findOne({
                where: { id: organizationId },
                include: [
                    {
                        model: Room,
                        include: options.include || [],
                    },
                ],
                rejectOnEmpty: true,
            })
        ).rooms!;
    }

    const organization = await Organization.findOne({
        where: { id: organizationId },
        include: [
            {
                model: Permission,
                attributes: ['type'],
                where: { uid: user.uid },
                required: true,
            },
            {
                model: Room,
                include: options.include || [],
            },
        ],
        rejectOnEmpty: true,
    });

    const actualPermissions = new Set(organization.permissions!.map((item) => item.type));
    if (!checkPermission(requiredPermissions, actualPermissions)) {
        throw new RestApiError('Permission denied', 403, {
            payload: { requiredPermissions },
        });
    }

    return organization.rooms!;
};

export const getOrganizations = async (
    user: ACLUser,
    requiredPermissions: Permissions,
    options: {
        include: Includeable[];
    } = { include: [] },
): Promise<OrganizationInstance[]> => {
    const roles = (await Role.findAll({ where: { uid: user.uid.toString() }}))!.map((role) => role.role)
    const permissions = getPermissionsByRoles(roles);

    if (permissions.size && checkPermission(requiredPermissions, permissions)) {
        return await Organization.findAll({
            include: [...options.include],
        });
    }

    const organizations = await Organization.findAll({
        include: [
            {
                model: Permission,
                attributes: ['type'],
                where: { uid: user.uid },
                required: true,
            },
            ...options.include,
        ],
    });

    return organizations.filter((organization) => {
        const actualPermissions = new Set(organization.permissions!.map((item) => item.type));
        return checkPermission(requiredPermissions, actualPermissions);
    });
};

export const getOperation = async (
    user: ACLUser,
    requiredPermissions: Permissions,
    where: { id: string },
    options: {
        include?: Includeable[];
    } = {},
): Promise<OperationInstance> => {
    const roles = (await Role.findAll({ where: { uid: user.uid.toString() }}))!.map((role) => role.role)
    const permissions = getPermissionsByRoles(roles);
    const hasGlobalPermission = permissions.size && checkPermission(requiredPermissions, permissions);

    const deviceOperation = await Operation.findOne({
        where,
        include: [
            {
                model: Device,
                required: true,
                paranoid: false,
                include: hasGlobalPermission
                    ? []
                    : [
                          {
                              model: Permission,
                              attributes: ['type'],
                              where: { uid: user.uid },
                              required: true,
                          },
                      ],
            },
            ...(options.include || []),
        ],
        rejectOnEmpty: true,
    }).catch((error) => {
        if (isEmptyResultError(error)) {
            return null;
        } else {
            throw error;
        }
    });
    const roomOperation = await Operation.findOne({
        where,
        include: [
            {
                model: Room,
                required: true,
                paranoid: false,
                include: hasGlobalPermission
                    ? []
                    : [
                          {
                              model: Permission,
                              attributes: ['type'],
                              where: { uid: user.uid },
                              required: true,
                          },
                      ],
            },
            ...(options.include || []),
        ],
        rejectOnEmpty: true,
    }).catch((error) => {
        if (isEmptyResultError(error)) {
            return null;
        } else {
            throw error;
        }
    });
    if (!deviceOperation && !roomOperation) {
        throw new EmptyResultError();
    }
    const operation = (deviceOperation || roomOperation)!;

    if (hasGlobalPermission) return operation!;

    const actualPermissions = new Set<Permission>();
    if (operation!.device) {
        operation.device!.permissions!.forEach((item) => actualPermissions.add(item.type));
    }
    if (operation.room) {
        operation.room!.permissions!.forEach((item) => actualPermissions.add(item.type));
    }

    if (!checkPermission(requiredPermissions, actualPermissions)) {
        throw new RestApiError('Permission denied', 403, {
            payload: { requiredPermissions },
        });
    }

    return operation!;
};

export const checkSupportCommandAccess = async (
    user: ACLUser,
): Promise<boolean> => {
    const roles = (await Role.findAll({ where: { uid: user.uid.toString() }}))!.map((role) => role.role)
    const permissions = getPermissionsByRoles(roles);

    return checkPermission(['support'], permissions)
}
