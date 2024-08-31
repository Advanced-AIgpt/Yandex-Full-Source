import { DataTypes, QueryInterface } from 'sequelize';

enum PROMOCODES {
    _tableName = 'promo_codes',

    ticketKey = 'ticket_key',
}

export default {
    async up(queryInterface: QueryInterface, Sequelize: typeof DataTypes) {
        await queryInterface.addColumn(PROMOCODES._tableName, PROMOCODES.ticketKey, {
            type: Sequelize.STRING,
            allowNull: false,
            defaultValue: '',
        });
    },

    async down(queryInterface: QueryInterface) {
        await queryInterface.removeColumn(PROMOCODES._tableName, PROMOCODES.ticketKey);
    },
};
