import logging

log = logging.getLogger('Mock')


class Mock:
    def __init__(self):
        pass

    @staticmethod
    async def parse(host_answer: str) -> int:
        return 1