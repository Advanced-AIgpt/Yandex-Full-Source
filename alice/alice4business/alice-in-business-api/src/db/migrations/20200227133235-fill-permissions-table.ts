import { QueryInterface } from 'sequelize';

export default {
    async up(queryInterface: QueryInterface) {
        await queryInterface.sequelize.query(`
            INSERT INTO permissions (organization_id, uid, type, source)
            SELECT organization_id,
                   user_id::int8                                                             as uid,
                   unnest(ARRAY ['view','edit','status','promocode'])::enum_permissions_type as type,
                   'native'                                                                  as source
            FROM organization_users
            ON CONFLICT DO NOTHING;
        `);
    },

    async down() {},
};
