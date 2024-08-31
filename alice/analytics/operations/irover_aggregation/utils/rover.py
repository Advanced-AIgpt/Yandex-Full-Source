from utils.word_transition_network import WordTransitionNetwork


class RoverVotingScheme(WordTransitionNetwork):
    def get_result(self):
        result = []
        for edges in self.edges:
            score, _, value = max((len(set(x.sources)), len(x.value), x.value) for x in edges.values())
            score = float(score)
            score /= len(set(self.hypotheses_sources))
            result.append((value, score))
        return result
