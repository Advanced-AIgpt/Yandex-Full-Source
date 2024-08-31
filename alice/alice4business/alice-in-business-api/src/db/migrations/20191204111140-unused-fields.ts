import { DataTypes, QueryInterface } from 'sequelize';

enum TABLE {
    devices = 'devices',
    operations = 'operations',
    organizationUsers = 'organization_users',
}

enum DEVICES {
    pending = 'pending',
}

enum OPERATIONS {
    errorId = 'error_id',
}

enum ORGANIZATION_USERS {
    organizationId = 'organization_id',
    userId = 'user_id',

    organizationUserUniqueKey = 'organization_user_unique_key',
}

export default {
    async up(queryInterface: QueryInterface) {
        await queryInterface.removeColumn(TABLE.devices, DEVICES.pending);
        await queryInterface.removeColumn(TABLE.operations, OPERATIONS.errorId);
        await queryInterface.removeConstraint(
            TABLE.organizationUsers,
            ORGANIZATION_USERS.organizationUserUniqueKey,
        );
    },

    async down(queryInterface: QueryInterface, Sequelize: typeof DataTypes) {
        await queryInterface.addColumn(TABLE.devices, DEVICES.pending, {
            type: Sequelize.BOOLEAN,
            allowNull: false,
            defaultValue: false,
        });
        await queryInterface.addColumn(TABLE.operations, OPERATIONS.errorId, {
            type: Sequelize.UUID,
            allowNull: true,
            field: 'error_id',
        });
        await queryInterface.addConstraint(
            TABLE.organizationUsers,
            [ORGANIZATION_USERS.organizationId, ORGANIZATION_USERS.userId],
            {
                type: 'unique',
                name: ORGANIZATION_USERS.organizationUserUniqueKey,
            },
        );
    },
};
