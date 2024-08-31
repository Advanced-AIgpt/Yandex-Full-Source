import { DataTypes, QueryInterface } from 'sequelize';

enum ROLES {
    _tableName = 'roles',
    userId = 'uid',
}

enum USERS {
    _tableName = 'users',
    id = 'id',
}


export default {
    async up(queryInterface: QueryInterface, Sequelize: typeof DataTypes) {
        await queryInterface.createTable(ROLES._tableName, {
            uid: {
                type: Sequelize.STRING,
                allowNull: false,
                primaryKey: true,
                references: {
                    model: USERS._tableName,
                    key: USERS.id
                },
                onDelete: 'CASCADE',
                onUpdate: 'CASCADE',
            },
            role: {
                type: Sequelize.ENUM('admin', 'support'),
                allowNull: false,
                primaryKey: true,
            },
        });

        await queryInterface.addIndex(
            ROLES._tableName,
            [ROLES.userId],
        );
    },

    async down(queryInterface: QueryInterface) {
        await queryInterface.removeIndex(ROLES._tableName, [ROLES.userId]);
        await queryInterface.dropTable(ROLES._tableName);
    },
};
