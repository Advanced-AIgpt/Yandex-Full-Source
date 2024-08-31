import { DataTypes, QueryInterface } from 'sequelize';

enum ORGANIZATIONS {
    _tableName = 'organizations',

    id = 'id',
}

enum PERMISSIONS {
    _tableName = 'permissions',
}

export default {
    async up(queryInterface: QueryInterface, Sequelize: typeof DataTypes) {
        await queryInterface.createTable(PERMISSIONS._tableName, {
            organizationId: {
                field: 'organization_id',
                type: Sequelize.UUID,
                allowNull: false,
                primaryKey: true,
                references: { model: ORGANIZATIONS._tableName, key: ORGANIZATIONS.id },
                onUpdate: 'CASCADE',
                onDelete: 'CASCADE',
            },
            uid: {
                type: Sequelize.BIGINT,
                allowNull: false,
                primaryKey: true,
            },
            type: {
                type: Sequelize.ENUM('view', 'edit', 'status', 'promocode'),
                allowNull: false,
                defaultValue: 'view',
                primaryKey: true,
            },
            source: {
                type: Sequelize.ENUM('native', 'connect'),
                allowNull: false,
                defaultValue: 'native',
                primaryKey: true,
            },
        });
    },

    async down(queryInterface: QueryInterface) {
        await queryInterface.dropTable(PERMISSIONS._tableName);
    },
};
