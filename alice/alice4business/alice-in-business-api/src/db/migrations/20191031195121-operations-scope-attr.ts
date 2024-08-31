import { DataTypes, QueryInterface } from 'sequelize';

const attr = 'scope';
const table = 'operations';

export default {
    async up(queryInterface: QueryInterface, Sequelize: typeof DataTypes) {
        const { JSONB } = Sequelize;
        await queryInterface.addColumn(table, attr, {
            type: JSONB,
            allowNull: true,
            field: attr,
        });
    },

    async down(queryInterface: QueryInterface, Sequelize: typeof DataTypes) {
        await queryInterface.removeColumn(table, attr);
    },
};
