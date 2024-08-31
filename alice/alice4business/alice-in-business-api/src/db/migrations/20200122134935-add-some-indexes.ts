import { QueryInterface } from 'sequelize';

enum DEVICES {
    _tableName = 'devices',

    deletedAt = 'deleted_at',
}

enum PROMO_CODES {
    _tableName = 'promo_codes',

    organizationId = 'organization_id',
    status = 'status',
    operationId = 'operation_id',
}

export default {
    async up(queryInterface: QueryInterface) {
        await queryInterface.addIndex(DEVICES._tableName, [DEVICES.deletedAt], {
            concurrently: true,
        });

        await queryInterface.addIndex(
            PROMO_CODES._tableName,
            [PROMO_CODES.organizationId, PROMO_CODES.status],
            { concurrently: true },
        );
    },

    async down(queryInterface: QueryInterface) {
        await queryInterface.removeIndex(DEVICES._tableName, [DEVICES.deletedAt]);

        await queryInterface.removeIndex(PROMO_CODES._tableName, [
            PROMO_CODES.organizationId,
            PROMO_CODES.status,
        ]);
    },
};
