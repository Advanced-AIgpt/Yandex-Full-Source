import { DataTypes, QueryInterface } from 'sequelize';

const attr = 'template_url';
const table = 'organizations';
export default {
    async up(queryInterface: QueryInterface, Sequelize: typeof DataTypes) {
        await queryInterface.addColumn(table, attr, {
            type: Sequelize.TEXT,
            allowNull: true,
        });
    },

    async down(queryInterface: QueryInterface) {
        await queryInterface.removeColumn(table, attr);
    },
};
