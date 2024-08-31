import { QueryInterface } from 'sequelize';

const operationsToYtView = 'operations_to_yt';

export default {
    async up(queryInterface: QueryInterface) {
        await queryInterface.sequelize.query(`CREATE OR REPLACE VIEW ${operationsToYtView}
            (id, type, device_pk, room_pk, is_error, status, payload, created_at, updated_at, scope, last_handling, error_details) AS
            SELECT operations.id,
                operations.type,
                operations.device_pk,
                operations.room_pk,
                operations.error IS NOT NULL AS is_error,
                operations.status,
                operations.payload,
                operations.created_at,
                operations.updated_at,
                operations.scope,
                operations.last_handling,
                CASE WHEN operations.error is NOT NULL THEN
                    jsonb_build_object(
                        'name', error -> 'name',
                        'stack', error -> 'stack',
                        'message', error -> 'message',
                        'path', error -> 'path',
                        'statusCode', error -> 'statusCode',
                        'payload', error -> 'payload')
                END AS error_details
            FROM operations`);
    },

    async down(queryInterface: QueryInterface) {
        await queryInterface.sequelize.query(`DROP VIEW ${operationsToYtView}`);
    },
};
