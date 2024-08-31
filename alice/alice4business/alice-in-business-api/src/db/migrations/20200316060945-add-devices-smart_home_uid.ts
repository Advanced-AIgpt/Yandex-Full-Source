import { DataTypes, QueryInterface } from 'sequelize';

enum DEVICES {
    _tableName = 'devices',

    smartHomeUid = 'smart_home_uid',
}

export default {
    async up(queryInterface: QueryInterface, Sequelize: typeof DataTypes) {
        await queryInterface.addColumn(DEVICES._tableName, DEVICES.smartHomeUid, {
            type: Sequelize.BIGINT,
            allowNull: true,
            defaultValue: null,
        });
    },

    async down(queryInterface: QueryInterface) {
        await queryInterface.removeColumn(DEVICES._tableName, DEVICES.smartHomeUid);
    },
};
