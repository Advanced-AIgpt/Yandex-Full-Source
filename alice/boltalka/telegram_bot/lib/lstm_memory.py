from alice.boltalka.telegram_bot.lib.module import Module
from alice.boltalka.memory.lstm_dssm.py_apply.model import Model
from alice.boltalka.py_libs.apply_nlg_dssm import apply_nlg_dssm
from alice.boltalka.telegram_bot.lib.cache import Cache
import numpy as np

class LSTMMemoryReranker(Module):
    class Options:
        weight=1
        path_to_dssm_applier = '/mnt/storage/alzaharov/Interests_LSTM/base_model'
        path_to_lstm_model = '/mnt/storage/alzaharov/Interests_LSTM/lstm_dyn.pb'
        path_to_dssm_model = '/mnt/storage/alzaharov/Interests_LSTM/dssm_dyn2.pb'

    def __init__(self):
        self.model = Cache(lambda lstm_path, dssm_path: Model(lstm_path, dssm_path))
        self.dssm_applier = Cache(lambda applier_path: apply_nlg_dssm.NlgDssmApplier(applier_path))
        self._state = np.zeros((1,2,256))

    def set_args(self, args):
        super().set_args(args)
        self.model.choose(self.path_to_lstm_model, self.path_to_dssm_model)
        self.dssm_applier.choose(self.path_to_dssm_applier)

    def get_scores(self, args, context, candidates):
        self.set_args(args)
        candidates = [el['text'] for el in candidates]
        prev_reply_embedding = None
        if len(context) > 2:
            prev_reply_embedding = np.array(self.dssm_applier.get_embeddings([['']], [context[-2]]))[1:]
        context_embedding = np.array(self.dssm_applier.get_embeddings([context], [""]))[:1]

        candidates_emb = np.array([np.array(el).reshape((-1, 300)) for el in
                                   np.array(self.dssm_applier.get_embeddings([['']] * len(candidates), candidates))])[1]
        scores, self._state = self.model.apply(prev_reply_embedding, context_embedding,candidates_emb, self._state)
        return scores
