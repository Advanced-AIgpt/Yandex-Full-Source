import { DataTypes, QueryInterface } from 'sequelize';

enum DEVICES {
    _tableName = 'devices',

    agreementAccepted = 'agreement_accepted',
}

export default {
    async up(queryInterface: QueryInterface, Sequelize: typeof DataTypes) {
        await queryInterface.addColumn(DEVICES._tableName, DEVICES.agreementAccepted, {
            type: Sequelize.BOOLEAN,
            allowNull: false,
            defaultValue: false,
        });
    },

    async down(queryInterface: QueryInterface) {
        await queryInterface.removeColumn(DEVICES._tableName, DEVICES.agreementAccepted);
    },
};
