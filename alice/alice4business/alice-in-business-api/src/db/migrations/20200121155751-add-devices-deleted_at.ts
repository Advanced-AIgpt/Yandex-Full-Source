import { DataTypes, Op, QueryInterface } from 'sequelize';

enum DEVICES {
    _tableName = 'devices',

    organizationId = 'organization_id',
    platform = 'platform',
    deviceId = 'device_id',
    externalDeviceId = 'external_device_id',
    deletedAt = 'deleted_at',

    unique_platform_deviceId = 'platform_device_id_key',
    unique_externalDeviceId_organizationId = 'external_device_id_organization_unique',
}

export default {
    async up(queryInterface: QueryInterface, Sequelize: typeof DataTypes) {
        await queryInterface.addColumn(DEVICES._tableName, DEVICES.deletedAt, {
            type: Sequelize.DATE,
            allowNull: true,
            defaultValue: null,
        });

        await queryInterface.sequelize.transaction(async (transaction) => {
            await queryInterface.removeConstraint(
                DEVICES._tableName,
                DEVICES.unique_platform_deviceId,
                { transaction },
            );
            await queryInterface.addIndex(DEVICES._tableName, {
                name: DEVICES.unique_platform_deviceId,

                type: 'UNIQUE',
                fields: [DEVICES.platform, DEVICES.deviceId],
                where: {
                    [DEVICES.deletedAt]: { [Op.eq]: null },
                },

                // ts hack
                ...{ transaction },
            });
        });

        await queryInterface.sequelize.transaction(async (transaction) => {
            await queryInterface.removeConstraint(
                DEVICES._tableName,
                DEVICES.unique_externalDeviceId_organizationId,
                { transaction },
            );
            await queryInterface.addIndex(DEVICES._tableName, {
                name: DEVICES.unique_externalDeviceId_organizationId,

                type: 'UNIQUE',
                fields: [DEVICES.externalDeviceId, DEVICES.organizationId],
                where: {
                    [DEVICES.deletedAt]: { [Op.eq]: null },
                },

                // ts hack
                ...{ transaction },
            });
        });
    },

    async down(queryInterface: QueryInterface) {
        await queryInterface.sequelize.transaction(async (transaction) => {
            await queryInterface.removeIndex(
                DEVICES._tableName,
                DEVICES.unique_platform_deviceId,
                { transaction },
            );

            await queryInterface.bulkDelete(
                DEVICES._tableName,
                {
                    [DEVICES.deletedAt]: { [Op.ne]: null },
                },
                { transaction },
            );

            await queryInterface.addConstraint(
                DEVICES._tableName,
                [DEVICES.platform, DEVICES.deviceId],
                {
                    type: 'unique',
                    name: DEVICES.unique_platform_deviceId,

                    // ts hack
                    ...{ transaction },
                },
            );
        });

        await queryInterface.sequelize.transaction(async (transaction) => {
            await queryInterface.removeIndex(
                DEVICES._tableName,
                DEVICES.unique_externalDeviceId_organizationId,
                { transaction },
            );
            await queryInterface.addConstraint(
                DEVICES._tableName,
                [DEVICES.organizationId, DEVICES.externalDeviceId],
                {
                    type: 'unique',
                    name: DEVICES.unique_externalDeviceId_organizationId,

                    // ts hack
                    ...{ transaction },
                },
            );
        });

        await queryInterface.removeColumn(DEVICES._tableName, DEVICES.deletedAt);
    },
};
