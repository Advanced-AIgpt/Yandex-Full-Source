# coding: utf-8

import calendar
import html
import logging
import re
import time
from random import random

import attr
import gevent
import requests
from dateutil.parser import parse

ARCANUM_BASE_URL = 'https://a.yandex-team.ru/'
arc_pattern = re.compile(ARCANUM_BASE_URL + r'review/(\d+)/?.*')
logger = logging.getLogger(__name__)


def to_timestemp(date_string):
    return calendar.timegm(parse(date_string).timetuple())


def get_review_info(review_id, token):
    attemps = 10
    factor = 0.3

    for i in range(attemps):
        try:
            resp = requests.get(
                ARCANUM_BASE_URL + 'api/v1/pullrequest/' + str(review_id),
                headers={
                    'accept': 'application/json',
                    'Authorization': 'OAuth {}'.format(token),
                },
            )
            resp.raise_for_status()
            data = resp.json()
            logger.info('Arcanum response %s %s', resp.url, data)
            break
        except requests.exceptions.RequestException as exc:
            logger.warning(
                "Can't get review info %s. Attempt %s. %s",
                review_id, i, exc
            )
            if i > attemps - 1:
                raise
            else:
                gevent.sleep(factor * i + random())

    return parse_review_data(review_id, data)


def parse_review_data(review_id, data):
    patches = sorted(data['patches'], key=lambda x: x['revision'])

    if patches:
        created = to_timestemp(patches[0]['created_time'])
        updated = to_timestemp(patches[-1]['created_time'])
    else:
        logger.warning('Empty patches for review %s %s', review_id, data)
        if 'created_time' in data:
            created = updated = to_timestemp(data['created_time'])
        else:
            created = updated = 0

    return ReviewInfo(
        review_id=review_id,
        author=data['author'],
        title=data['summary'],
        reviewers=[],
        merged=data['state'] in ('discarded', 'submitted'),
        timestamp=created,
        updated=updated,
    )


@attr.s
class ReviewInfo(object):
    review_id = attr.ib(type=int)
    author = attr.ib(type=str)
    title = attr.ib(type=str)
    reviewers = attr.ib(type=set, converter=set)
    merged = attr.ib(type=bool)
    timestamp = attr.ib(type=float)
    updated = attr.ib(type=float)

    def to_dict(self):
        return attr.asdict(self)

    @classmethod
    def from_dict(cls, data):
        return cls(**data)

    def update_reviewers(self, reviewers):
        self.reviewers |= set(reviewers)


def update_review(review, token):
    new_review = get_review_info(review.review_id, token)
    new_review.update_reviewers(review.reviewers)
    return new_review


def parse_review_id(url):
    match = arc_pattern.match(url)
    if match:
        logger.info('Review mentioned %s', url)
        review_id = int(match.groups()[0])
        return review_id


def merge_reviews(cur_reviews, new_reviews, mentions):
    res = []
    cur_reviews = {r.review_id: r for r in cur_reviews}

    for review in new_reviews:
        if review.review_id in cur_reviews:
            review.update_reviewers(
                cur_reviews[review.review_id].reviewers
            )
        review.update_reviewers(mentions)
        cur_reviews[review.review_id] = review

    res = [rev for rev in cur_reviews.values() if not rev.merged]
    res.sort(key=lambda x: x.timestamp)
    return res


def update_state(state, urls, mentions, token):
    new_ids = map(parse_review_id, urls)
    new_reviews = list(get_review_info(id_, token) for id_ in new_ids if id_)

    old_reviews = (update_review(ReviewInfo.from_dict(review), token) for review in state)

    new_state = merge_reviews(old_reviews, new_reviews, mentions)
    return list(map(lambda x: x.to_dict(), new_state)), new_reviews


def get_smile(review):
    """ Get emoji that represents age of the last review patch"""
    now = time.time()

    day = 24 * 3600
    delta = float(now - review.updated) / day

    if delta < 0.5:
        return u'\U0001f47c'   # newborn
    elif delta < 1:
        return u'\U0001f476'   # baby
    elif delta < 3:
        return u'\U0001f9d2'   # young
    elif delta < 5:
        return u'\U0001f468'   # grown
    elif delta < 10:
        return u'\U0001f474'   # old
    elif delta < 14:
        return u'\U0001f480'   # dead
    else:
        return u'\U000026B0'   # coffin


def format_review(review):
    url_tmplt = '<a href="{url}">{title}</a>'
    url = url_tmplt.format(
        url=ARCANUM_BASE_URL + 'review/' + str(review.review_id),
        title=html.escape(review.title),
    )

    res = '{smile} [{author}] {url}'.format(
        author=review.author,
        smile=get_smile(review),
        url=url,
    )

    if review.reviewers:
        res += ', ' + ' '.join(review.reviewers)

    return res


def format_review_list(reviews):
    messages = []
    for i, review in enumerate(reviews, 1):
        messages.append("{}. {}".format(i, format_review(review)))

    return '\n'.join(messages)
