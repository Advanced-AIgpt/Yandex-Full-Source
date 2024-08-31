from alice.uniproxy.library.global_counter import GlobalCounter, GlobalTimings, TimingsResolution


class DeliveryCounter:

    _delivery_counters = [
        'push_messages_recv_summ',
        'push_messages_sent_summ',
        'push_messages_fail_summ',
        'push_messages_no_locations_summ',
    ]

    @classmethod
    def init(cls):
        for name in cls._delivery_counters:
            GlobalCounter.register_counter(name)

        GlobalCounter.init()


class DeliveryTimings:

    __delivery = [
        ('push_msg_total', TimingsResolution.HIGH_RESOLUTION_COUNTER_VALUES),
        ('push_msg_resolve', TimingsResolution.HIGH_RESOLUTION_COUNTER_VALUES),
    ]

    @classmethod
    def init(cls):
        GlobalTimings.init()
