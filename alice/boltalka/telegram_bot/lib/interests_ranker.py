import numpy as np
import pandas as pd
from alice.boltalka.telegram_bot.lib.module import Module
from alice.boltalka.py_libs.apply_nlg_dssm import apply_nlg_dssm
from alice.boltalka.telegram_bot.lib.cache import Cache

class InterestsRanker(Module):
    class Options:
        weight = 390
        interest = 'собаки'
        kickout = 10
        path_to_dssm_applier = '/place/home/nickpon/arcadia/junk/nickpon/interests_alice_bot/tests/data'
        path_to_to_interests_table = '~/arcadia/alice/boltalka/telegram_bot/top_500_interests'
    
    def __init__(self):
        self.dssm_applier = Cache(lambda path: apply_nlg_dssm.NlgDssmApplier(path))
        self.top_interests = Cache(lambda path: [[el] for el in list(pd.read_csv(path)['interest'])])
        self.interests_emb = Cache(lambda applier, interests:
                                   np.array([np.array(el).reshape((-1, 300)) for el in
                                             np.array(applier.get_embeddings(interests, [''] * len(self.top_interests)))])[0])

    def set_args(self, args):
        super().set_args(args)
        self.dssm_applier.choose(self.path_to_dssm_applier)
        self.top_interests.choose(self.path_to_to_interests_table)
        self.interests_emb.choose(self.dssm_applier, self.top_interests)

    def softmax_kickout(self, scores, pos_interest):
        e_x = np.exp(np.array(scores) / 0.105)
        return float(e_x[pos_interest]) / (np.sum(-np.partition(-np.delete(e_x, pos_interest), self.kickout)[self.kickout:]) +
                                           e_x[pos_interest])

    def get_scores(self, args, context, candidates):
        self.set_args(args)
        pos_interest = self.top_interests.index([self.interest])
        candidates = [el['text'] for el in candidates]
        candidates_emb = np.array([np.array(el).reshape((-1, 300)) for el in
                                   np.array(self.dssm_applier.get_embeddings([['']] * len(candidates), candidates))])[1]
        dot = np.dot(self.interests_emb, candidates_emb.T).T
        return [self.softmax_kickout(dot[i], pos_interest) for i in range(len(dot))]
