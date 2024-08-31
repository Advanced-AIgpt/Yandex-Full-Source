class AppsTimespent(object):
    def __init__(self):
        self.counter = {}
        self.prev_value = {}

    def add_event(self, rec):
        if rec['event_name'] == 'apps_time_spent_info':
            if 'apps' in rec['event_value']:
                apps_times = rec['event_value']['apps']
            else:
                apps_times = rec['event_value']

            for app, timespent in apps_times.items():
                if not isinstance(timespent, dict):
                    value = int(timespent)
                    if self.prev_value.get(app) != value:
                        self.prev_value[app] = value
                        self.counter[app] = self.counter.get(app, 0) + value

    def get_info(self):
        return self.counter
