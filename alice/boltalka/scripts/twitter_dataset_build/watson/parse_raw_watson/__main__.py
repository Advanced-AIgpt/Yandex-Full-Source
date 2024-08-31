import argparse
import re
import yt.wrapper as yt
from alice.boltalka.scripts.twitter_dataset_build.watson.constants import EMPTY_TOKEN

yt.config.set_proxy("hahn.yt.yandex.net")


def replace_tabs(text):
    return re.sub(r'\t+', ' ', text, flags=re.U).strip()


class Mapper(object):
    def __init__(self, tas_format):
        self.tas_format = tas_format
    def __call__(self, row):
        json = row['Result'][0]
        tweets = json['ancestor'] + [json['tweet']]
        if not (len(tweets) > 1 and tweets[-1]):
            return
        out = {
            'url': row['Url'],
            'root_tweet_id': tweets[0]['data-tweet-id'],
            'root_user_id': tweets[0]['data-user-id'],
            'root_user_name': tweets[0]['data-screen-name'],
        }
        texts = []
        users = []
        langs = []
        for tweet in tweets:
            users.append(tweet['data-user-id'])
            texts.append(tweet['text'] if tweet['text'] is not None else EMPTY_TOKEN)
            langs.append(tweet['lang'] if tweet['lang'] is not None else '')
        out['langs'] = '\t'.join(langs)
        if self.tas_format:
            out['key'] = out['root_tweet_id']
            out['value'] = '\t'.join([user + ' ' + text for user, text in zip(users, texts)])
        else:
            out['text'] = '\t'.join(texts)
            out['users'] = '\t'.join(users)
        yield out


def main(args):
    yt.run_map(Mapper(args.tas_format), args.src, args.dst)
    yt.run_sort(args.dst, sort_by='root_tweet_id')


if __name__ == '__main__':
    parser = argparse.ArgumentParser(add_help=True)
    parser.add_argument('--src', required=True)
    parser.add_argument('--dst', required=True)
    parser.add_argument('--tas-format', action='store_true')
    args = parser.parse_args()
    main(args)
