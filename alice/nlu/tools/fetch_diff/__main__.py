# coding: utf-8
import argparse
import requests
import sys


class ArcanumAPI:
    DIFF_SETS_URL_TEMPLATE = 'https://a.yandex-team.ru/api/v1/review-requests/{review_id}/diff-sets/'
    DIFF_URL_TEMPLATE = 'https://a.yandex-team.ru/api/v1/review-requests/{review_id}/diff-sets/{diff_set_id}/patch'

    def __init__(self, oauth_token):
        self._oauth_token = oauth_token

    def get(self, url):
        return requests.get(url, headers={'Authorization': 'OAuth {0}'.format(self._oauth_token)})

    def get_last_raw_diff(self, review_id):
        r = self.get(self.DIFF_SETS_URL_TEMPLATE.format(review_id=review_id))
        r.raise_for_status()
        last_raw_diff_set_id = r.json()['data'][-1]['id']
        r = self.get(self.DIFF_URL_TEMPLATE.format(review_id=review_id, diff_set_id=last_raw_diff_set_id))
        r.raise_for_status()
        return r.text


ARCANUM_URI_PREFIX = 'arc:'


def get_diff(review_uri, oauth_token=None):
    if review_uri.startswith(ARCANUM_URI_PREFIX):
        return ArcanumAPI(oauth_token).get_last_raw_diff(review_uri[len(ARCANUM_URI_PREFIX):])
    if review_uri.startswith('http'):
        headers = {}
        if oauth_token:
            headers['Authorization'] = 'OAuth {0}'.format(oauth_token)
        return requests.get(review_uri, headers=headers).text
    raise ValueError('Unknown URI: {0}'.format(review_uri))


def main(argv):
    parser = argparse.ArgumentParser()
    parser.add_argument('review_uri', metavar='REVIEW_URI', nargs=1, help='Review URI')
    parser.add_argument('--oauth-token', help='OAuth token')
    opts = parser.parse_args(argv)
    print(get_diff(opts.review_uri[0], opts.oauth_token))
    return 0


if __name__ == '__main__':
    sys.exit(main(sys.argv[1:]))
