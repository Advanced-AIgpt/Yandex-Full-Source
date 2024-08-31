import { DataTypes, QueryInterface } from 'sequelize';

enum DEVICES {
    _tableName = 'devices',

    preactivatedSkillIds = 'preactivated_skill_ids',
}

export default {
    async up(queryInterface: QueryInterface, Sequelize: typeof DataTypes) {
        await queryInterface.addColumn(DEVICES._tableName, DEVICES.preactivatedSkillIds, {
            type: Sequelize.ARRAY(Sequelize.STRING),
            allowNull: false,
            defaultValue: [],
        });
    },

    async down(queryInterface: QueryInterface) {
        await queryInterface.removeColumn(
            DEVICES._tableName,
            DEVICES.preactivatedSkillIds,
        );
    },
};
