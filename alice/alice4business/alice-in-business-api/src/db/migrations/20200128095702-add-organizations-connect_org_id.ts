import { DataTypes, QueryInterface } from 'sequelize';

enum ORGANIZATIONS {
    _tableName = 'organizations',

    name = 'name',
    connectOrgId = 'connect_org_id',
}

export default {
    async up(queryInterface: QueryInterface, Sequelize: typeof DataTypes) {
        await queryInterface.addColumn(
            ORGANIZATIONS._tableName,
            ORGANIZATIONS.connectOrgId,
            {
                type: Sequelize.INTEGER,
                allowNull: true,
                defaultValue: null,
            },
        );

        await queryInterface.addConstraint(
            ORGANIZATIONS._tableName,
            [ORGANIZATIONS.connectOrgId, ORGANIZATIONS.name],
            {
                type: 'unique',
            },
        );
    },

    async down(queryInterface: QueryInterface) {
        await queryInterface.removeColumn(
            ORGANIZATIONS._tableName,
            ORGANIZATIONS.connectOrgId,
        );
    },
};
