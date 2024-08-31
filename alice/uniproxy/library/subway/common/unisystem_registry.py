import collections
import logging

from alice.uniproxy.library.subway.common.registry_base import RegistryBase


class UnisystemRegistry(RegistryBase):
    def __init__(self):
        super(UnisystemRegistry, self).__init__()
        self._log = logging.getLogger('uniproxy.registry')
        self._registry = collections.defaultdict(dict)

    def canonical_bytes(self, guid):
        return self.uuid_as_bytes(guid)

    def add(self, unisystem, guid, session_id: str):
        guid_bytes = self.canonical_bytes(guid)
        session_id_bytes = self.uuid_as_bytes(session_id)
        self._registry[guid_bytes][session_id_bytes] = unisystem

    def remove(self, guid, session_id):
        guid_bytes = self.canonical_bytes(guid)
        session_id_bytes = self.uuid_as_bytes(session_id)

        if guid_bytes not in self._registry:
            return

        if session_id_bytes not in self._registry[guid_bytes]:
            return

        del self._registry[guid_bytes][session_id_bytes]

        if not self._registry[guid_bytes]:
            del self._registry[guid_bytes]

    def enumerate(self, guid):
        guid = self.uuid_as_bytes(guid)

        if guid not in self._registry:
            return

        for session_id, unisystem in self._registry[guid].items():
            if isinstance(guid, str):
                yield guid, self.uuid_as_str(session_id), unisystem
            else:
                yield self.uuid_as_str(guid), self.uuid_as_str(session_id), unisystem

    def enumerate_many(self, guids):
        for guid in guids:
            if guid not in self._registry:
                continue

            for session_id, unisystem in self._registry[guid].items():
                yield guid, session_id, unisystem

    def count(self, guid: str):
        guid_bytes = self.uuid_as_bytes(guid)
        if guid_bytes in self._registry:
            return len(self._registry[guid_bytes])
        return 0
