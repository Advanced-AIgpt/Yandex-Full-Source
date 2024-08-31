import { DataTypes, QueryInterface } from 'sequelize';

enum ORGANIZATIONS {
    _tableName = 'organizations',

    id = 'id',
}

enum USERS {
    _tableName = 'users',

    id = 'id',
}

enum ORGANIZATION_USERS {
    _tableName = 'organization_users',
}

export default {
    async up(queryInterface: QueryInterface) {
        await queryInterface.dropTable(ORGANIZATION_USERS._tableName);
    },

    async down(queryInterface: QueryInterface, Sequelize: typeof DataTypes) {
        await queryInterface.createTable(ORGANIZATION_USERS._tableName, {
            organizationId: {
                type: Sequelize.UUID,
                allowNull: false,
                references: { model: ORGANIZATIONS._tableName, key: ORGANIZATIONS.id },
                primaryKey: true,
                field: 'organization_id',
            },
            userId: {
                type: Sequelize.TEXT,
                allowNull: false,
                references: { model: USERS._tableName, key: USERS.id },
                primaryKey: true,
                field: 'user_id',
            },
            createdAt: {
                type: Sequelize.DATE,
                allowNull: false,
                defaultValue: Sequelize.NOW,
                field: 'created_at',
            },
            updatedAt: {
                type: Sequelize.DATE,
                allowNull: false,
                defaultValue: Sequelize.NOW,
                field: 'updated_at',
            },
        });

        await queryInterface.sequelize.query(`
            INSERT INTO organization_users (organization_id, user_id, created_at, updated_at)
            SELECT DISTINCT organization_id,
                            uid::text as user_id,
                            now()     as created_at,
                            now()     as updated_at
            FROM permissions
            WHERE source = 'native'
            ON CONFLICT DO NOTHING;
        `);
    },
};
