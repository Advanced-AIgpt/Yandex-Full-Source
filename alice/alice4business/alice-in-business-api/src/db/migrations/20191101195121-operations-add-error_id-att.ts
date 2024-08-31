import { DataTypes, QueryInterface } from 'sequelize';

const attr = 'error_id';
const table = 'operations';

export default {
    async up(queryInterface: QueryInterface, Sequelize: typeof DataTypes) {
        const { UUID } = Sequelize;
        await queryInterface.addColumn(table, attr, {
            type: UUID,
            allowNull: true,
            field: 'error_id',
        });
        // await queryInterface.removeColumn(table, 'error');
    },

    async down(queryInterface: QueryInterface, Sequelize: typeof DataTypes) {
        await queryInterface.removeColumn(table, attr);
    },
};
