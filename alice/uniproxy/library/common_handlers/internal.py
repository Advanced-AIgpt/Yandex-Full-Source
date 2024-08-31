from alice.uniproxy.library.common_handlers import CommonRequestHandler


class InternalHandler(CommonRequestHandler):
    def is_external_request(self):
        if self.request.headers.get("x-balancer-host"):
            return True
        return False
