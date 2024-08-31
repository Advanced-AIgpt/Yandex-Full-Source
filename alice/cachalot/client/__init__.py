import base64
import grpc
import json
import logging
import uuid

from tornado.ioloop import IOLoop

from google.protobuf.json_format import MessageToDict, MessageToJson

import apphost.lib.grpc.protos.service_pb2_grpc as apphost_grpc
import apphost.lib.grpc.protos.service_pb2 as apphost_protos
import library.python.codecs as codecs

import alice.cachalot.api.protos.cachalot_pb2 as protos

from alice.uniproxy.library.async_http_client import QueuedHTTPClient, HTTPRequest


class CachalotClientException(Exception):
    pass


def with_logger(class_name):
    class WithLogger:
        def __init__(self, log):
            if log:
                self._log = log
            else:
                self._log = logging.getLogger(class_name)

    return WithLogger


class UuidBuilder:
    @classmethod
    def build(cls):
        return str(uuid.uuid4())


def handle_map_mixin(target):
    http_to_grpc_handle_names = {
        "cache_get": "cache_get_grpc",
        "cache_set": "cache_set_grpc",
        "mm_session_http": "mm_session",
        "yabio_context/v2": "yabio_context",
    }

    if target == "grpc":
        handles_map = http_to_grpc_handle_names
    else:
        handles_map = {v: k for k, v in http_to_grpc_handle_names.items()}

    class HandleMapMixin:
        def _map_handle(self, handle):
            return handles_map.get(handle, handle)

    return HandleMapMixin


class GrpcLayer(handle_map_mixin("grpc"), with_logger("CachalotClient.GrpcLayer")):
    ITEM_TYPE_MAP = {
        "activation_announcement": "activation_announcement_request",
        "activation_final": "activation_final_request",
        "cache_get": "cache_get_request",
        "cache_set": "cache_set_request",
        "yabio_context": "yabio_context_request",
    }

    RESPONSE_PROTO_TYPE_MAP = {
        "yabio_context": protos.TYabioContextResponse,
    }

    def __init__(self, endpoint, log=None):
        super().__init__(log)
        self._grpc_channel = grpc.insecure_channel(endpoint)
        self._grpc_stub = apphost_grpc.TServantStub(self._grpc_channel)

    def _decompress_item_data(self, data):
        codec_id = data[0]
        if codec_id == 0:
            return data[1:]
        if codec_id == 2:
            return codecs.loads("lz4", data[1:])
        raise RuntimeError(f"Unable to unpack data with codec_id={codec_id}")

    def _extract_protobuf(self, data, value):
        data = self._decompress_item_data(data)
        if not data.startswith(b"p_"):
            raise RuntimeError(f"There is no protobuf in data starting with {data[:10]}...")
        value.ParseFromString(data[2:])
        return value

    def _compress_item_data(self, data, codec_id=0):
        if codec_id == 0:
            return b"\0" + data
        if codec_id == 2:
            return codecs.dumps("lz4", data)
        raise RuntimeError(f"Unable to compress data with codec_id={codec_id}")

    def _pack_protobuf(self, item, codec_id=0):
        return self._compress_item_data(b"p_" + item.SerializeToString(), codec_id=codec_id)

    def _pack_json(self, item, codec_id=0):
        return self._compress_item_data(json.dumps(item).encode("utf-8"), codec_id=codec_id)

    def _create_grpc_request(self, items, codec_id=0):
        request = apphost_protos.TServiceRequest()
        for i in items:
            answer = request.Answers.add()
            answer.SourceName = i.get("source_name", "INIT")
            answer.Type = i.get("type", "some_type")
            data = i["data"]
            if isinstance(data, dict):
                answer.Data = self._pack_json(data, codec_id)
            else:
                answer.Data = self._pack_protobuf(data, codec_id)
        return request

    def _make_grpc_request(self, grpc_handle, request):
        request.Path = "/" + grpc_handle
        response = self._grpc_stub.Invoke((r for r in [request]))
        return response

    def _pack_proto_request(self, proto_req):
        if isinstance(proto_req, protos.TMegamindSessionLoadRequest):
            result = protos.TMegamindSessionRequest()
            result.LoadRequest.CopyFrom(proto_req)
            return result

        if isinstance(proto_req, protos.TMegamindSessionStoreRequest):
            result = protos.TMegamindSessionRequest()
            result.StoreRequest.CopyFrom(proto_req)
            return result

        if isinstance(proto_req, protos.TYabioContextSave):
            result = protos.TYabioContextRequest()
            result.Save.CopyFrom(proto_req)
            return result

        if isinstance(proto_req, protos.TYabioContextLoad):
            result = protos.TYabioContextRequest()
            result.Load.CopyFrom(proto_req)
            return result

        if isinstance(proto_req, protos.TYabioContextDelete):
            result = protos.TYabioContextRequest()
            result.Delete.CopyFrom(proto_req)
            return result

        return proto_req

    async def execute_complex_request(self, handle, reqs, rsp_types):
        reqs = [(item_type, self._pack_proto_request(proto)) for (item_type, proto) in reqs]
        grpc_handle = self._map_handle(handle)

        items = list()
        for (item_type, proto) in reqs:
            items.append({
                "type": item_type,
                "data": proto,
            })

        grpc_response = self._make_grpc_request(grpc_handle, self._create_grpc_request(items=items))

        rsp = dict()
        for ans in grpc_response.Answers:
            proto_type = rsp_types.get(ans.Type)
            if proto_type is not None:
                rsp[ans.Type] = self._extract_protobuf(ans.Data, proto_type())

        return rsp

    async def execute_request(self, handle, proto_req):
        item_type = self.ITEM_TYPE_MAP.get(handle, "request")
        proto_req = self._pack_proto_request(proto_req)
        grpc_handle = self._map_handle(handle)
        grpc_response = self._make_grpc_request(grpc_handle, self._create_grpc_request(
            items=[{
                "type": item_type,
                "data": proto_req,
            }]
        ))

        data = grpc_response.Answers[0].Data  # only one answer of name "response" and type "TResponse"
        response_proto_type = self.RESPONSE_PROTO_TYPE_MAP.get(handle, protos.TResponse)
        response = response_proto_type()
        return True, self._extract_protobuf(data, response)


