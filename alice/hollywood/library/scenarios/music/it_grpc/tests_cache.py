import pytest
from .common import HttpRequestMockServicer

# it_grpc classes
from alice.hollywood.library.python.testing.it_grpc.mock_server import MockServicer
from alice.hollywood.library.python.testing.it_grpc.wrappers import GraphRequest, GraphResponse

# music protos
import alice.hollywood.library.scenarios.music.proto.cache_data_pb2 as cache_protos

# other protos
import alice.cachalot.api.protos.cachalot_pb2 as cachalot_protos
import apphost.lib.proto_answers.http_pb2 as http_protos


def build_http_request(path, content):
    http_request = http_protos.THttpRequest()
    http_request.Method = http_protos.THttpRequest.EMethod.Get
    http_request.Scheme = http_protos.THttpRequest.EScheme.Http
    http_request.Path = path
    http_request.Content = content
    return http_request


class Consts:
    # ITEM NAMES
    CACHE_GET_REQUEST_ITEM = 'cache_get_request'
    CACHE_GET_RESPONSE_ITEM = 'cache_get_response'
    CACHE_SET_REQUEST_ITEM = 'cache_set_request'
    CACHE_SET_RESPONSE_ITEM = 'cache_set_response'
    INPUT_CACHE_META_ITEM = 'input_cache_meta'
    OUTPUT_CACHE_META_ITEM = 'output_cache_meta'
    HTTP_REQUEST_ITEM = 'http_request'
    HTTP_RESPONSE_ITEM = 'http_response'

    MUSIC_CONTENT_HTTP_REQUEST_ITEM = 'hw_music_http_request'
    MUSIC_CONTENT_HTTP_RESPONSE_ITEM = 'hw_music_http_response'
    MUSIC_LIKES_HTTP_REQUEST_ITEM = 'hw_music_likes_tracks_http_request'
    MUSIC_LIKES_HTTP_RESPONSE_ITEM = 'hw_music_likes_tracks_http_response'
    MUSIC_DISLIKES_HTTP_REQUEST_ITEM = 'hw_music_dislikes_tracks_http_request'
    MUSIC_DISLIKES_HTTP_RESPONSE_ITEM = 'hw_music_dislikes_tracks_http_response'

    # HTTP CONSTANTS
    HTTP_REQUEST_PATH = '/users/77777777/likes/tracks'
    HTTP_REQUEST_CONTENT = b''  # (no content)
    HTTP_RESPONSE_CONTENT = b'{"data":"original response"}'
    HTTP_CACHED_RESPONSE_CONTENT = b'{"data":"cached response"}'

    # OTHER CONSTANT
    CACHE_KEY = '15448026511483288311'  # for http request above


# "MUSIC_CACHE_GET" mock
class _MockCacheGetBase(MockServicer):

    def _assert_request(self, req):
        assert req.path == '/cache_get_grpc'

        get_request_item = req.get_only_item(
            Consts.CACHE_GET_REQUEST_ITEM,
            cachalot_protos.TGetRequest
        )
        assert get_request_item.Key == Consts.CACHE_KEY
        assert get_request_item.StorageTag == 'MusicScenario'

    def _prepare_item(self):
        raise NotImplementedError()

    def _prepare_response(self, req):
        resp = GraphResponse(req)
        resp.add_item(self._prepare_item(), Consts.CACHE_GET_RESPONSE_ITEM)
        return resp

    def on_request(self, req):
        self._assert_request(req)
        return self._prepare_response(req)


class MockCacheGetHit(_MockCacheGetBase):
    def _prepare_item(self):
        item = cachalot_protos.TResponse()
        item.Status = cachalot_protos.EResponseStatus.OK
        item.GetResp.Key = Consts.CACHE_KEY
        item.GetResp.Data = Consts.HTTP_CACHED_RESPONSE_CONTENT
        return item


class MockCacheGetMiss(_MockCacheGetBase):
    def _prepare_item(self):
        item = cachalot_protos.TResponse()
        item.Status = cachalot_protos.EResponseStatus.NO_CONTENT
        return item


# "MUSIC_CACHE_SET" mock
class MockCacheSet(MockServicer):

    def _assert_request(self, req):
        assert req.path == '/cache_set_grpc'

        set_request_item = req.get_only_item(
            Consts.CACHE_SET_REQUEST_ITEM,
            cachalot_protos.TSetRequest
        )
        assert set_request_item.Key == Consts.CACHE_KEY
        assert set_request_item.Data == Consts.HTTP_RESPONSE_CONTENT  # original response
        assert set_request_item.StorageTag == 'MusicScenario'

    def _prepare_response(self, req):
        item = cachalot_protos.TResponse()
        item.Status = cachalot_protos.EResponseStatus.OK
        item.SetResp.Key = Consts.CACHE_KEY

        resp = GraphResponse(req)
        resp.add_item(item, Consts.CACHE_SET_RESPONSE_ITEM)
        return resp

    def on_request(self, req):
        self._assert_request(req)
        return self._prepare_response(req)


