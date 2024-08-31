import { DataTypes, QueryInterface } from 'sequelize';

const table = 'promo_codes';

export default {
    async up(queryInterface: QueryInterface, Sequelize: typeof DataTypes) {
        const { DATE, UUID, UUIDV4, NOW, INTEGER, STRING, BOOLEAN } = Sequelize;

        await queryInterface.createTable(table, {
            id: {
                type: UUID,
                primaryKey: true,
                defaultValue: UUIDV4,
            },

            code: {
                type: STRING,
                allowNull: false,
                unique: true,
            },

            activatedAt: {
                type: DATE,
                allowNull: true,
                field: 'activated_at',
            },
            reject: {
                type: BOOLEAN,
                allowNull: false,
                defaultValue: false,
            },
            // Здесь храним пользователя НА который активировали промокод
            userId: {
                unique: true,
                type: STRING,
                field: 'user_id',
                allowNull: true,
            },
            createdAt: {
                type: DATE,
                allowNull: false,
                defaultValue: NOW,
                field: 'created_at',
            },
            updatedAt: {
                type: DATE,
                allowNull: false,
                defaultValue: NOW,
                field: 'updated_at',
            },
        });
    },
    async down(queryInterface: QueryInterface) {
        await queryInterface.dropTable(table);
    },
};
