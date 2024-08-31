import argparse
import yt.wrapper as yt
import string


PUNCTUATION = string.punctuation.replace('"', '').replace('\'', '').replace('@', '').replace('#', '')


def strip_link(text, link, start=False, end=False):
    if start and text.startswith(link):
        text = text[len(link):]
    if end and text.endswith(link):
        text = text[:-len(link)]

    return text


def strip_punct(text, start=False, end=False):
    if start:
        text = text.lstrip(PUNCTUATION)
    if end:
        text = text.rstrip(PUNCTUATION)

    return text

def strip_quotes(text):
    if len(text) > 1 and (text[0] == '"' and text[-1] == '"'):
        text = text[1:-1]
    if len(text) > 1 and (text[0] == '\'' and text[-1] == '\''):
        text = text[1:-1]

    return text


def is_to_filter(link, link_filters):
    if link_filters is None:
        return False

    for f in link_filters:
        if link.startswith(f):
            return True

    return False


class KeyValueBuilder(object):
    def __init__(self, args):
        self.start_strip_link_filters = args.start_strip_link_filters
        self.end_strip_link_filters = args.end_strip_link_filters
        self.start_strip_punct = args.start_strip_punct
        self.end_strip_punct = args.end_strip_punct
        self.quote_strip = args.quote_strip

    def preprocess_tweet(self, text_raw, links):
        if text_raw is None:
            return ''
        if links is None:
            links = []

        text = text_raw.strip()
        for link in links:
            text = strip_link(
                    text,
                    link,
                    is_to_filter(link, self.start_strip_link_filters),
                    is_to_filter(link, self.end_strip_link_filters)
            )
        text = strip_punct(text, self.start_strip_punct, self.end_strip_punct)

        if self.quote_strip:
            text = strip_quotes(text)

        if text == text_raw:
            return text

        return self.preprocess_tweet(text, links)

    def __call__(self, row):
        for message in row['Result']:
            last_tweet = None
            for tweet in message['ancestor']:
                tweet['processed_text'] = self.preprocess_tweet(tweet['text-raw'], tweet['links'])
                yield {
                    'key': tweet['data-item-id'],
                    'subkey': last_tweet['data-item-id'] if last_tweet else None,
                    'value': ' '.join([str(tweet['data-user-id']), tweet['processed_text']]),
                    'lang': tweet['lang']
                }
                last_tweet = tweet
            tweet = message['tweet']
            if tweet is None:
                continue

            tweet['processed_text'] = self.preprocess_tweet(tweet['text-raw'], tweet['links'])
            yield {
                'key': tweet['data-item-id'],
                'subkey': last_tweet['data-item-id'] if last_tweet else None,
                'value': ' '.join([str(tweet['data-user-id']), tweet['processed_text']]),
                'lang': tweet['lang']
            }


class DeduplicateReducer(object):
    def __init__(self, lang_filters):
        self.lang_filters = lang_filters

    def __call__(self, key, rows):
        for row in rows:
            if self.lang_filters is None or row['lang'] in self.lang_filters:
                yield row
                break


def main(args):
    yt.run_map_reduce(
            KeyValueBuilder(args),
            DeduplicateReducer(args.lang_filters),
            args.src,
            args.dst,
            reduce_by=['key'],
            sort_by=['key', 'subkey', 'value']
    )

if __name__ == '__main__':
    parser = argparse.ArgumentParser(add_help=True)
    parser.add_argument('--src', required=True)
    parser.add_argument('--dst', required=True)
    parser.add_argument('--start-strip-link-filters', choices=['#', '@', 'pic', 'http'], nargs='*')
    parser.add_argument('--end-strip-link-filters', choices=['#', '@', 'pic', 'http'], nargs='*')
    parser.add_argument('--start-strip-punct', action='store_true')
    parser.add_argument('--end-strip-punct', action='store_true')
    parser.add_argument('--quote-strip', action='store_true')
    parser.add_argument('--lang-filters', nargs='*')
    args = parser.parse_args()
    main(args)
