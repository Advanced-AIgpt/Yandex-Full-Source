class BaseReplacer(object):
    def __init__(self, args):
        pass

    def start(self, local=False):
        pass

    def get_yt_extra_args(self):
        return {}

    def process(self, reply):
        return reply

    def process_row(self, row):
        row['rewritten_reply'] = self.process(row['rewritten_reply'])
        return row

    def __call__(self, row):
        key = 'rewritten_reply' if 'rewritten_reply' in row else 'reply'
        reply = unicode(row[key], 'utf-8')
        row['rewritten_reply'] = reply
        row = self.process_row(row)
        if row['rewritten_reply']:
            yield row

    @staticmethod
    def register_args(parser):
        pass
