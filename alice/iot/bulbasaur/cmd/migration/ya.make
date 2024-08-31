OWNER(g:alice_iot)

RECURSE(
    # 01_tuya_daylight_migrate
    # 02_huid_column
    # 03_huid_shards_migrate
    # 04_external_users_table
    # 05_created_users_filler
    # 06_station_owners_table
    # 07_user_skills_table
    # 08_speaker_users_migrate
    # 09_add_updated_column
    # 10_fill_user_id
    # 11_eliminate_recursive_scenarios
    # 12_eliminate_scenarios_empty_capability
    #    13_set_updated_to_created
    #    14_add_properties_column
    #    15_set_empty_properties_column
    #    16_user_networks_table
    #    17_add_archived_at_column
    #    18_remove_device_groups_for_archived_devices
    #    19_scenarios_from_ea_to_capabilities
    #    20_scenarios_requested_speaker_capabilities
    #    21_tuya_external_user_ids_fix
    #    22_experiments_table
    #    23_fill_capability_for_speakers
    #    24_add_columns_for_timer_triggered_scenarios
    #    25_external_users_alter_table
    #    26_add_triggers_column_for_scenarios
    #    27_fill_scenario_triggers_column
    #    28_add_aliases_for_devices
    #    29_add_aliases_for_groups
    #    30_archive_stations_for_previous_owners
    #    31_normalize_voice_triggers_in_scenario
    #    32_scenario_launches_table
    #    33_scenario_is_active_column
    #    34_scenario_is_active_fill
    #    35_begemot_eliminate_recursive_scenarios
    #    36_scenario_launches_cleanup
    #    37_scenario_launches_launch_devices_update
    #    38_scenario_timers_cleanup
    #    39_scenario_redundant_columns_drop
    #    40_device_states_history_table
    #    41_household_id_column
    #    42_household_table
    #    43_current_household_id_column
    #    43_device_triggers_index_table
    #    44_first_household_creation
    #    45_household_id_filler
    #    46_scenario_launches_add_launch_trigger_value_column
    #    46_short_address_column
    #    47_homeless_rooms_groups_cleanup
    #    48_devices_add_status_column
    #    49_scenario_steps_column
    #    50_add_user_groups_for_experiments
    #    51_scenario_effective_time_column
    #    52_drop_table_station_owners
    #    53_scenario_launches_current_step_index_column
    #    54_history_table_fix_ttl
    #    55_stereopair_table
    #    56_favorites_table
    #    57_delete_draft_user_skills
    58_user_storage_table
    60_fill_quasar_capability_for_speakers
    61_null_zero_ttl_for_scenariolaunch
    62_enable_ttl_for_scenariolaunches
    63_devices_add_internal_config_column
    64_add_stereopair_created_time
    65_set_default_value_for_stereopair_created
    66_add_read_replicas
    67_scenarios_add_push_on_invoke
    68_stepify_and_revalidate_scenarios
    69_scenario_launches_add_push_on_invoke
    70_intent_states_table
    71_add_index_for_scenario_launches_status
    72_experiments_add_description_column
    73_shared_households_table
    74_remove_old_index_for_scenario_launches_status
    75_fill_colorsetting_capability_for_yandexmidi
    76_enable_ttl_for_intent_states
    77_drop_archived_for_one_user
    78_shared_invitations_tables
    79_shared_links_table
    80_shared_households_add_household_name_column
    dbmigration
)
