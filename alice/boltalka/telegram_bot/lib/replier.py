import numpy as np
from alice.boltalka.telegram_bot.lib.module import Module


class Source(Module):
    def get_candidates(args, context):
        raise NotImplementedError()


class Ranker(Module):
    def get_scores(args, context, candidates):
        raise NotImplementedError()


class Replier(Module):
    class Options:
        sources = []
        rankers = []

    def get_replies(self, args, context, modules, module_options):
        self.set_args(args)
        candidates = []
        for source in self.sources:
            source_candidates = []
            try:
                source_candidates = modules[source].get_candidates(module_options[source], context)
            except Exception as e:
                print(f"Source {source} failed")
                print(e)
            for el in source_candidates:
                el['source'] = f"{source}:{el.get('source', '')}"
            candidates += source_candidates
        scores = []
        for ranker in self.rankers:
            options = module_options[ranker]
            weight = options['weight']
            scores.append(
                np.array(modules[ranker].get_scores(options, context, candidates)) * weight
                if weight > 0 else
                [0] * len(candidates)
            )
        scores = np.array(scores)
        sum_scores = np.sum(scores, axis=0)
        for candidate, score, sum_score in zip(candidates, scores.T, sum_scores):
            candidate['score'] = sum_score
            candidate['score_explain'] = list(score)
        candidates.sort(key=lambda x: -x['score'])
        return candidates
