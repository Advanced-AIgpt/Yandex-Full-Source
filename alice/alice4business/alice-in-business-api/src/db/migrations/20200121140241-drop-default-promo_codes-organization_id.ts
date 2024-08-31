import { QueryInterface } from 'sequelize';

enum PROMO_CODES {
    _tableName = 'promo_codes',

    organizationId = 'organization_id',
}

export default {
    async up(queryInterface: QueryInterface) {
        await queryInterface.sequelize.query(
            `ALTER TABLE "${PROMO_CODES._tableName}" ALTER COLUMN "${PROMO_CODES.organizationId}" DROP DEFAULT;`,
        );
    },

    async down() {},
};
