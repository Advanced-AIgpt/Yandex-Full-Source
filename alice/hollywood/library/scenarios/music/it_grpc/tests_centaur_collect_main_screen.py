import pytest
from alice.hollywood.library.python.testing.it2.scenario_responses import proto_to_json
from .common import HttpRequestMockServicer, load_from_data

# it_grpc classes
from alice.hollywood.library.python.testing.it_grpc.wrappers import GraphRequest

# music protos
import alice.hollywood.library.scenarios.music.proto.centaur_pb2 as centaur_protos


class Consts:
    # ITEM NAMES
    COLLECT_MAIN_SCREEN_REQUEST_ITEM = 'collect_main_screen_request'
    COLLECT_MAIN_SCREEN_RESPONSE_ITEM = 'collect_main_screen_response'

    # HTTP CONSTANTS
    INFINITE_FEED_HTTP_REQUEST_PATH = '/infinite-feed?landingType=navigator&supportedBlocks=generic&__uid=007'
    INFINITE_FEED_HTTP_REQUEST_CONTENT = b''  # (no content)
    INFINITE_FEED_HTTP_RESPONSE_CONTENT = load_from_data('infinite_feed_response.json')

    CHILDREN_LANDING_CATALOGUE_HTTP_REQUEST_PATH = '/children-landing/catalogue?requestedBlocks=CATEGORIES_TAB&__uid=007'
    CHILDREN_LANDING_CATALOGUE_HTTP_REQUEST_CONTENT = b''  # (no content)
    CHILDREN_LANDING_CATALOGUE_HTTP_RESPONSE_CONTENT = load_from_data('children_landing_catalogue.json')


# 'MUSIC_INFINITE_FEED_PROXY'
class MockMusicInfiniteFeedProxy(HttpRequestMockServicer):
    HTTP_REQUEST_PATH = Consts.INFINITE_FEED_HTTP_REQUEST_PATH
    HTTP_REQUEST_CONTENT = Consts.INFINITE_FEED_HTTP_REQUEST_CONTENT
    HTTP_RESPONSE_CONTENT = Consts.INFINITE_FEED_HTTP_RESPONSE_CONTENT


# 'MUSIC_CHILDREN_LANDING_CATALOGUE_PROXY'
class MockMusicChildrenCatalogueProxy(HttpRequestMockServicer):
    HTTP_REQUEST_PATH = Consts.CHILDREN_LANDING_CATALOGUE_HTTP_REQUEST_PATH
    HTTP_REQUEST_CONTENT = Consts.CHILDREN_LANDING_CATALOGUE_HTTP_REQUEST_CONTENT
    HTTP_RESPONSE_CONTENT = Consts.CHILDREN_LANDING_CATALOGUE_HTTP_RESPONSE_CONTENT


@pytest.mark.graph_name('music_centaur_collect_main_screen')
@pytest.mark.mock({
    'MUSIC_INFINITE_FEED_PROXY': MockMusicInfiniteFeedProxy,
    'MUSIC_CHILDREN_LANDING_CATALOGUE_PROXY': MockMusicChildrenCatalogueProxy,
})
class TestsCentaurCollectMainScreen:

    def test_simple(self, graph):
        item = centaur_protos.TCollectMainScreenRequest()
        item.MusicRequestMeta.RequestMeta.OAuthToken = 'TestOAuthToken'
        item.MusicRequestMeta.UserId = '007'

        req = GraphRequest()
        req.add_item(item, Consts.COLLECT_MAIN_SCREEN_REQUEST_ITEM)

        resp = graph(req)
        collect_response = resp.get_only_item(
            Consts.COLLECT_MAIN_SCREEN_RESPONSE_ITEM,
            centaur_protos.TCollectMainScreenResponse
        )

        return proto_to_json(collect_response)
