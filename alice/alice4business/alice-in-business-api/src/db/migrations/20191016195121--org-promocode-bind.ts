import { DataTypes, QueryInterface } from 'sequelize';

const table = 'promo_codes';
const attr = 'organization_id';

export default {
    async up(queryInterface: QueryInterface, dataTypes: typeof DataTypes) {
        await queryInterface.addColumn(table, attr, {
            type: dataTypes.UUID,
            allowNull: false,
            references: { model: 'organizations', key: 'id' },
            field: attr,
        });

        await queryInterface.changeColumn(table, attr, {
            type: dataTypes.UUID,
            allowNull: false,
            references: { model: 'organizations', key: 'id' },
            field: attr,
        });

        await queryInterface.addIndex(table, ['user_id'], { concurrently: true } as any);
    },

    async down(queryInterface: QueryInterface, Sequelize: typeof DataTypes) {
        await queryInterface.removeIndex(table, ['user_id'], {
            concurrently: true,
        } as any);
        await queryInterface.removeColumn(table, attr);
    },
};
