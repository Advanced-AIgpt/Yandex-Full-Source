# -*- coding: utf-8 -*-
from yandex_lemmer import AnalyzeWord
from catboost import CatBoostClassifier

CONTEXT_RADIUS = 2
PAD_WORD = u"<none>"
PAD = [PAD_WORD] * CONTEXT_RADIUS
CASES = ["nom", "gen", "acc", "dat", "ins", "abl"]


def get_default(container, index):
    try:
        return container[index]
    except IndexError:
        return PAD_WORD


def get_features(word):
    info = AnalyzeWord(word)
    if not info:
        return {"pos": PAD_WORD, "case": PAD_WORD, "lemma": word}
    info = info[0]
    case = PAD_WORD
    if info.FormFeature:
        for el in info.FormFeature[0]:
            if el in CASES:
                case = el
                break
    return {
        "pos": get_default(info.LexicalFeature, 0),
        "case": case,
        "lemma": info.Lemma
    }


def expand_context(context):
    row = {}
    for i in range(2 * CONTEXT_RADIUS):
        for key in context[i]:
            row["{}{}".format(key, i)] = context[i][key]
    return row


def get_context_features(context, index):
    return expand_context([
        get_features(get_default(context, i))
        for i in xrange(index - CONTEXT_RADIUS, index + CONTEXT_RADIUS + 1)
        if i != index
    ])


def features_as_list(features):
    ans = []
    names = ["pos{}", "case{}", "lemma{}"]
    for i in range(4):
        for feature in names:
            ans.append(features[feature.format(i)].encode("utf-8"))
    return ans


class RespectClassifierModel(object):
    def __init__(self):
        self.model = CatBoostClassifier()

    def load_model(self, path):
        self.model.load_model(path)

    def predict(self, tokens, index):
        data = [features_as_list(get_context_features(tokens, index))]
        proba = self.model.predict_proba(data)[0]
        if proba[0] > proba[1]:
            return u"вас", proba[0]
        return u"вам", proba[1]
