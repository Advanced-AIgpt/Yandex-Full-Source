import { QueryInterface } from 'sequelize';

export default {
    async up(queryInterface: QueryInterface) {
        await queryInterface.sequelize.query(`
            UPDATE promo_codes pc
            SET operation_id = (
                    SELECT id
                    FROM operations o
                    WHERE
                        o.type = 'promo-activate' AND
                        o.status = 'resolved' AND
                        o.payload ->> 'kolonkishUid' = pc.user_id
            )
            WHERE
                pc.status IS NOT NULL AND
                pc.user_id IS NOT NULL AND
                pc.operation_id IS NULL;
        `);
    },

    async down() {},
};
