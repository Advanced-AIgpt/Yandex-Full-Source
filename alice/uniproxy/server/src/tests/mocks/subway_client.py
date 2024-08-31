import logging


class SubwayClientMock:
    def __init__(self):
        self._logger = logging.getLogger("mock.subway_client")

    async def add_client(self, unisystem, **kwargs):
        guid = kwargs.get("guid", unisystem.guid)
        self._logger.info("add_client session_id={} guid={}".format(unisystem.session_id, guid))

    async def remove_client(self, unisystem):
        self._logger.info("remove_client session_id={}".format(unisystem.session_id))

