import logging


class TvmClientMock:
    def __init__(self):
        self._logger = logging.getLogger("mock.tvm_client")

    async def check_service_ticket(self, ticket):
        self._logger.info("check_service_ticket ticket={}".format(ticket))