class HttpLayer(handle_map_mixin("http"), with_logger("CachalotClient.HttpLayer")):
    def __init__(self, host, port, log=None, prefix=None, req_id_builder=None):
        super().__init__(log)
        self._http_client = QueuedHTTPClient.get_client(host, port)
        self._prefix = prefix
        self._req_id_builder = req_id_builder or UuidBuilder()

    # returns (ok, body)
    async def execute_request_raw(self, http_handle, proto_req):
        self._log.debug("Executing request. Handle: %s, req: %s", http_handle, MessageToJson(proto_req))

        headers = {
            "Content-Type": "application/protobuf",
        }

        if isinstance(proto_req, protos.TRequest):
            # set special http header for cache_get/cache_set requests.
            if proto_req.HasField("GetReq"):
                headers["X-Cachalot-Key"] = proto_req.GetReq.Key
            if proto_req.HasField("SetReq"):
                headers["X-Cachalot-Key"] = proto_req.SetReq.Key

            if proto_req.HasField("MegamindSessionReq") and proto_req.MegamindSessionReq.HasField("LoadRequest"):
                headers["X-Cachalot-Key"] = proto_req.MegamindSessionReq.LoadRequest.Uuid
            if proto_req.HasField("MegamindSessionReq") and proto_req.MegamindSessionReq.HasField("StoreRequest"):
                headers["X-Cachalot-Key"] = proto_req.MegamindSessionReq.StoreRequest.Uuid

            headers["X-Ya-ReqId"] = self._req_id_builder.build()

        request = HTTPRequest(
            f"/{http_handle}" if self._prefix is None else f"/{self._prefix}/{http_handle}",
            method="POST",
            headers=headers,
            body=proto_req.SerializeToString(),
        )

        response = await self._http_client.fetch(request, raise_error=False)
        ok_code = response.code // 100 != 5

        if not ok_code:
            self._log.error(
                "Got response. Code: %s, headers: %s, body: %s",
                response.code,
                response.headers,
                response.body
            )

        return ok_code, response.body

    def _pack_proto_request(self, proto_req) -> protos.TRequest:
        result = protos.TRequest()

        if isinstance(proto_req, protos.TRequest):
            result.CopyFrom(proto_req)

        elif isinstance(proto_req, protos.TGetRequest):
            result.GetReq.CopyFrom(proto_req)

        elif isinstance(proto_req, protos.TSetRequest):
            result.SetReq.CopyFrom(proto_req)

        elif isinstance(proto_req, protos.TMegamindSessionRequest):
            result.MegamindSessionReq.CopyFrom(proto_req)

        elif isinstance(proto_req, protos.TMegamindSessionLoadRequest):
            result.MegamindSessionReq.LoadRequest.CopyFrom(proto_req)

        elif isinstance(proto_req, protos.TMegamindSessionStoreRequest):
            result.MegamindSessionReq.StoreRequest.CopyFrom(proto_req)

        elif isinstance(proto_req, protos.TActivationAnnouncementRequest):
            result.ActivationAnnouncement.CopyFrom(proto_req)

        elif isinstance(proto_req, protos.TActivationFinalRequest):
            result.ActivationFinal.CopyFrom(proto_req)

        elif isinstance(proto_req, protos.TGDPRSetUserDataRequest):
            result.GDPRSetReq.CopyFrom(proto_req)

        elif isinstance(proto_req, protos.TGDPRGetUserDataRequest):
            result.GDPRGetReq.CopyFrom(proto_req)

        elif isinstance(proto_req, protos.TGDPRGetRequestsRequest):
            result.GDPRGetRequestsReq.CopyFrom(proto_req)

        elif isinstance(proto_req, protos.TTakeoutGetResultsRequest):
            result.TakeoutGetResultsReq.CopyFrom(proto_req)

        elif isinstance(proto_req, protos.TTakeoutSetResultsRequest):
            result.TakeoutSetResultsReq.CopyFrom(proto_req)

        elif isinstance(proto_req, protos.TYabioContextSave):
            result.YabioContextReq.Save.CopyFrom(proto_req)

        elif isinstance(proto_req, protos.TYabioContextLoad):
            result.YabioContextReq.Load.CopyFrom(proto_req)

        elif isinstance(proto_req, protos.TYabioContextDelete):
            result.YabioContextReq.Delete.CopyFrom(proto_req)

        else:
            raise CachalotClientException("Unknown proto request format")

        return result

    # returns (True, proto_ans) or (False, error_msg)
    async def execute_request(self, handle, proto_req):
        http_handle = self._map_handle(handle)
        proto_req = self._pack_proto_request(proto_req)
        ok, body = await self.execute_request_raw(http_handle, proto_req)

        if not ok:
            return False, body

        response_proto = protos.TResponse()
        response_proto.ParseFromString(body)
        return True, response_proto


