#!/usr/bin/python
import sys
import argparse
import yt.wrapper as yt
import simplejson as json
import regex as re

yt.config.set_proxy("hahn.yt.yandex.net")

class TwitsPreprocessor(object):
    def __init__(self, args):
        self.skip_urls = args.skip_urls
        self.skip_hashtags = args.skip_hashtags
        self.strip_mentions = args.strip_mentions
        self.strip_non_utf = args.strip_non_utf
        self.url_regex = re.compile('.*https?://.*')
        self.hashtag_regex = re.compile('.*#[^\s]+.*')
        self.mention_regex = re.compile('@[^\s]+')
        self.utf_regex = r'[\w\p{P}$+<=>^`|~\t\s]+'

    def __call__(self, row):
        key = row['key']
        value = row['value']
        if self.skip_urls and self.url_regex.match(value):
            return
        if self.skip_hashtags and self.hashtag_regex.match(value):
            return
        if 'New post: ' in value: # most of them are ads
            return
        d = json.loads(value)
        user_id = d['user_id']
        text = d['text']
        if self.strip_mentions:
            text = re.sub(self.mention_regex, '', text)
        if self.strip_non_utf:
            text = ''.join(re.findall(self.utf_regex, text, flags=re.U))
        text = re.sub(r'\s+', ' ', text, flags=re.U).strip()
        if text:
            yield {'key': key, 'subkey': row['subkey'], 'value': user_id + ' ' + text}

def main(args):
    yt.run_map(TwitsPreprocessor(args), args.src, args.dst)

if __name__ == '__main__':
    parser = argparse.ArgumentParser(add_help=True)
    parser.add_argument('--src', required=True)
    parser.add_argument('--dst', required=True)
    parser.add_argument('--skip-urls', action='store_true', help='Skip dialogs with urls')
    parser.add_argument('--skip-hashtags', action='store_true', help='Skip dialogs with hashtags')
    parser.add_argument('--strip-mentions', action='store_true', help='Strip mentions (@user_name)')
    parser.add_argument('--strip-non-utf', action='store_true')
    args = parser.parse_args()

    main(args)
