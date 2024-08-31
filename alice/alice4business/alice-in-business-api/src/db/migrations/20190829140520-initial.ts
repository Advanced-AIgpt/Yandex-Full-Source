import { DataTypes, QueryInterface } from 'sequelize';

const status = {
    active: 'active',
    inactive: 'inactive',
    offline: 'offline',
};

const statuses = Object.values(status);
const platformDeviceIdUnique = 'platform_device_id_key';
const organizationUserUnique = 'organization_user_unique_key';
const externalDeviceIdOrganizationUnique = 'external_device_id_organization_unique';
export default {
    async up(queryInterface: QueryInterface, Sequelize: typeof DataTypes) {
        const { DATE, ENUM, UUID, UUIDV4, TEXT, NOW } = Sequelize;

        await queryInterface.createTable('users', {
            id: {
                type: TEXT,
                primaryKey: true,
            },
            login: {
                type: TEXT,
                allowNull: false,
            },
            createdAt: {
                type: DATE,
                allowNull: false,
                defaultValue: NOW,
                field: 'created_at',
            },
            updatedAt: {
                type: DATE,
                allowNull: false,
                defaultValue: NOW,
                field: 'updated_at',
            },
        });

        await queryInterface.createTable('organizations', {
            id: {
                type: UUID,
                primaryKey: true,
                defaultValue: UUIDV4,
            },
            name: {
                type: TEXT,
                allowNull: false,
            },
            createdAt: {
                type: DATE,
                allowNull: false,
                defaultValue: NOW,
                field: 'created_at',
            },
            updatedAt: {
                type: DATE,
                allowNull: false,
                defaultValue: NOW,
                field: 'updated_at',
            },
        });

        await queryInterface.createTable('organization_users', {
            organizationId: {
                type: UUID,
                allowNull: false,
                references: { model: 'organizations', key: 'id' },
                unique: organizationUserUnique,
                primaryKey: true,
                field: 'organization_id',
            },
            userId: {
                type: TEXT,
                allowNull: false,
                references: { model: 'users', key: 'id' },
                unique: organizationUserUnique,
                primaryKey: true,
                field: 'user_id',
            },
            createdAt: {
                type: DATE,
                allowNull: false,
                defaultValue: NOW,
                field: 'created_at',
            },
            updatedAt: {
                type: DATE,
                allowNull: false,
                defaultValue: NOW,
                field: 'updated_at',
            },
        });

        await queryInterface.addConstraint(
            'organization_users',
            ['organization_id', 'user_id'],
            {
                type: 'unique',
                name: organizationUserUnique,
            },
        );

        await queryInterface.createTable('devices', {
            id: {
                type: UUID,
                primaryKey: true,
                defaultValue: UUIDV4,
            },
            organizationId: {
                type: UUID,
                allowNull: false,
                references: { model: 'organizations', key: 'id' },
                field: 'organization_id',
                unique: externalDeviceIdOrganizationUnique,
            },
            platform: {
                type: TEXT,
                allowNull: false,
                unique: platformDeviceIdUnique,
            },
            deviceId: {
                type: TEXT,
                allowNull: false,
                field: 'device_id',
                unique: platformDeviceIdUnique,
            },
            externalDeviceId: {
                type: TEXT,
                allowNull: true,
                field: 'external_device_id',
                unique: externalDeviceIdOrganizationUnique,
            },
            kolonkishId: {
                type: TEXT,
                allowNull: true,
                field: 'kolonkish_id',
                unique: true,
            },
            kolonkishLogin: {
                type: TEXT,
                allowNull: true,
                field: 'kolonkish_login',
            },
            note: {
                type: TEXT,
                allowNull: true,
            },
            status: {
                type: ENUM(...statuses),
                allowNull: false,
                defaultValue: status.inactive,
            },
            createdAt: {
                type: DATE,
                allowNull: false,
                defaultValue: NOW,
                field: 'created_at',
            },
            updatedAt: {
                type: DATE,
                allowNull: false,
                defaultValue: NOW,
                field: 'updated_at',
            },
        });

        await queryInterface.addConstraint(
            'devices',
            ['external_device_id', 'organization_id'],
            {
                type: 'unique',
                name: externalDeviceIdOrganizationUnique,
            },
        );
        await queryInterface.addConstraint('devices', ['device_id', 'platform'], {
            type: 'unique',
            name: platformDeviceIdUnique,
        });
    },

    async down(queryInterface: QueryInterface) {
        await queryInterface.dropTable('devices');
        await queryInterface.dropTable('organization_users');
        await queryInterface.dropTable('organizations');
        await queryInterface.dropTable('users');
    },
};
