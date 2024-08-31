import { QueryInterface } from 'sequelize';
import { Platform } from '../tables/device';
import { Source, Types as PermissionTypes } from '../tables/permission';

const currentDate = new Date();

const connectOrganizations = [
    {
        id: 103813,
        name: 'синхронизируй меня',
        active: true,

        created_at: currentDate,
        updated_at: currentDate,
    },
];

const organizations = [
    {
        id: '8cee5894-073b-441e-a11b-af6a98387b9c',
        name: 'Crowne Plaza',
        connect_org_id: connectOrganizations[0].id,

        created_at: currentDate,
        updated_at: currentDate,
    },
];

const devices = [
    {
        id: 'fede0f22-99ca-4e05-a355-397b5338fae4',
        organization_id: organizations[0].id,

        platform: Platform.YandexStation,
        device_id: '04007884c9145803048f',
        note: '...',

        created_at: currentDate,
        updated_at: currentDate,
    },
];

const users = [
    {
        login: 'sanya-rogov35',
        id: '4014931526',

        created_at: currentDate,
        updated_at: currentDate,
    },
];

const permissions: {
    organization_id: string;
    uid: number;
    type: string;
    source: Source;
}[] = [];
for (const organization of organizations) {
    for (const user of users) {
        for (const type of Object.keys(PermissionTypes)) {
            permissions.push({
                organization_id: organization.id,
                uid: parseInt(user.id, 10),
                type,
                source: Source.Native,
            });
        }
    }
}

export default {
    async up(queryInterface: QueryInterface) {
        await queryInterface.bulkInsert('connect_organizations', connectOrganizations);
        await queryInterface.bulkInsert('organizations', organizations);
        await queryInterface.bulkInsert('devices', devices);
        await queryInterface.bulkInsert('users', users);
        await queryInterface.bulkInsert('permissions', permissions);
    },
    async down(queryInterface: QueryInterface) {
        await queryInterface.bulkDelete('users', {
            id: users.map((user) => user.id),
        });
        await queryInterface.bulkDelete('organizations', {
            id: organizations.map((organization) => organization.id),
        });
    },
};
