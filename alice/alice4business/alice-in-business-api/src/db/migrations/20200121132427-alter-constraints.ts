import { QueryInterface } from 'sequelize';

enum DEVICES {
    _tableName = 'devices',

    id = 'id',
    organizationId = 'organization_id',

    foreignKey_organizationId = 'devices_organization_id_fkey',
}

enum OPERATIONS {
    _tableName = 'operations',

    devicePk = 'device_pk',

    foreignKey_devicePk = 'operations_device_pk_fkey',
}

enum ORGANIZATION_USERS {
    _tableName = 'organization_users',

    organizationId = 'organization_id',
    userId = 'user_id',

    foreignKey_organizationId = 'organization_users_organization_id_fkey',
    foreignKey_userId = 'organization_users_user_id_fkey',
}

enum ORGANIZATIONS {
    _tableName = 'organizations',

    id = 'id',
}

enum PROMO_CODES {
    _tableName = 'promo_codes',

    organizationId = 'organization_id',

    foreignKey_organizationId = 'promo_codes_organization_id_fkey',
    foreignKey_organizationId_2 = 'promo_codes_organization_id_fkey1',
    foreignKey_organizationId_3 = 'organization_id_foreign_idx',
}

enum USERS {
    _tableName = 'users',

    id = 'id',
}

export default {
    async up(queryInterface: QueryInterface) {
        // 1. devices (organization_id) → organizations (id)
        await queryInterface.sequelize.transaction(async (transaction) => {
            await queryInterface.removeConstraint(
                DEVICES._tableName,
                DEVICES.foreignKey_organizationId,
                { transaction },
            );
            await queryInterface.addConstraint(
                DEVICES._tableName,
                [DEVICES.organizationId],
                {
                    type: 'foreign key',
                    name: DEVICES.foreignKey_organizationId,
                    references: {
                        table: ORGANIZATIONS._tableName,
                        field: ORGANIZATIONS.id,
                    },
                    onUpdate: 'CASCADE',
                    onDelete: 'CASCADE',

                    // @ts-ignore
                    transaction,
                },
            );
        });

        // 2. operations (device_pk) → devices (id)
        await queryInterface.sequelize.transaction(async (transaction) => {
            await queryInterface.removeConstraint(
                OPERATIONS._tableName,
                OPERATIONS.foreignKey_devicePk,
                { transaction },
            );
            await queryInterface.addConstraint(
                OPERATIONS._tableName,
                [OPERATIONS.devicePk],
                {
                    type: 'foreign key',
                    name: OPERATIONS.foreignKey_devicePk,
                    references: {
                        table: DEVICES._tableName,
                        field: DEVICES.id,
                    },
                    onUpdate: 'CASCADE',
                    onDelete: 'CASCADE',

                    // @ts-ignore
                    transaction,
                },
            );
        });

        // 3. organization_users (organization_id) → organizations (id)
        await queryInterface.sequelize.transaction(async (transaction) => {
            await queryInterface.removeConstraint(
                ORGANIZATION_USERS._tableName,
                ORGANIZATION_USERS.foreignKey_organizationId,
                { transaction },
            );
            await queryInterface.addConstraint(
                ORGANIZATION_USERS._tableName,
                [ORGANIZATION_USERS.organizationId],
                {
                    type: 'foreign key',
                    name: ORGANIZATION_USERS.foreignKey_organizationId,
                    references: {
                        table: ORGANIZATIONS._tableName,
                        field: ORGANIZATIONS.id,
                    },
                    onUpdate: 'CASCADE',
                    onDelete: 'CASCADE',

                    // @ts-ignore
                    transaction,
                },
            );
        });

        // 4. organization_users (user_id) → users (id)
        await queryInterface.sequelize.transaction(async (transaction) => {
            await queryInterface.removeConstraint(
                ORGANIZATION_USERS._tableName,
                ORGANIZATION_USERS.foreignKey_userId,
                { transaction },
            );
            await queryInterface.addConstraint(
                ORGANIZATION_USERS._tableName,
                [ORGANIZATION_USERS.userId],
                {
                    type: 'foreign key',
                    name: ORGANIZATION_USERS.foreignKey_userId,
                    references: {
                        table: USERS._tableName,
                        field: USERS.id,
                    },
                    onUpdate: 'CASCADE',
                    onDelete: 'CASCADE',

                    // @ts-ignore
                    transaction,
                },
            );
        });

        // 5. promo_codes (organization_id) → organizations (id)
        await queryInterface.sequelize.transaction(async (transaction) => {
            await queryInterface.removeConstraint(
                PROMO_CODES._tableName,
                PROMO_CODES.foreignKey_organizationId,
                { transaction },
            );
            await queryInterface.addConstraint(
                PROMO_CODES._tableName,
                [PROMO_CODES.organizationId],
                {
                    type: 'foreign key',
                    name: PROMO_CODES.foreignKey_organizationId,
                    references: {
                        table: ORGANIZATIONS._tableName,
                        field: ORGANIZATIONS.id,
                    },
                    onUpdate: 'CASCADE',
                    onDelete: 'CASCADE',

                    // @ts-ignore
                    transaction,
                },
            );
        });

        try {
            await queryInterface.removeConstraint(
                PROMO_CODES._tableName,
                PROMO_CODES.foreignKey_organizationId_2,
            );
        } catch (e) {}
        try {
            // due to different naming in sequelize@4 and sequelize@5
            await queryInterface.removeConstraint(
                PROMO_CODES._tableName,
                PROMO_CODES.foreignKey_organizationId_3,
            );
        } catch (e) {}
    },

    async down(queryInterface: QueryInterface) {
        await queryInterface.addConstraint(
            PROMO_CODES._tableName,
            [PROMO_CODES.organizationId],
            {
                type: 'foreign key',
                name: PROMO_CODES.foreignKey_organizationId_2,
                references: {
                    table: ORGANIZATIONS._tableName,
                    field: ORGANIZATIONS.id,
                },
                onUpdate: '',
                onDelete: '',
            },
        );
    },
};
