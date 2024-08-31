import base64
import rtlog

from cityhash import hash64 as CityHash64

from alice.uniproxy.library.global_counter import GlobalCounter, UnistatTiming
from alice.uniproxy.library.settings import config

from alice.cachalot.client import CachalotClient


# Session is loaded by |prev_req_id|, but saved both by |request_id|
# and empty request id. We did this because some clients, like, YaBro
# or Watches, do not send |prev_req_id|, or send it
# sporadically. Therefore, to prevent session loss for such clients,
# session is saved by empty request id to be loaded in the next
# request in case of empty |prev_req_id|, and saved by correct
# |request_id| to be loaded in the next request in case of correct
# |prev_req_id|.

CACHALOT_HOST = config.get('cachalot-mm', {}).get('host', 'cachalot.alice.yandex.net')
CACHALOT_PORT = config.get('cachalot-mm', {}).get('http_port', 80)
CACHALOT_PREFIX = config.get('cachalot-mm', {}).get('prefix', None)


def get_by_path(dct, *keys):
    for k in keys:
        dct = dct.get(k, None)
        if dct is None:
            return None
    return dct


def make_http_cachalot_client():
    return CachalotClient(
        CACHALOT_HOST,
        http_port=CACHALOT_PORT,
        http_layer_kwargs=dict(
            prefix=CACHALOT_PREFIX,
        )
    )


class VinsContextAccessor:
    def __init__(self, unisystem_session_id, context, rt_log, puid):
        self._uuid = get_by_path(context, 'application', 'uuid')
        if not self._uuid:
            self._uuid = get_by_path(context, 'vins', 'application', 'uuid')
        self._dialog_id = get_by_path(context, 'header', 'dialog_id') or ''
        self._request_id = get_by_path(context, 'header', 'request_id') or ''
        self._prev_request_id = get_by_path(context, 'header', 'prev_req_id') or ''
        self._puid = puid or ''
        self._rt_log = rt_log
        self._cachalot = make_http_cachalot_client()

    async def load(self):
        """ returns None if not found
        """
        try:
            with UnistatTiming('cachalot_mm_load_time'):
                ret = await self._cachalot.mm_session_get(
                    self._uuid,
                    dialog_id=self._dialog_id,
                    request_id=self._prev_request_id,
                    raise_on_error=True,
                )
                data = ret.get('MegamindSessionLoadResp', {}).get('Data')

                if data:
                    GlobalCounter.CACHALOT_MM_LOAD_OK_SUMM.increment()
                else:
                    GlobalCounter.CACHALOT_MM_LOAD_NOT_FOUND_SUMM.increment()

                return data
        except Exception as exc:
            self._rt_log.exception(f'Got exception from cachalot/mm_session_get: {exc}')
            GlobalCounter.CACHALOT_MM_LOAD_ERR_SUMM.increment()

    async def load_base64_ex(self):
        result = await self.load()
        if result:
            return CityHash64(result), base64.b64encode(result).decode('ascii')
        else:
            return 0, None

    async def save(self, items):
        for k, v in items.items():
            try:
                with UnistatTiming('cachalot_mm_save_time'):
                    await self._cachalot.mm_session_set(
                        self._uuid,
                        v,
                        dialog_id=k,
                        request_id=self._request_id,
                        puid=self._puid,
                    )
                GlobalCounter.CACHALOT_MM_SAVE_OK_SUMM.increment()
            except Exception as exc:
                self._rt_log.exception(f'Got exception from cachalot/mm_session_save: {exc}')
                GlobalCounter.CACHALOT_MM_SAVE_ERR_SUMM.increment()

    async def save_base64(self, items):
        items_bytes = {}
        for k, v in items.items():
            items_bytes[k] = base64.b64decode(v)
        await self.save(items_bytes)

    async def save_base64_ex(self, items):
        items_bytes = {}
        items_infos = {}
        for k, v in items.items():
            data = base64.b64decode(v)
            items_bytes[k] = data
            items_infos[k] = {
                'size': len(data) if data else 0,
                'hash': CityHash64(data) if data else 0,
            }
        await self.save(items_bytes)
        return items_infos

    def _create_key(self, dialog_id, request_id):
        key = self._uuid
        if dialog_id:
            key += "$" + dialog_id
        if request_id:
            key += "@" + request_id
        return key.encode('ascii')

    def _get_load_key(self):
        return self._create_key(self._dialog_id, self._prev_request_id)

    def _get_save_key(self):
        return self._create_key(self._dialog_id, self._request_id)


class VinsContextStorage:
    def __init__(self):
        pass

    def create_accessor(self, unisystem_session_id, puid, context, rt_log=rtlog.null_logger()):
        return VinsContextAccessor(unisystem_session_id, context, rt_log, puid)


_instance = None


async def get_instance():
    global _instance
    if not _instance:
        _instance = VinsContextStorage()
    return _instance


async def get_accessor(unisystem_session_id, puid, context, rt_log=rtlog.null_logger()):
    storage = await get_instance()
    return storage.create_accessor(unisystem_session_id, puid, context, rt_log)
