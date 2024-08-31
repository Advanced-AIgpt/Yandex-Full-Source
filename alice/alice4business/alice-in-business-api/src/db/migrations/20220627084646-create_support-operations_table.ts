import { DataTypes, QueryInterface } from 'sequelize';

enum SUPPORT_OPERATIONS {
    _tableName = 'support_operations_audit_logs'
}

export default {
    async up(queryInterface: QueryInterface, Sequelize: typeof DataTypes) {
        const { DATE, UUID, UUIDV4, NOW, STRING, BOOLEAN } = Sequelize;

        await queryInterface.createTable(SUPPORT_OPERATIONS._tableName, {
            id: {
                type: UUID,
                primaryKey: true,
                defaultValue: UUIDV4,
            },
            operationType: {
                type: STRING,
                allowNull: false,
                field: 'operation_type',
            },
            succeed: {
                type: BOOLEAN,
                allowNull: false,
                defaultValue: false,
            },
            puid: {
                type: STRING,
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
            message: {
                type: STRING,
                allowNull: true,
            },
        });
    },

    async down(queryInterface: QueryInterface) {
        await queryInterface.dropTable(SUPPORT_OPERATIONS._tableName);
    },
};
