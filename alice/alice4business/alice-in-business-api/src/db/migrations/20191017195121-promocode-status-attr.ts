import { DataTypes, QueryInterface } from 'sequelize';

const table = 'promo_codes';
const attr = 'status';

export default {
    async up(queryInterface: QueryInterface, Sequelize: typeof DataTypes) {
        const { TEXT } = Sequelize;

        await queryInterface.addColumn(table, attr, {
            type: TEXT,
            allowNull: true,
            defaultValue: null,
        });
        await queryInterface.removeColumn(table, 'reject');
    },

    async down(queryInterface: QueryInterface, Sequelize: typeof DataTypes) {
        await queryInterface.removeColumn(table, attr);
    },
};
