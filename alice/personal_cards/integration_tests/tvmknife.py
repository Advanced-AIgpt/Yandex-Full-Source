import subprocess


class TVMKnife:
    def __init__(self):
        import yatest.common
        self._path = yatest.common.binary_path('passport/infra/tools/tvmknife/bin/tvmknife')

    def service_ticket(self):
        return subprocess.check_output([self._path, "unittest", "service", "-s", "2005865", "-d", "2026486"]).decode().strip()

    def user_ticket(self, uid):
        return subprocess.check_output([self._path, "unittest", "user", "-d", str(uid)]).decode().strip()
