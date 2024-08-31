import pytest
from alice.hollywood.library.python.testing.it2.stubber import create_stubber_fixture, StubberEndpoint


def create_nlgsearch_stubber_fixture():
    return create_stubber_fixture(
        'general-conversation.yandex.net',
        80,
        [
            StubberEndpoint('/yandsearch', ['GET']),
        ],
        stubs_subdir='nlgsearch',
    )


def create_entity_search_stubber_fixture():
    return create_stubber_fixture(
        'entitysearch.yandex.net',
        80,
        [
            StubberEndpoint('/get', ['GET']),
        ],
        stubs_subdir='entity_search',
    )


def create_seq2seq_stubber_fixture():
    return create_stubber_fixture(
        'general-conversation-gpu.yandex.net',
        80,
        [
            StubberEndpoint('/generative', ['POST']),
            StubberEndpoint('/generative_toast', ['POST']),
            StubberEndpoint('/generative_bart', ['POST']),
            StubberEndpoint('/bert_factor', ['POST']),
            StubberEndpoint('/tales/generative', ['POST']),
        ],
        stubs_subdir='seq2seq',
    )


def create_create_or_commit_shared_link_stubber_fixture():
    return create_stubber_fixture(
        # to recanonize the tests, that depend on ALICE_SOCIAL_SHARING, obtain
        # the actual host from https://horizon.z.yandex-team.ru/backends/info/ALICE_SOCIAL_SHARE/8350088/trunk or something
        'n427hmck37lbdjxq.sas.yp-c.yandex.net',
        10000,
        [
            StubberEndpoint('/document/commit_candidate', ['GET', 'POST']),
            StubberEndpoint('/document/create_candidate', ['GET', 'POST']),
        ],
        scheme='grpc',
        stubs_subdir='create_or_commit_shared_link_candidate',
    )


nlgsearch_stubber = create_nlgsearch_stubber_fixture()
seq2seq_stubber = create_seq2seq_stubber_fixture()
entity_search_stubber = create_entity_search_stubber_fixture()
create_or_commit_shared_link_stubber = create_create_or_commit_shared_link_stubber_fixture()


@pytest.fixture(scope="function")
def srcrwr_params(nlgsearch_stubber, seq2seq_stubber, entity_search_stubber, create_or_commit_shared_link_stubber):
    return {
        'GENERAL_CONVERSATION_SCENARIO_PROXY_CANDIDATES_SELECTIVE': f'localhost:{nlgsearch_stubber.port}',
        'GENERAL_CONVERSATION_SCENARIO_PROXY_CANDIDATES_SEQ2SEQ': f'localhost:{seq2seq_stubber.port}',
        'GENERAL_CONVERSATION_SCENARIO_PROXY_SUGGESTS': f'localhost:{nlgsearch_stubber.port}',
        'GENERAL_CONVERSATION_SCENARIO_PROXY_RERANKER_BERT': f'localhost:{seq2seq_stubber.port}',
        'GENERAL_CONVERSATION_SCENARIO_PROXY_ENTITY_SEARCH': f'localhost:{entity_search_stubber.port}',
        'ALICE_SOCIAL_SHARE': f'localhost:{create_or_commit_shared_link_stubber.port}',
    }
