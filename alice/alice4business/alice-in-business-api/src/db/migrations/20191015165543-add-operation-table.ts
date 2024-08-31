import { DataTypes, QueryInterface } from 'sequelize';

const table = 'operations';

export default {
    async up(queryInterface: QueryInterface, Sequelize: typeof DataTypes) {
        const { DATE, UUID, UUIDV4, NOW, STRING, JSONB, TEXT } = Sequelize;

        await queryInterface.createTable(table, {
            id: {
                type: UUID,
                primaryKey: true,
                defaultValue: UUIDV4,
            },

            type: {
                type: STRING,
                allowNull: false,
            },

            devicePk: {
                type: UUID,
                allowNull: false,
                references: { model: 'devices', key: 'id' },
                onDelete: 'cascade',
                field: 'device_pk',
            },

            error: {
                type: JSONB,
                allowNull: true,
            },

            status: {
                type: STRING,
                allowNull: false,
                defaultValue: 'pending',
            },

            payload: {
                type: JSONB,
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

        await queryInterface.addIndex(table, ['status'], {
            concurrently: true,
        } as any);
    },
    async down(queryInterface: QueryInterface) {
        await queryInterface.removeIndex(table, ['status'], {
            concurrently: true,
        } as any);
        await queryInterface.dropTable(table);
    },
};
