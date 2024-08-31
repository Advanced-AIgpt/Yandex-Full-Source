import { DataTypes, QueryInterface } from 'sequelize';

enum TABLE {
    devices = 'devices',
}

enum DEVICES {
    lastSyncStart = 'last_sync_start',
}

export default {
    async up(queryInterface: QueryInterface, Sequelize: typeof DataTypes) {
        await queryInterface.addColumn(TABLE.devices, DEVICES.lastSyncStart, {
            type: Sequelize.DATE,
            allowNull: false,
            defaultValue: 0,
        });
    },

    async down(queryInterface: QueryInterface) {
        await queryInterface.removeColumn(TABLE.devices, DEVICES.lastSyncStart);
    },
};
