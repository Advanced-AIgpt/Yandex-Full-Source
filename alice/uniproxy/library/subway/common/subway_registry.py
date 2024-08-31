import collections
import logging

from alice.uniproxy.library.subway.common.registry_base import RegistryBase


class SubwayRegistry(RegistryBase):
    def __init__(self):
        super(SubwayRegistry, self).__init__()
        self._log = logging.getLogger('subway.registry')
        self._registry = collections.defaultdict(self._int_dict)

    def _int_dict(self):
        return collections.defaultdict(int)

    def add(self, process, guid: bytes):
        self._registry[guid][process] += 1
        self._log.debug('add: {}'.format(guid))

        if self._registry[guid][process] == 0:
            self._log.error('ADD GUID(%s) COUNT == 0')

    def remove(self, process, guid: bytes):
        if guid not in self._registry:
            return

        if process not in self._registry[guid]:
            return

        _count = self._registry[guid][process]
        self._registry[guid][process] -= 1

        if _count == 1:
            del self._registry[guid][process]
            if not self._registry[guid]:
                del self._registry[guid]
        elif _count <= 0:
            self._log.error('REMOVE GUID(%s) COUNT < 0')

    def enumerate(self, guid: bytes):
        if guid not in self._registry:
            yield False, guid, None
            return

        for process_id, count in self._registry[guid].items():
            if count == 0:
                yield False, guid, process_id
            else:
                yield True, guid, process_id

    def enumerate_many(self, guids):
        for guid in guids:
            if guid not in self._registry:
                yield False, guid, None
            else:
                for process_id, count in self._registry[guid].items():
                    if count == 0:
                        yield False, guid, process_id
                    else:
                        yield True, guid, process_id

    def enumerate_clients(self, as_str=True):
        result = collections.defaultdict(int)
        for guid, processes in self._registry.items():
            self._log.debug('GUID BYTES => %s', guid)
            if as_str:
                guid = self.uuid_as_str(guid)
            self._log.debug('GUID STR => %s', guid)
            for proc in processes:
                count = processes[proc]
                result[guid] += count
        self._log.debug('RSLT => %s', result)
        return result

    def count(self, guid: bytes):
        if guid not in self._registry:
            return 0

        sessions_count = 0
        for process_id, c in self._registry[guid].items():
            sessions_count += c
        return sessions_count
