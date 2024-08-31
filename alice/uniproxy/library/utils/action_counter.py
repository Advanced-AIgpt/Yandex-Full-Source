from alice.uniproxy.library.global_counter import GlobalTimings

from time import monotonic


class ActionCounter:
    def __init__(self, signal_name):
        self.signal_name = signal_name
        self.last_action_time = None

    def record_action(self):
        current_time = monotonic()

        if self.last_action_time is not None:
            seconds = current_time - self.last_action_time
            GlobalTimings.store(self.signal_name, seconds)

        self.last_action_time = current_time
