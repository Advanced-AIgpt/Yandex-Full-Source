# -*- coding: utf-8 -*-
import logging
import pytest

from alice.hollywood.library.python.testing.integration.conftest import read_run_request_prototext, requester,\
    create_hollywood_fixture
from alice.hollywood.library.python.testing.integration.test_functions import request_run
from alice.hollywood.library.python.testing.it2.stubber import create_stubber_fixture, StubberEndpoint
from yatest import common as yc


logger = logging.getLogger(__name__)

assert requester  # To satisfy the linter


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
            StubberEndpoint('/bert_factor', ['POST']),
        ],
        stubs_subdir='seq2seq',
    )


@pytest.fixture(scope="function")
def srcrwr_params(nlgsearch_stubber, seq2seq_stubber, entity_search_stubber):
    return {
        'GENERAL_CONVERSATION_SCENARIO_PROXY_CANDIDATES_SELECTIVE': f'localhost:{nlgsearch_stubber.port}',
        'GENERAL_CONVERSATION_SCENARIO_PROXY_CANDIDATES_SEQ2SEQ': f'localhost:{seq2seq_stubber.port}',
        'GENERAL_CONVERSATION_SCENARIO_PROXY_SUGGESTS': f'localhost:{nlgsearch_stubber.port}',
        'GENERAL_CONVERSATION_SCENARIO_PROXY_RERANKER_BERT': f'localhost:{seq2seq_stubber.port}',
        'GENERAL_CONVERSATION_SCENARIO_PROXY_ENTITY_SEARCH': f'localhost:{entity_search_stubber.port}'
    }


SCENARIO_HANDLE = 'general_conversation'
BASE_REQUEST = yc.source_path('alice/hollywood/library/scenarios/general_conversation/shooter/data/run_request.pb.txt')
REQUESTS = yc.source_path('alice/hollywood/library/scenarios/general_conversation/shooter/data/requests.tsv')
RESPONSES = yc.source_path('alice/hollywood/library/scenarios/general_conversation/shooter/data/responses.tsv')

hollywood = create_hollywood_fixture([SCENARIO_HANDLE])
nlgsearch_stubber = create_nlgsearch_stubber_fixture()
seq2seq_stubber = create_seq2seq_stubber_fixture()
entity_search_stubber = create_entity_search_stubber_fixture()


def test_gc_run(requester, srcrwr_params):
    request = read_run_request_prototext(BASE_REQUEST)
    request.BaseRequest.Experiments["hw_gc_enable_aggregated_reply"] = 1
    request.BaseRequest.Experiments["hw_gc_reply_RankByLinearCombination=false"] = 1
    request.BaseRequest.Experiments["hw_gc_reply_MaxResults=54"] = 1
    request.BaseRequest.Experiments["hw_gc_reply_UniqueReplies=0"] = 1
    with open(REQUESTS, 'r') as f_r:
        for i, line in enumerate(f_r):
            sp = line.rstrip('\n').split('\t')
            assert len(sp) % 2 == 1

    with open(REQUESTS, 'r') as f_r, open(RESPONSES, 'w') as f_w:
        for line in f_r:
            sp = line.rstrip('\n').split('\t')
            for i in range(1, len(sp), 2):
                dialogTurn = request.DataSources[7].DialogHistory.DialogTurns.add()
                dialogTurn.Request = sp[i - 1]
                dialogTurn.Response = sp[i]
            request.Input.Text.Utterance = sp[-1]
            response = request_run(requester, request, SCENARIO_HANDLE, srcrwr_params)
            text = response.ResponseBody.Layout.Cards[0].Text
            source = response.ResponseBody.AnalyticsInfo.Objects[0].GCResponseInfo.Source
            print(text, source, sep='\t', file=f_w)
