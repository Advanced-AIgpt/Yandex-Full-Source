import { QueryInterface } from "sequelize";

export default {
    async up(queryInterface: QueryInterface) {
        await queryInterface.sequelize.transaction(async (transaction) => {
            // 1. rooms (organization_id) → organizations (id)
            await queryInterface.removeConstraint(
                "rooms",
                "rooms_organization_id_fkey",
                {
                    transaction
                });
            await queryInterface.addConstraint(
                "rooms",
                ["organization_id"],
                {
                    type: "foreign key",
                    name: "rooms_organization_id_fkey",
                    references: {
                        table: "organizations",
                        field: "id",
                    },
                    onUpdate: "CASCADE",
                    onDelete: "CASCADE",

                    transaction,
                },
            );
            // 2. operations (room_pk) → rooms (id)
            await queryInterface.removeConstraint(
                "operations",
                "operations_room_pk_fkey", {
                transaction
            });
            await queryInterface.addConstraint(
                "operations",
                ["room_pk"],
                {
                    type: "foreign key",
                    name: "operations_room_pk_fkey",
                    references: {
                        table: "rooms",
                        field: "id",
                    },
                    onUpdate: "CASCADE",
                    onDelete: "CASCADE",
                    transaction
                },
            );
            // 3. operations (parent_id) → operations (id)
            await queryInterface.removeConstraint(
                "operations",
                "operations_parent_id_fkey", {
                transaction
            });
            await queryInterface.addConstraint(
                "operations",
                ["parent_id"],
                {
                    type: "foreign key",
                    name: "operations_parent_id_fkey",
                    references: {
                        table: "operations",
                        field: "id",
                    },
                    onUpdate: "CASCADE",
                    onDelete: "CASCADE",
                    transaction
                },
            );
        });
    },

    async down(queryInterface: QueryInterface) {
        await queryInterface.sequelize.transaction(async (transaction) => {
            // 1. rooms (organization_id) → organizations (id)
            await queryInterface.removeConstraint(
                "rooms",
                "rooms_organization_id_fkey",
                {
                    transaction
                });
            await queryInterface.addConstraint(
                "rooms",
                ["organization_id"],
                {
                    type: "foreign key",
                    name: "rooms_organization_id_fkey",
                    references: {
                        table: "organizations",
                        field: "id",
                    },
                    onUpdate: "",
                    onDelete: "",

                    transaction,
                },
            );
            // 2. operations (room_pk) → rooms (id)
            await queryInterface.removeConstraint(
                "operations",
                "operations_room_pk_fkey", {
                transaction
            });
            await queryInterface.addConstraint(
                "operations",
                ["room_pk"],
                {
                    type: "foreign key",
                    name: "operations_room_pk_fkey",
                    references: {
                        table: "rooms",
                        field: "id",
                    },
                    onUpdate: "",
                    onDelete: "",
                    transaction
                },
            );
            // 3. operations (parent_id) → operations (id)
            await queryInterface.removeConstraint(
                "operations",
                "operations_parent_id_fkey", {
                transaction
            });
            await queryInterface.addConstraint(
                "operations",
                ["parent_id"],
                {
                    type: "foreign key",
                    name: "operations_parent_id_fkey",
                    references: {
                        table: "operations",
                        field: "id",
                    },
                    onUpdate: "",
                    onDelete: "",
                    transaction
                },
            );
        });
    }
}