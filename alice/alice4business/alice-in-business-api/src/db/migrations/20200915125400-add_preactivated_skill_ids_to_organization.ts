import { DataTypes, QueryInterface } from 'sequelize';

enum ORGANIZATIONS {
    _tableName = 'organizations',

    preactivatedSkillIds = 'preactivated_skill_ids',
}

export default {
    async up(queryInterface: QueryInterface, Sequelize: typeof DataTypes) {
        await queryInterface.addColumn(
            ORGANIZATIONS._tableName,
            ORGANIZATIONS.preactivatedSkillIds,
            {
                type: Sequelize.ARRAY(Sequelize.STRING),
                allowNull: false,
                defaultValue: [],
            },
        );
    },

    async down(queryInterface: QueryInterface) {
        await queryInterface.removeColumn(
            ORGANIZATIONS._tableName,
            ORGANIZATIONS.preactivatedSkillIds,
        );
    },
};
