import { DataTypes, QueryInterface } from 'sequelize';

enum CONNECT_ORGANIZATIONS {
    _tableName = 'connect_organizations',

    id = 'id',
}

enum ORGANIZATIONS {
    _tableName = 'organizations',

    connectOrgId = 'connect_org_id',
}

export default {
    async up(queryInterface: QueryInterface, Sequelize: typeof DataTypes) {
        await queryInterface.createTable(CONNECT_ORGANIZATIONS._tableName, {
            id: {
                type: Sequelize.INTEGER,
                primaryKey: true,
            },
            name: {
                type: Sequelize.TEXT,
                allowNull: false,
                defaultValue: '',
            },
            active: {
                type: Sequelize.BOOLEAN,
                allowNull: false,
                defaultValue: true,
            },
            lastSync: {
                type: Sequelize.DATE,
                allowNull: true,
                field: 'last_sync',
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

        await queryInterface.addConstraint(
            ORGANIZATIONS._tableName,
            [ORGANIZATIONS.connectOrgId],
            {
                type: 'foreign key',
                name: `${ORGANIZATIONS._tableName}_${ORGANIZATIONS.connectOrgId}_fk`,
                references: {
                    table: CONNECT_ORGANIZATIONS._tableName,
                    field: CONNECT_ORGANIZATIONS.id,
                },
                onUpdate: 'CASCADE',
                onDelete: 'RESTRICT',
            },
        );
    },

    async down(queryInterface: QueryInterface, Sequelize: typeof DataTypes) {
        await queryInterface.removeConstraint(
            ORGANIZATIONS._tableName,
            `${ORGANIZATIONS._tableName}_${ORGANIZATIONS.connectOrgId}_fk`,
        );

        await queryInterface.dropTable(CONNECT_ORGANIZATIONS._tableName);
    },
};
