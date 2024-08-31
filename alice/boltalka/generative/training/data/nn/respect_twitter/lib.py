import os

import vh
from alice.boltalka.generative.training.data.nn.util.experimental.ops import map_fn_lazy
from alice.boltalka.generative.training.data.nn.util.lib import tokenize_columns
from alice.boltalka.generative.training.data.nn.util.ops import get_yt_table, mr_left_join, mr_sort, rewrite_respect, \
    get_yt_file, mr_copy, train_test_split_by_column


class ReplyDetokenizer:
    def __call__(self, row):
        row['reply'] = row['reply'].replace(' `', '')
        yield row


class ReplyRespectToReply:
    def __call__(self, row):
        row['reply'] = row['reply_respect']
        yield row


class TruncateBertScore:
    def __init__(self, min_value):
        self.min_value = min_value

    def __call__(self, row):
        if row['bert_score'] >= self.min_value:
            yield row


def main():
    with vh.Graph() as g:
        # TODO put scoring by bert on VH as well
        folder = '//home/voice/artemkorenev/boltalka/twitter_filtered_whitelist'

        table = get_yt_table(os.path.join(folder, 'train'))
        bert_scores_table = get_yt_table(os.path.join(folder, 'train.scores_by_bert'))
        table = mr_left_join(table, bert_scores_table, join_by=['key'])
        table = map_fn_lazy(table, TruncateBertScore(2.02))
        table = map_fn_lazy(table, ReplyDetokenizer())
        table = rewrite_respect(table, source_key='reply', target_key='reply_respect', target_gender='femn')
        table = tokenize_columns(
            table,
            bpe_voc=get_yt_file('//home/voice/artemkorenev/boltalka/bart_lm/bpe.voc'),
            columns_to_tokenize=['reply_respect']
        )
        table = map_fn_lazy(table, ReplyRespectToReply())
        table = mr_sort(table, sort_by=['bert_score'])

        train, val = train_test_split_by_column(table, test_size=0.05, column='key')

        mr_copy(train, os.path.join(folder, 'bert_and_respect', 'train'))
        mr_copy(val, os.path.join(folder, 'bert_and_respect', 'val'))

    vh.run(g, quota='dialogs', yt_token_secret='artemkorenev_yt_token', yt_proxy='hahn', yt_pool='voice').get_workflow_info()
