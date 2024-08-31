import { DataTypes, QueryInterface } from 'sequelize';

enum DEVICES {
    _tableName = 'devices',

    pendingOperationId = 'pending_operation_id',
    online = 'online',
    ownerId = 'owner_id',
}

enum OPERATIONS {
    _tableName = 'operations',

    id = 'id',
}

export default {
    async up(queryInterface: QueryInterface) {
        await queryInterface.removeColumn(DEVICES._tableName, DEVICES.pendingOperationId);
    },

    async down(queryInterface: QueryInterface, Sequelize: typeof DataTypes) {
        await queryInterface.addColumn(DEVICES._tableName, DEVICES.pendingOperationId, {
            type: Sequelize.UUID,
            allowNull: true,
            references: { model: OPERATIONS._tableName, key: OPERATIONS.id },
        });
    },
};
