import os
import codecs
from collections import OrderedDict
from alice.boltalka.py_libs.normalization.normalization import normalize
from alice.boltalka.tools.reply_rewriter.base_replacer import BaseReplacer


def load_replace(filename):
    dct = OrderedDict()
    with codecs.open(filename, 'r', 'utf-8') as inp:
        for line in inp:
            if not line.strip() or line.startswith('#'):
                continue
            reply, replace = line.rstrip('\r\n').split('\t')
            reply = normalize(reply)
            dct[reply] = replace
    return dct


def substitute_reply(replace, reply):
    key = normalize(reply)
    return replace[key] if key in replace else reply


class RewrittenReplacer(BaseReplacer):
    def __init__(self, args):
        self.replace_file = args.rewritten_replies_file
        self.replace = None

    def start(self, local=False):
        if not local:
            self.replace_file = os.path.basename(self.replace_file)
        self.replace = load_replace(self.replace_file)

    def get_yt_extra_args(self):
        return dict(local_files=[self.replace_file])

    def process(self, reply):
        return substitute_reply(self.replace, reply)

    @staticmethod
    def register_args(parser):
        parser.add_argument('--rewritten-replies-file')
