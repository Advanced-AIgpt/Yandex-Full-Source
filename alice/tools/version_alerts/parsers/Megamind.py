import logging

log = logging.getLogger('Megamind')


class Megamind:
    def __init__(self):
        pass

    @staticmethod
    async def parse(host_answer: str) -> int:
        log.debug(f'Parsing {host_answer} from Megamind')
        if type(host_answer) is not str:
            return host_answer
        for line in host_answer.split('\n'):
            if 'Rev' in line:
                log.debug(f'Found {line}')
                version = line.split()[-1]
                try:
                    return int(version)
                except ValueError:
                    log.error(f'Bad Megamind version {version} in line {line}')
                    return -1
        return host_answer
