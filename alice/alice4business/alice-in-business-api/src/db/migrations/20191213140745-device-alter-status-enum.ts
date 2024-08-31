import { QueryInterface } from 'sequelize';

export default {
    async up(queryInterface: QueryInterface) {
        await queryInterface.sequelize.query(`
            BEGIN;
            ALTER TYPE enum_devices_status RENAME VALUE 'offline' TO 'reset';
            ALTER TABLE devices ALTER COLUMN status SET DEFAULT 'reset'::enum_devices_status;
            COMMIT;
        `);
    },

    async down(queryInterface: QueryInterface) {
        await queryInterface.sequelize.query(`
            BEGIN;
            ALTER TABLE devices ALTER COLUMN status SET DEFAULT 'inactive'::enum_devices_status;
            ALTER TYPE enum_devices_status RENAME VALUE 'reset' TO 'offline';
            COMMIT;
        `);
    },
};
