import { DataTypes, QueryInterface } from 'sequelize';

const operationsParentIdIdx = 'operations_parent_id_idx';

export default {
    async up(queryInterface: QueryInterface, Sequelize: typeof DataTypes) {
        await queryInterface.addIndex('operations', {
            fields: ['parent_id'],
            name: operationsParentIdIdx,
        });
    },

    async down(queryInterface: QueryInterface) {
        await queryInterface.removeIndex('operations', operationsParentIdIdx);
    },
};
