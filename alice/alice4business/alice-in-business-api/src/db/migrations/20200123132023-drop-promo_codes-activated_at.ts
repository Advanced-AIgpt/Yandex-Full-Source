import { DataTypes, QueryInterface } from 'sequelize';

enum PROMO_CODES {
    _tableName = 'promo_codes',

    activatedAt = 'activated_at',
}

export default {
    async up(queryInterface: QueryInterface) {
        await queryInterface.removeColumn(
            PROMO_CODES._tableName,
            PROMO_CODES.activatedAt,
        );
    },

    async down(queryInterface: QueryInterface, Sequelize: typeof DataTypes) {
        await queryInterface.addColumn(PROMO_CODES._tableName, PROMO_CODES.activatedAt, {
            type: Sequelize.DATE,
            allowNull: true,
            field: 'activated_at',
        });

        await queryInterface.sequelize.query(`
            UPDATE promo_codes SET activated_at = updated_at WHERE status = 'success';
        `);
    },
};
