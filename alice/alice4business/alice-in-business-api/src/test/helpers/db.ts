import {
    sequelize,
    Device,
    Operation,
    Organization,
    Permission,
    PromoCode,
    User,
    Room,
} from '../../db';
import data from './data';
import { OrganizationSchema } from '../../db/tables/organization';
import { PromoCodeSchema } from '../../db/tables/promoCode';
import { DeviceSchema } from '../../db/tables/device';
import {
    PermissionSchema,
    Source,
    Types as PermissionTypes,
} from '../../db/tables/permission';
import Sequelize from 'sequelize';
import { UserSchema } from '../../db/tables/user';
import { RoomSchema } from '../../db/tables/room';

export const wipeDatabase = async () => {
    const tables: any[] = await sequelize.query(
        `
        SELECT
            *
        FROM
            information_schema.tables
        WHERE
            table_schema = 'public'
            AND table_type = 'BASE TABLE'
            AND table_name != 'SequelizeMeta'
        ;
    `,
        {
            type: Sequelize.QueryTypes.SELECT,
        },
    );
    const queryInterface = sequelize.getQueryInterface();
    for (const table of tables) {
        const tableName = queryInterface.quoteIdentifier(table.table_name, true);
        await sequelize.query(`TRUNCATE ${tableName} CASCADE;`);
    }
};

export const createUser = async (uid?: number, login?: string) =>
    User.create({
        id: (uid || data.user.uid).toString(10),
        login: login || (await data.user.login),
    } as UserSchema);

export const createOrganization = async (props: Partial<OrganizationSchema> = {}) =>
    Organization.create({
        ...data.organization,
        ...props,
    } as OrganizationSchema);

export const bindUserToOrganization = async (
    uid: number = data.user.uid,
    organizationId: string = data.organization.id,
    grants: (keyof typeof PermissionTypes)[] = Object.keys(PermissionTypes) as any,
) =>
    Permission.bulkCreate(
        grants.map((type) => ({
            uid,
            organizationId,
            type,
            source: Source.Native,
        })) as PermissionSchema[],
        {
            ignoreDuplicates: true,
        },
    );

export const createPromoCode = async (props: Partial<PromoCodeSchema> = {}) =>
    PromoCode.create({ ...data.promocode, ...props } as PromoCodeSchema);

export const createDevice = async (props: Partial<DeviceSchema> = {}, userId?: number) =>
    Device.create({ ...data.device, ...props } as DeviceSchema).then((device) =>
        device.reload({ include: [Organization, Room] }),
    );

export const createRoom = async (props: Partial<RoomSchema> = {}) =>
    Room.create({ ...data.room, ...props } as RoomSchema).then((room) =>
        room.reload({ include: [Organization, Device] }),
    );

export const bulkCreateDevices = async (
    count: number,
    props: Partial<DeviceSchema> = {},
) =>
    Promise.all(
        Array(count)
            .fill(null)
            .map((x) =>
                createDevice({
                    id: data.uuid(),
                    deviceId: data.uniqueString(),
                    kolonkishId: data.uniqueString(),
                    kolonkishLogin: data.uniqueString(),
                    externalDeviceId: data.uniqueString(),
                    ...props,
                }),
            ),
    );

export const getLastOperation = async (devicePk?: string) =>
    Operation.findOne({
        where: devicePk ? { devicePk } : undefined,
        order: [['updatedAt', 'DESC']],
        limit: 1,
    }).then((x) => x!);
