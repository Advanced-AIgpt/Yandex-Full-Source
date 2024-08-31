import { DataTypes, QueryInterface } from 'sequelize';

enum TABLE {
    devices = 'devices',
    operations = 'operations',
}

enum DEVICES {
    pendingOperationId = 'pending_operation_id',
    online = 'online',
    ownerId = 'owner_id',
}

enum OPERATIONS {
    id = 'id',
}

export default {
    async up(queryInterface: QueryInterface, Sequelize: typeof DataTypes) {
        await queryInterface.addColumn(TABLE.devices, DEVICES.online, {
            type: Sequelize.BOOLEAN,
            allowNull: false,
            defaultValue: false,
        });
        await queryInterface.addColumn(TABLE.devices, DEVICES.ownerId, {
            type: Sequelize.TEXT,
            allowNull: true,
        });
        await queryInterface.addColumn(TABLE.devices, DEVICES.pendingOperationId, {
            type: Sequelize.UUID,
            allowNull: true,
            references: { model: TABLE.operations, key: OPERATIONS.id },
        });
    },

    async down(queryInterface: QueryInterface) {
        await queryInterface.removeColumn(TABLE.devices, DEVICES.online);
        await queryInterface.removeColumn(TABLE.devices, DEVICES.ownerId);
        await queryInterface.removeColumn(TABLE.devices, DEVICES.pendingOperationId);
    },
};
