import { DataTypes, QueryInterface } from "sequelize/types";


export default {
    async up(queryInterface: QueryInterface, Sequelize: typeof DataTypes) {
        const { DATE, UUID, UUIDV4, TEXT, NOW, BOOLEAN } = Sequelize;
        await queryInterface.createTable('activation_links', {
            id: {
                type: UUID,
                primaryKey: true,
                defaultValue: UUIDV4,
            },
            deviceId: {
                type: UUID,
                allowNull: true,
                references: { model: 'devices', key: 'id' },
                field: 'device_id',
            },
            roomId: {
                type: UUID,
                allowNull: true,
                references: { model: 'rooms', key: 'id' },
                field: 'room_id',
            },
            enabled: {
                type: BOOLEAN,
                allowNull: false,
            },
            promoActivated: {
                type: BOOLEAN,
                defaultValue: false,
                allowNull: false,
                field: 'promo_activated'
            },
            activeSince: {
                type: DATE,
                allowNull: false,
                field: 'active_since',
            },
            activeTill: {
                type: DATE,
                allowNull: false,
                field: 'active_till',
            },
            createdAt: {
                allowNull: false,
                type: DATE,
                defaultValue: NOW,
                field: 'created_at'
            },
        });

        await queryInterface.removeColumn('rooms', 'activation_id');
        await queryInterface.removeColumn('rooms', 'last_activation_date');
    },

    async down(queryInterface: QueryInterface, Sequelize: typeof DataTypes) {
        const { DATE, UUID } = Sequelize;
        await queryInterface.addColumn('rooms', 'activation_id', {
            type: UUID,
            allowNull: true,
        });
        await queryInterface.addColumn('rooms', 'last_activation_date', {
            type: DATE,
            allowNull: true,
        });
        await queryInterface.dropTable('activation_links');
    }
}