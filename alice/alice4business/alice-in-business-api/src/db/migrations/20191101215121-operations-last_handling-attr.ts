import { DataTypes, QueryInterface } from 'sequelize';

const attr = 'last_handling';
const table = 'operations';
export default {
    async up(queryInterface: QueryInterface, Sequelize: typeof DataTypes) {
        const { DATE, NOW } = Sequelize;
        await queryInterface.addColumn(table, attr, {
            type: DATE,
            allowNull: true,
            defaultValue: NOW,
            field: 'last_handling',
        });
    },

    async down(queryInterface: QueryInterface, Sequelize: typeof DataTypes) {
        await queryInterface.removeColumn(table, attr);
    },
};