class CachalotClient:
    @classmethod
    def _make_grpc_connection(cls, host, port=None):
        return GrpcLayer(f"{host}:{port}")

    @classmethod
    def _make_http_connection(cls, host, port, http_layer_kwargs={}):
        return HttpLayer(host, port, **http_layer_kwargs)

    @classmethod
    def _convert_proto_response(cls, proto_response):
        return MessageToDict(
            proto_response,
            including_default_value_fields=True,
            preserving_proto_field_name=True,
        )

    @classmethod
    def _convert_bytes_field(cls, string):
        # google.protobuf.json_format.MessageToDict makes base64 instead of bytes :(
        return base64.b64decode(string)

    @classmethod
    def _convert_bytes_field_by_path(cls, response, path):
        key = path[0]
        if len(path) == 1:
            field = response.get(key)
            if field is not None:
                response[key] = cls._convert_bytes_field(field)
        else:
            cls._convert_bytes_field_by_path(response.get(key, {}), path[1:])

    @classmethod
    def _set_cache_request_options(cls, proto_req, force_imdb=None, force_redis=None, force_ydb=None):
        if force_imdb is not None:
            proto_req.Options.ForceInternalStorage = force_imdb

        if force_redis is not None:
            proto_req.Options.ForceRedisStorage = force_redis

        if force_ydb is not None:
            proto_req.Options.ForceYdbStorage = force_ydb

    def __init__(self, host, http_port=None, grpc_port=None, raise_on_error=False, http_layer_kwargs={}):
        self.raise_on_error = raise_on_error

        if grpc_port:
            self._connection = self._make_grpc_connection(host, grpc_port)
        elif http_port:
            self._connection = self._make_http_connection(host, http_port, http_layer_kwargs)
        else:
            raise CachalotClientException("Can not connect to Cachalot: both ports are None")

    async def _execute_request(self, handle, proto_req, raise_on_error, raw_response=False):
        if raise_on_error is None:
            raise_on_error = self.raise_on_error

        ok, rsp = await self._connection.execute_request(handle, proto_req)

        if ok:
            if raw_response:
                return rsp
            return self._convert_proto_response(rsp)
        elif raise_on_error:
            raise CachalotClientException(rsp)
        else:
            return None

    async def request_stats(self):
        assert isinstance(self._connection, HttpLayer)
        ok, body = await self._connection.execute_request_raw("unistat", protos.TRequest())
        if ok:
            return json.loads(body)
        else:
            return None

    async def cache_get(self, key, storage_tag=None, force_imdb=None, force_redis=None, force_ydb=None, raise_on_error=True):
        get_request = protos.TGetRequest()
        get_request.Key = key

        if storage_tag is not None:
            get_request.StorageTag = storage_tag

        self._set_cache_request_options(
            get_request,
            force_imdb=force_imdb,
            force_redis=force_redis,
            force_ydb=force_ydb
        )

        response = await self._execute_request("cache_get", get_request, raise_on_error=raise_on_error)
        if response is not None and response["Status"] == "OK":
            self._convert_bytes_field_by_path(response, ["GetResp", "Data"])
        return response

    async def cache_set(self, key, value, ttl=None, storage_tag=None, force_imdb=None,
                        force_redis=None, force_ydb=None, raise_on_error=True):
        set_request = protos.TSetRequest()
        set_request.Key = key
        set_request.Data = value

        if ttl is not None:
            set_request.TTL = ttl

        if storage_tag is not None:
            set_request.StorageTag = storage_tag

        self._set_cache_request_options(
            set_request,
            force_imdb=force_imdb,
            force_redis=force_redis,
            force_ydb=force_ydb
        )
        return await self._execute_request("cache_set", set_request, raise_on_error=raise_on_error)

    async def mm_session_get(self, uuid, dialog_id=None, request_id=None, raise_on_error=True):
        load_req = protos.TMegamindSessionLoadRequest()
        load_req.Uuid = uuid
        if dialog_id is not None:
            load_req.DialogId = dialog_id
        if request_id is not None:
            load_req.RequestId = request_id

        response = await self._execute_request("mm_session", load_req, raise_on_error=raise_on_error)
        self._convert_bytes_field_by_path(response, ["MegamindSessionLoadResp", "Data"])
        return response

    async def mm_session_set(self, uuid, data, dialog_id=None, request_id=None,
                             puid=None, raise_on_error=True):
        store_req = protos.TMegamindSessionStoreRequest()
        store_req.Uuid = uuid
        store_req.Data = data
        if dialog_id is not None:
            store_req.DialogId = dialog_id
        if request_id is not None:
            store_req.RequestId = request_id

        if puid is not None:
            store_req.Puid = puid

        return await self._execute_request("mm_session", store_req, raise_on_error=raise_on_error)

    @classmethod
    def _decode_activation_response(cls, response):
        cls._convert_bytes_field_by_path(response, ["ActivationAnnouncement", "Error"])
        cls._convert_bytes_field_by_path(response, ["ActivationAnnouncement", "BestCompetitor", "UserId"])
        cls._convert_bytes_field_by_path(response, ["ActivationAnnouncement", "BestCompetitor", "DeviceId"])
        cls._convert_bytes_field_by_path(response, ["ActivationFinal", "Error"])
        cls._convert_bytes_field_by_path(response, ["ActivationFinal", "LeaderInfo", "UserId"])
        cls._convert_bytes_field_by_path(response, ["ActivationFinal", "LeaderInfo", "DeviceId"])
        cls._convert_bytes_field_by_path(response, ["ActivationFinal", "SpotterValidatedBy"])
        return response

    async def activation_announcement(self, user_id, device_id, avg_rms,
                                      timestamp, spotter_validated, freshness_delta_milliseconds=None,
                                      raise_on_error=True):
        assert isinstance(self._connection, HttpLayer)  # grpc handlers have different api.

        activation_request = protos.TActivationAnnouncementRequest()
        activation_request.Info.UserId = user_id
        activation_request.Info.DeviceId = device_id
        activation_request.Info.ActivationAttemptTime.CopyFrom(timestamp)
        activation_request.Info.SpotterFeatures.AvgRMS = avg_rms
        activation_request.Info.SpotterFeatures.Validated = spotter_validated

        if freshness_delta_milliseconds is not None:
            activation_request.FreshnessDeltaMilliSeconds = freshness_delta_milliseconds

        return self._decode_activation_response(
            await self._execute_request("activation_announcement", activation_request, raise_on_error=raise_on_error))

    async def activation_final(self, user_id, device_id, avg_rms, timestamp, ignore_rms=False,
                               freshness_delta_milliseconds=None,
                               raise_on_error=True):
        assert isinstance(self._connection, HttpLayer)  # grpc handlers have different api.

        activation_request = protos.TActivationFinalRequest()
        activation_request.Info.UserId = user_id
        activation_request.Info.DeviceId = device_id
        activation_request.Info.ActivationAttemptTime.CopyFrom(timestamp)
        activation_request.Info.SpotterFeatures.AvgRMS = avg_rms
        activation_request.Info.SpotterFeatures.Validated = True
        activation_request.IgnoreRms = ignore_rms

        if freshness_delta_milliseconds is not None:
            activation_request.FreshnessDeltaMilliSeconds = freshness_delta_milliseconds

        return self._decode_activation_response(
            await self._execute_request("activation_final", activation_request, raise_on_error=raise_on_error))

    async def gdpr_set_user_data(self, puid, service_info, raise_on_error=False):
        gdpr_request = protos.TGDPRSetUserDataRequest()
        gdpr_request.Key.Puid = puid
        status = gdpr_request.Status
        status.Service = service_info['service']
        status.Status = service_info['status']
        status.Timestamp = service_info.get('ts', '')

        return await self._execute_request("gdpr", gdpr_request, raise_on_error=raise_on_error)

    async def gdpr_get_user_data(self, puid, raise_on_error=False):
        gdpr_request = protos.TGDPRGetUserDataRequest()
        gdpr_request.Key.Puid = puid

        return await self._execute_request("gdpr", gdpr_request, raise_on_error=raise_on_error)

    async def gdpr_get_requests(self, limit=100, offset=0, raise_on_error=False):
        gdpr_request = protos.TGDPRGetRequestsRequest()
        gdpr_request.Limit = limit
        gdpr_request.Offset = offset

        return await self._execute_request("gdpr", gdpr_request, raise_on_error=raise_on_error)

    async def takeout_set_results(self, results, raise_on_error=False):
        request = protos.TTakeoutSetResultsRequest()
        for result in results:
            res = request.Results.add()
            res.JobId = result['job_id']
            res.Puid = result['puid']
            res.Texts.extend(result['texts'])

        return await self._execute_request("takeout", request, raise_on_error=raise_on_error)

    async def takeout_get_results(self, job_id, limit=100, offset=0, raise_on_error=False):
        request = protos.TTakeoutGetResultsRequest()
        request.JobId = job_id
        request.Limit = limit
        request.Offset = offset

        return await self._execute_request("takeout", request, raise_on_error=raise_on_error)

    async def yabio_context_save(self, group_id, device_model, device_manufacturer, context, raise_on_error=False):
        request = protos.TYabioContextSave()
        request.Key.GroupId = group_id
        request.Key.DevModel = device_model
        request.Key.DevManuf = device_manufacturer
        request.Context = context
        return await self._execute_request("yabio_context", request, raise_on_error=raise_on_error)

    async def yabio_context_load(self, group_id, device_model, device_manufacturer, raise_on_error=False):
        request = protos.TYabioContextLoad()
        request.Key.GroupId = group_id
        request.Key.DevModel = device_model
        request.Key.DevManuf = device_manufacturer
        return await self._execute_request("yabio_context", request, raise_on_error=raise_on_error)

    async def yabio_context_delete(self, group_id, device_model, device_manufacturer, raise_on_error=False):
        request = protos.TYabioContextDelete()
        request.Key.GroupId = group_id
        request.Key.DevModel = device_model
        request.Key.DevManuf = device_manufacturer
        return await self._execute_request("yabio_context", request, raise_on_error=raise_on_error)


class SyncMixin:
    @classmethod
    def _wait(self, func, *args, **kwargs):
        async def wrapper():
            return await func(*args, **kwargs)
        return IOLoop.current().run_sync(wrapper)

    @classmethod
    def _sync_wrapper(self, func):
        def wrapper(*args, **kwargs):
            return self._wait(func, *args, **kwargs)
        return wrapper


class SyncCachalotClient(SyncMixin):
    def __init__(self, *args, **kwargs):
        self._client = CachalotClient(*args, **kwargs)

        for method in (
            "request_stats",
            "cache_get",
            "cache_set",
            "mm_session_get",
            "mm_session_set",
            "activation_announcement",
            "activation_final",
            "takeout_get_results",
            "takeout_set_results",
            "yabio_context_save",
            "yabio_context_load",
            "yabio_context_delete",
        ):
            setattr(self, method, self._sync_wrapper(getattr(self._client, method)))
