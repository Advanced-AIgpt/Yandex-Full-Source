# coding: utf-8
import json

import attr
import pytest
import requests_mock
from requests.exceptions import RequestException

from alice.review_bot.lib.helpers import (
    ARCANUM_BASE_URL,
    ReviewInfo,
    get_review_info,
    merge_reviews,
    parse_review_id,
    update_review,
    update_state,
    parse_review_data,
    format_review,
    format_review_list,
)


def create_review(review_id=1, author='author', title='title',
                  reviewers=(), merged=False, timestamp=0, updated=0):
    return ReviewInfo(
        review_id=review_id,
        author=author,
        title=title,
        reviewers=reviewers,
        merged=merged,
        timestamp=timestamp,
        updated=updated,
    )


@pytest.mark.parametrize('url, expected', [
    ('https://a.yandex-team.ru/review/937858/files/', 937858),
    ('https://a.yandex-team.ru/review/937858/', 937858),
    ('https://a.yandex-team.ru/review/937858', 937858),
    ('https://a.yandex-team.ru/arc/trunk/arcadia/alice', None),
    ('http://ya.ru', None),
])
def test_parse_review_id(url, expected):
    assert parse_review_id(url) == expected


def test_update_review(mocker):
    review = create_review(
        reviewers=['a', 'b', 'c'],
        merged=False,
    )

    new_review = attr.evolve(review, reviewers=[], merged=True)

    mocker.patch(
        'alice.review_bot.lib.helpers.get_review_info',
        return_value=new_review,
    )

    updated_review = update_review(review, 'token')
    assert updated_review.merged is True
    assert updated_review.reviewers == {'a', 'b', 'c'}


def test_merge_reviews():
    cur_state = [
        create_review(1, reviewers=['a'], merged=True),
        create_review(2, reviewers=['a', 'b']),
    ]
    new_state = [
        create_review(2, timestamp=2),
        create_review(3, merged=True, timestamp=3),
        create_review(4, timestamp=4),
    ]

    state = merge_reviews(cur_state, new_state, ['c'])

    assert state == [
        create_review(2, reviewers=['a', 'b', 'c'], timestamp=2),
        create_review(4, reviewers=['c'], timestamp=4),
    ]


def test_merge_reviews2():
    cur_state = [
        create_review(review_id=1, reviewers={'a'}, timestamp=1),
        create_review(review_id=2, reviewers={'a', 'b'}, timestamp=2),
    ]

    new_state = [create_review(review_id=3, timestamp=3)]

    assert merge_reviews(cur_state, new_state, ['c']) == [
        create_review(review_id=1, reviewers={'a'}, timestamp=1),
        create_review(review_id=2, reviewers={'a', 'b'}, timestamp=2),
        create_review(review_id=3, reviewers={'c'}, timestamp=3)
    ]


def test_update_state(mocker):
    cur_state = [
        create_review(1, reviewers=['a'], merged=True).to_dict(),
        create_review(2, reviewers=['a', 'b']).to_dict(),
    ]

    urls = ['https://a.yandex-team.ru/review/3/']
    mentions = ['c']

    mocker.patch(
        'alice.review_bot.lib.helpers.get_review_info',
        side_effect=lambda review_id, token: create_review(
            review_id,
            timestamp=review_id,
            merged=review_id == 1,
        )
    )

    assert list(map(
        ReviewInfo.from_dict,
        update_state(cur_state, urls, mentions, 'token')[0],
    )) == [
        create_review(2, reviewers=['a', 'b'], timestamp=2),
        create_review(3, reviewers=['c'], timestamp=3),
    ]


JSON_RESPONSE = b'''
    {
      "merge": false,
      "reqs": {
        "ship": {
          "done": false
        },
        "build": {
          "done": true
        },
        "tests": {
          "done": true
        },
        "no_draft_diffs": {
          "done": true
        }
      },
      "author": "the0",
      "summary": "Test review",
      "description": "",
      "state": "pending_review",
      "patches": [
        {
          "id": 1982045,
          "revision": 1,
          "created_time": "2019-10-14T16:30:02.211542Z",
          "published": true,
          "base_dir": "/",
          "base_rev": "5824241",
          "svn_base_rev": 5824241,
          "zipatch": "https://storage-int.mds.yandex.net/get-arcadia-review/860113/4f5d10535138ce368d5edee02156a1d2f7dc6a09/review-svn.zipatch",
          "gsid": "USER:the0 YA:66rtl23eb5ahssar SVN_TXN:5824241-3ybj5",
          "in_arcadia": true
        },
        {
          "id": 1982046,
          "revision": 2,
          "created_time": "2019-10-15T16:30:04.211542Z",
          "published": true,
          "base_dir": "/",
          "base_rev": "5824241",
          "svn_base_rev": 5824241,
          "zipatch": "https://storage-int.mds.yandex.net/get-arcadia-review/860113/4f5d10535138ce368d5edee02156a1d2f7dc6a09/review-svn.zipatch",
          "gsid": "USER:the0 YA:66rtl23eb5ahssar SVN_TXN:5824241-3ybj5",
          "in_arcadia": true
        }
      ],
      "branch": "",
      "repository": {
        "name": "arc",
        "type": "svn"
      }
    }'''


def test_get_review_info():
    with requests_mock.Mocker() as m:
        m.get(ARCANUM_BASE_URL + 'api/v1/pullrequest/' + str(989769), content=JSON_RESPONSE)
        assert get_review_info(989769, 'token') == ReviewInfo(
            review_id=989769,
            author='the0',
            title="Test review",
            reviewers=[],
            merged=False,
            timestamp=1571070602,
            updated=1571157004,
        )


def test_get_review_info_retry(mocker):
    REVIEW_ID = 989769
    sleep = mocker.patch('alice.review_bot.lib.helpers.gevent.sleep')

    try_ = []

    def callback(request, context):
        if len(try_) == 0:
            try_.append(1)
            raise RequestException
        else:
            return JSON_RESPONSE

    with requests_mock.Mocker() as m:
        m.get(ARCANUM_BASE_URL + 'api/v1/pullrequest/' + str(REVIEW_ID), content=callback)

        assert get_review_info(REVIEW_ID, 'token') == ReviewInfo(
            review_id=REVIEW_ID,
            author='the0',
            title="Test review",
            reviewers=[],
            merged=False,
            timestamp=1571070602,
            updated=1571157004,
        )

        assert sleep.call_count == 1


def test_format_review():
    review = parse_review_data(123, json.loads(JSON_RESPONSE))
    return format_review(review)


def test_format_review_list():
    review1 = parse_review_data(123, json.loads(JSON_RESPONSE))
    review2 = parse_review_data(321, json.loads(JSON_RESPONSE))

    review2.title = 'Test title2'
    review2.timestamp -= 3600 * 24 * 7  # minus one week
    review2.updated += 1                # more recent

    return format_review_list([review1, review2])


def test_parsing_without_patches():
    data = json.loads(JSON_RESPONSE)
    data['patches'] = []
    review1 = parse_review_data(123, data)

    assert review1.timestamp == review1.updated == 0