# mock for all http proxy nodes
class MockHttpRequestProxy(HttpRequestMockServicer):
    HTTP_REQUEST_PATH = Consts.HTTP_REQUEST_PATH
    HTTP_REQUEST_CONTENT = Consts.HTTP_REQUEST_CONTENT
    HTTP_RESPONSE_CONTENT = Consts.HTTP_RESPONSE_CONTENT


@pytest.mark.graph_name('music_cache')
@pytest.mark.mock({
    'MUSIC_CACHE_SET': MockCacheSet,
})
class _TestsCacheBase:
    '''
    На вход: http_request + input_cache_meta
    На выход: http_response + output_cache_meta
    '''

    def _prepare_request(self, use_cache):
        input_cache_meta = cache_protos.TInputCacheMeta(UseCache=use_cache)
        http_req = build_http_request(Consts.HTTP_REQUEST_PATH, Consts.HTTP_REQUEST_CONTENT)

        req = GraphRequest()
        req.add_item(input_cache_meta, Consts.INPUT_CACHE_META_ITEM)
        req.add_item(http_req, self.HTTP_REQUEST_ITEM_NAME)
        return req

    def _assert_http_response(self, resp, content):
        http_resp = resp.get_only_item(
            self.HTTP_RESPONSE_ITEM_NAME,
            http_protos.THttpResponse
        )
        assert http_resp.StatusCode == 200
        assert http_resp.Content == content

    def _get_output_cache_meta(self, resp):
        return resp.get_only_item_or_none(
            Consts.OUTPUT_CACHE_META_ITEM,
            cache_protos.TOutputCacheMeta
        )

    @pytest.mark.mock({
        'MUSIC_CACHE_GET': MockCacheGetHit,
    })
    def test_dont_use_cache(self, graph):
        '''
        Кэш выключен, запросы в него не идут
        Возвращаются НОВЫЕ данные
        В кэш ничего не записывается
        '''
        req = self._prepare_request(use_cache=False)
        resp = graph(req)
        self._assert_http_response(resp, content=Consts.HTTP_RESPONSE_CONTENT)
        assert not self._get_output_cache_meta(resp)  # no 'output_cache_meta' item returned

    @pytest.mark.mock({
        'MUSIC_CACHE_GET': MockCacheGetMiss,
    })
    def test_cache_miss(self, graph):
        '''
        Делается запрос в кэш
        В кэше нет записи => возвращаются НОВЫЕ данные
        В кэш записываются данные
        '''
        req = self._prepare_request(use_cache=True)
        resp = graph(req)
        self._assert_http_response(resp, content=Consts.HTTP_RESPONSE_CONTENT)
        assert not self._get_output_cache_meta(resp).CacheHit

    @pytest.mark.mock({
        'MUSIC_CACHE_GET': MockCacheGetHit,
    })
    def test_cache_hit(self, graph):
        '''
        Делается запрос в кэш
        В кэше есть запись => возвращаются КЭШИРОВАННЫЕ данные
        В кэш ничего не записывается
        '''
        req = self._prepare_request(use_cache=True)
        resp = graph(req)
        self._assert_http_response(resp, content=Consts.HTTP_CACHED_RESPONSE_CONTENT)
        assert self._get_output_cache_meta(resp).CacheHit


@pytest.mark.mock({
    'MUSIC_SCENARIO_LIKES_TRACKS_PROXY': MockHttpRequestProxy,
})
class TestsCacheLikes(_TestsCacheBase):
    HTTP_REQUEST_ITEM_NAME = Consts.MUSIC_LIKES_HTTP_REQUEST_ITEM
    HTTP_RESPONSE_ITEM_NAME = Consts.MUSIC_LIKES_HTTP_RESPONSE_ITEM


@pytest.mark.mock({
    'MUSIC_SCENARIO_DISLIKES_TRACKS_PROXY': MockHttpRequestProxy,
})
class TestsCacheDislikes(_TestsCacheBase):
    HTTP_REQUEST_ITEM_NAME = Consts.MUSIC_DISLIKES_HTTP_REQUEST_ITEM
    HTTP_RESPONSE_ITEM_NAME = Consts.MUSIC_DISLIKES_HTTP_RESPONSE_ITEM


@pytest.mark.mock({
    'MUSIC_SCENARIO_CONTENT_PROXY': MockHttpRequestProxy,
})
class TestsCacheContent(_TestsCacheBase):
    HTTP_REQUEST_ITEM_NAME = Consts.MUSIC_CONTENT_HTTP_REQUEST_ITEM
    HTTP_RESPONSE_ITEM_NAME = Consts.MUSIC_CONTENT_HTTP_RESPONSE_ITEM
