import { DataTypes, QueryInterface } from 'sequelize';

enum ACTIVATION_CODES {
    _tableName = 'activation_codes',
}
enum DEVICES {
    _tableName = 'devices',

    id = 'id',
}

export default {
    async up(queryInterface: QueryInterface, Sequelize: typeof DataTypes) {
        await queryInterface.createTable(ACTIVATION_CODES._tableName, {
            code: {
                type: Sequelize.STRING,
                allowNull: false,
                primaryKey: true,
            },
            deviceId: {
                field: 'device_id',
                type: Sequelize.UUID,
                allowNull: false,
                references: { model: DEVICES._tableName, key: DEVICES.id },
            },
            createdAt: {
                field: 'created_at',
                type: Sequelize.DATE,
                allowNull: false,
                defaultValue: Sequelize.NOW,
            },
        });

        await queryInterface.addIndex(ACTIVATION_CODES._tableName, [
            'device_id',
            'created_at',
        ]);
    },

    async down(queryInterface: QueryInterface) {
        await queryInterface.dropTable(ACTIVATION_CODES._tableName);
    },
};
