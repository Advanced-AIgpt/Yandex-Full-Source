import { Op, DataTypes, QueryInterface, BOOLEAN } from 'sequelize';

import { Status as OperationStatus } from '../tables/operation';
const externalRoomIdOrganizationUnique = 'external_room_id_organization_unique';
const operationsPendingRoomPk = 'operations_pending_room_pk';



export default {
    async up(queryInterface: QueryInterface, Sequelize: typeof DataTypes) {
        const { DATE, UUID, UUIDV4, TEXT, NOW } = Sequelize;

        await queryInterface.createTable('rooms', {
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
                unique: externalRoomIdOrganizationUnique,
            },
            name: {
                type: TEXT,
                allowNull: true
            },
            externalRoomId: {
                type: TEXT,
                allowNull: true,
                field: 'external_room_id',
            },
            activationId: {
                type: UUID,
                allowNull: true,
                field: 'activation_id',
            },
            lastActivationDate: {
                type: DATE,
                allowNull: true,
                field: 'last_activation_date'
            },
            createdAt: {
                allowNull: false,
                type: DATE,
                defaultValue: NOW,
                field: 'created_at'
            },
            updatedAt: {
                allowNull: false,
                type: DATE,
                defaultValue: NOW,
                field: 'updated_at'
            },
            deletedAt: {
                allowNull: true,
                type: DATE,
                field: 'deleted_at'
            }
        });

        await queryInterface.addColumn('devices', 'room_id', {
            type: UUID,
            allowNull: true,
            defaultValue: null,
            references: { model: 'rooms', key: 'id' }
        });

        await queryInterface.changeColumn('operations', 'device_pk', {
            type: UUID,
            allowNull: true
        });

        await queryInterface.addColumn('operations', 'room_pk', {
            type: UUID,
            allowNull: true,
            defaultValue: null,
            references: {model: 'rooms', key: 'id'}
        });

        await queryInterface.addColumn('operations', 'parent_id', {
            type: UUID,
            allowNull: true,
            defaultValue: null,
            references: {model: 'operations', key: 'id'}
        });

        await queryInterface.addColumn('organizations', 'uses_rooms', {
            type: BOOLEAN,
            allowNull: true,
            defaultValue: false,
        });

        await queryInterface.addColumn('organizations', 'image_url', {
            type: TEXT,
            allowNull: true
        });

        await queryInterface.addColumn('organizations', 'info_url', {
            type: TEXT,
            allowNull: true
        });

        await queryInterface.addColumn('organizations', 'info_title', {
            type: TEXT,
            allowNull: true
        });

        await queryInterface.addColumn('organizations', 'info_subtitle', {
            type: TEXT,
            allowNull: true
        });

        await queryInterface.addIndex('rooms',  {
            type: 'UNIQUE',
            fields: ['external_room_id', 'organization_id'],
            name: externalRoomIdOrganizationUnique,
            where: {
                deleted_at: { [Op.eq]: null },
            },
        });

        await queryInterface.addIndex('operations', {
            name: operationsPendingRoomPk,
            type: 'UNIQUE',
            fields: ['room_pk'],
            where: {
                status: { [Op.eq]: OperationStatus.Pending },
            },
            concurrently: true,
        });

        await queryInterface.removeConstraint('devices', 'devices_kolonkish_id_key').catch(() => {});
    },

    async down(queryInterface: QueryInterface) {
        await queryInterface.removeColumn('devices', 'room_id');
        await queryInterface.removeIndex('operations', operationsPendingRoomPk);
        await queryInterface.removeColumn('operations', 'room_pk');
        await queryInterface.removeColumn('operations', 'parent_id');
        await queryInterface.dropTable('rooms');
        await queryInterface.removeColumn('organizations', 'uses_rooms');
        await queryInterface.removeColumn('organizations', 'image_url');
        await queryInterface.removeColumn('organizations', 'info_url');
        await queryInterface.removeColumn('organizations', 'info_title');
        await queryInterface.removeColumn('organizations', 'info_subtitle');
    }
};
