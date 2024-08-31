from tornado.gen import Future
from alice.uniproxy.library.backends_common.apikey import check_key
from alice.uniproxy.library.auth.tvm2 import check_service_ticket


class AuthError(Exception):
    def __init__(self, message):
        super().__init__(f"AuthError: {message}")


class HandlerWithYandexHeaders:
    def get_client_ip(self):
        return self.request.headers.get("X-Real-Ip") \
            or self.request.headers.get("X-Forwarded-For") \
            or self.request.remote_ip

    def get_client_uuid(self):
        return self.request.headers.get("X-UPRX-UUID")

    def get_client_auth_token(self):  # aka apikey
        return self.request.headers.get("X-UPRX-AUTH-TOKEN")

    def get_client_oauth_token(self):
        return self.request.headers.get("Authorization")

    def get_client_service_ticket(self):
        return self.request.headers.get("X-Ya-Service-Ticket")

    def get_client_user_ticket(self):
        return self.request.headers.get("X-Ya-User-Ticket")

    async def check_auth_token(self, rt_log=None):
        auth_token = self.get_client_auth_token()
        if auth_token is None:
            return False

        result = Future()
        check_key(
            auth_token,
            self.get_client_ip(),
            on_ok=lambda _: result.set_result(True),
            on_fail=lambda: result.set_result(False),
            rt_log=rt_log)
        return await result

    async def check_service_ticket(self, rt_log=None):
        ticket = self.get_client_service_ticket()
        if ticket is None:
            return False

        try:
            await check_service_ticket(ticket)
            return True
        except:
            return False
