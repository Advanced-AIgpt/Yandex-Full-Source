import { DataTypes, QueryInterface } from 'sequelize';

export default {
    async up(queryInterface: QueryInterface, Sequelize: typeof DataTypes) {
        await queryInterface.sequelize.query(`
            ALTER TABLE promo_codes
            DROP CONSTRAINT promo_codes_user_id_key
        `);
    },

    async down(queryInterface: QueryInterface, Sequelize: typeof DataTypes) {
        await queryInterface.sequelize.query(`
            ALTER TABLE promo_codes
            ADD CONSTRAINT promo_codes_user_id_key UNIQUE (user_id)
        `);
    },
};
