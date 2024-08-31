import { DataTypes, QueryInterface } from 'sequelize';

enum OPERATIONS {
    _tableName = 'operations',

    id = 'id',
}

enum PROMO_CODES {
    _tableName = 'promo_codes',

    operationId = 'operation_id',
}

export default {
    async up(queryInterface: QueryInterface, Sequelize: typeof DataTypes) {
        await queryInterface.addColumn(PROMO_CODES._tableName, PROMO_CODES.operationId, {
            type: Sequelize.UUID,
            allowNull: true,
            references: { model: OPERATIONS._tableName, key: OPERATIONS.id },
            onUpdate: 'CASCADE',
            onDelete: 'RESTRICT',
        });
    },

    async down(queryInterface: QueryInterface) {
        await queryInterface.removeColumn(
            PROMO_CODES._tableName,
            PROMO_CODES.operationId,
        );
    },
};
