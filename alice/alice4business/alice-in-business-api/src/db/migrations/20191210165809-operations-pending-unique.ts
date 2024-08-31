import { Op, QueryInterface } from 'sequelize';
import { Status as OperationStatus } from '../tables/operation';

enum TABLE {
    operations = 'operations',
}

enum OPERATIONS {
    devicePk = 'device_pk',

    operationsPendingDevicePk = 'operations_pending_device_pk',
}

export default {
    async up(queryInterface: QueryInterface) {
        await queryInterface.addIndex(TABLE.operations, {
            name: OPERATIONS.operationsPendingDevicePk,

            type: 'UNIQUE',
            fields: [OPERATIONS.devicePk],
            where: {
                status: { [Op.eq]: OperationStatus.Pending },
            },

            concurrently: true,
        });
    },

    async down(queryInterface: QueryInterface) {
        await queryInterface.removeIndex(
            TABLE.operations,
            OPERATIONS.operationsPendingDevicePk,
        );
    },
};
