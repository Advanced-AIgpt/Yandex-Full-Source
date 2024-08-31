import { DataTypes, QueryInterface } from 'sequelize';

enum ORGANIZATIONS {
    _tableName = 'organizations',

    maxVolumeColumnName = 'max_station_volume',
}

export default {
    async up(queryInterface: QueryInterface, Sequelize: typeof DataTypes) {
        await queryInterface.addColumn(ORGANIZATIONS._tableName, ORGANIZATIONS.maxVolumeColumnName, {
            type: Sequelize.INTEGER,
            allowNull: true,
            defaultValue: null,
        });
    },

    async down(queryInterface: QueryInterface) {
        await queryInterface.removeColumn(ORGANIZATIONS._tableName, ORGANIZATIONS.maxVolumeColumnName);
    },
};