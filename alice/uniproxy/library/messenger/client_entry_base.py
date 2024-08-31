from alice.uniproxy.library.utils.hostname import current_hostname
from alice.uniproxy.library.settings import config

CLIENT_BASE_ENVIRONMENT = config['messenger']['locator']['environment']
CLIENT_BASE_COMPONENT = config['messenger']['locator']['component']


class ClientEntryBase:

    # ----------------------------------------------------------------------------------------------------------------
    def encoded_hostname(self, hostname: str = None, default=None):
        if hostname is None:
            hostname = current_hostname()

        try:
            instance, component, environment, application, _ = hostname.split('.', 4)
            if application != 'uniproxy':
                return None, None, None, hostname

            _, host_no_string = instance.rsplit('-', 1)

            host_no = int(host_no_string)

            component = component if component != CLIENT_BASE_COMPONENT else default
            environment = environment if environment != CLIENT_BASE_ENVIRONMENT else default

            return host_no, component, environment, None
        except Exception:
            return None, None, None, hostname

    # ----------------------------------------------------------------------------------------------------------------
    def enumerate_locations(self, **kwargs):
        raise NotImplementedError
