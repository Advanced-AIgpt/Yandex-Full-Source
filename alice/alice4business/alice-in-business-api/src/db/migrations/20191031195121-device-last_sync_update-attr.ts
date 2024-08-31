import { DataTypes, QueryInterface } from 'sequelize';

const attr = 'last_sync_update';
const table = 'devices';

export default {
    async up(queryInterface: QueryInterface, Sequelize: typeof DataTypes) {
        const { DATE } = Sequelize;
        await queryInterface.addColumn(table, attr, {
            type: DATE,
            allowNull: true,
            field: attr,
        });
    },

    async down(queryInterface: QueryInterface, Sequelize: typeof DataTypes) {
        await queryInterface.removeColumn(table, attr);
    },
};
