import sys
sys.path.append('../py_libs/nlgsearch_simple/')
import nlgsearch


class NlgsearchSource:
    def __init__(self, index_dir):
        self.search = nlgsearch.NlgSearch(index_dir, 20, "insight_c3_rus_lister,factor_dssm_0_index", ranker_model_name="assessors_catboost",
                                          base_dssm_model_name="insight_c3_rus_lister", knn_index_names="base,assessors", base_knn_index_name="base")

    def get_candidates(self, args, context):
        self.set_args(args)
        candidates = self.search.get_reply_texts(context)
        return [{'text': el, 'relevance': 0} for el in candidates]
