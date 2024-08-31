import { DataTypes, QueryInterface } from 'sequelize';

const table = 'devices';
const attr = 'pending';

export default {
    async up(queryInterface: QueryInterface, Sequelize: typeof DataTypes) {
        const { BOOLEAN } = Sequelize;
        await queryInterface.addColumn(table, attr, {
            type: BOOLEAN,
            allowNull: false,
            defaultValue: false,
        });
    },

    async down(queryInterface: QueryInterface) {
        await queryInterface.removeColumn(table, attr);
    },
};
