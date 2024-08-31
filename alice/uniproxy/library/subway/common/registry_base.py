import uuid as uuid_


class RegistryBase(object):
    def __init__(self):
        super(RegistryBase, self).__init__()

    def uuid_as_bytes(self, uuid):
        try:
            return uuid_.UUID(uuid).bytes
        except (ValueError, TypeError):
            return uuid

    def uuid_as_str(self, uuid: bytes):
        return str(uuid_.UUID(bytes=uuid))
