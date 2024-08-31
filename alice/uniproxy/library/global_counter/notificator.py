from alice.uniproxy.library.global_counter import GlobalCounter, GlobalTimings, TimingsResolution


class NotificatorCounter:

    __counters = [
        'handler_delivery_reqs_summ',
        'handler_delivery_reqs_dropped_summ',
        'handler_delivery_sup_reqs_summ',
        'handler_delivery_sup_card_reqs_summ',
        'handler_delivery_demo_reqs_summ',
        'handler_delivery_on_connect_reqs_summ',
        'handler_delivery_directive_reqs_summ',
        'handler_list_subscribed_users_reqs_summ',
        'handler_manage_subscription_reqs_summ',
        'handler_subscribes_get_reqs_summ',
        'handler_notifications_reqs_summ',
        'handler_notifications_reqs_dropped_summ',
        'handler_notifications_archive_reqs_summ',
        'handler_notifications_change_status_reqs_summ',
        'handler_locator_reqs_summ',
        'handler_locator_reqs_dropped_summ',
        'handler_device_subscriptions_reqs_summ',
        'handler_dropped_reqs_summ',
        'handler_gdpr_reqs_summ',
        'handler_directives_change_status_reqs_summ',
        'handler_directive_status_reqs_summ',
        'handler_devices_get_reqs_summ',
        'handler_delete_personal_cards_reqs_summ',

        'push_messages_recv_summ',
        'push_messages_sent_summ',
        'push_messages_fail_summ',
        'push_messages_no_locations_summ',

        'push_sup_messages_recv_summ',
        'push_sup_messages_sent_summ',
        'push_sup_messages_fail_summ',

        'push_sup_card_messages_recv_summ',
        'push_sup_card_messages_sent_summ',
        'push_sup_card_messages_fail_summ',

        'push_on_connect_ok_summ',
        'push_on_connect_fail_summ',

        'messages_recv_summ',
        'messages_sent_summ',
        'messages_fail_summ',
        'messages_no_locations_summ',
        'messages_out_service_ticket_validation_error_summ',
        'messages_out_no_service_ticket_error_summ',
        'user_not_subscribed_summ',

        'smart_home_request_ok_summ',
        'smart_home_request_fail_summ',
        'smart_home_request_unauthorized_summ',
        'smart_home_request_forbidden_summ',

        'notification_change_status_ok_summ',
        'notification_change_status_fail_summ',

        'notification_state_ok_summ',
        'notification_state_fail_summ',
        'notification_archive_ok_summ',
        'notification_archive_fail_summ',

        'devices_get_ok_summ',
        'devices_get_fail_summ',

        'delete_personal_cards_ok_summ',
        'delete_personal_cards_fail_summ',

        'locator_post_ok_summ',
        'locator_post_fail_summ',
        'locator_delete_ok_summ',
        'locator_delete_fail_summ',

        'notificator_subscriptions_list_cached_summ',
        'notificator_categories_list_cached_summ',

        'notificator_directive_new_summ',
        'notificator_directive_send_summ',
        'notificator_directive_on_connect_summ',
        'notificator_directive_no_location_summ',
        'notificator_directive_read_summ',
        'notificator_directive_delivered_summ',
        'notificator_directive_delete_summ',
    ]

    @classmethod
    def init(cls):
        for name in cls.__counters:
            GlobalCounter.register_counter(name)

        GlobalCounter.init()


class NotificatorTimings:

    __timings = [
        ('delivery_msg_total', TimingsResolution.HIGH_RESOLUTION_COUNTER_VALUES),
        ('smart_home_request', TimingsResolution.HIGH_RESOLUTION_COUNTER_VALUES),
        ('push_msg_total', TimingsResolution.HIGH_RESOLUTION_COUNTER_VALUES),
        ('push_sup_msg_total', TimingsResolution.HIGH_RESOLUTION_COUNTER_VALUES),
        ('push_sup_card_msg_total', TimingsResolution.HIGH_RESOLUTION_COUNTER_VALUES),
        ('push_delivery_total', TimingsResolution.HIGH_RESOLUTION_COUNTER_VALUES),
        ('push_directive_total', TimingsResolution.HIGH_RESOLUTION_COUNTER_VALUES),
        ('push_msg_resolve', TimingsResolution.HIGH_RESOLUTION_COUNTER_VALUES),
        ('notifications_change_status', TimingsResolution.HIGH_RESOLUTION_COUNTER_VALUES),
        ('notifications_state', TimingsResolution.HIGH_RESOLUTION_COUNTER_VALUES),
        ('notifications_archive', TimingsResolution.HIGH_RESOLUTION_COUNTER_VALUES),
        ('locator_post', TimingsResolution.HIGH_RESOLUTION_COUNTER_VALUES),
        ('locator_delete', TimingsResolution.HIGH_RESOLUTION_COUNTER_VALUES),
        ('directive_status', TimingsResolution.HIGH_RESOLUTION_COUNTER_VALUES),
        ('directive_change_status', TimingsResolution.HIGH_RESOLUTION_COUNTER_VALUES),
        ('device_get', TimingsResolution.HIGH_RESOLUTION_COUNTER_VALUES),
        ('directive_ack', TimingsResolution.HIGH_RESOLUTION_COUNTER_VALUES),
    ]

    @classmethod
    def init(cls):
        for name, boundaries in cls.__timings:
            GlobalTimings.register_counter(name, boundaries)
        GlobalTimings.init()
