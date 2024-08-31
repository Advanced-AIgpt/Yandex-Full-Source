# -*- coding: utf-8 -*-
import os
import sys
import re
import codecs

from yandex_lemmer import AnalyzeWord
from alice.boltalka.tools.reply_rewriter.base_replacer import BaseReplacer
from alice.boltalka.py_libs.normalization.normalization import tokenize
from alice.boltalka.py_libs.respect_classifier import RespectClassifierModel

YOU = AnalyzeWord(u"вы")[0]
YOURS = AnalyzeWord(u"ваш")[0]
FREQUENCY_THRESHOLD = 0.01
PROBA_THRESHOLD = 0.9
SKIP_LEMMAS = set((u"прямой",))


class Failure(object):
    def __init__(self, *tags):
        self.tags = tags

    def add(self, *tags):
        if isinstance(tags[0], Failure):
            self.tags += tags[0].tags
        else:
            self.tags += tags

    def format(self):
        return u",".join(self.tags)


def generate_form(info, form):
    form = u",".join(form)
    for word, features in info.Generate():
        if form in features:
            return word
    return Failure(u"noform", u"noform" + info.Lemma)


def pluralize(info, form_feature):
    return generate_form(info, [
        "pl" if f == "sg" else f for f in form_feature if f not in ["m", "f"]
    ])


def conditioned_pluralize(info, form, form_feature, context):
    if u"bad" in context or ("you" in context and "not_you" in context):
        return Failure(u"badcontext")
    if u"you" in context:
        if info.Lemma in SKIP_LEMMAS:
            return form
        return pluralize(info, form_feature)
    if u"not_you" in context:
        return form
    return Failure(u"dependency")


def change_form(info, form, form_feature, context=[]):
    # print >> sys.stderr, form, form_feature, info.LexicalFeature, info.Weight
    if "V" in info.LexicalFeature:
        if "ger" in form_feature:
            return form
        if "partcp" in form_feature:
            if "brev" in form_feature and "sg" in form_feature and "n" not in form_feature:
                if "praes" in form_feature:
                    return None
                return conditioned_pluralize(info, form, form_feature, context)
            return form
        if "sg" in form_feature:
            if 'praet' in form_feature and "n" not in form_feature:
                return conditioned_pluralize(info, form, form_feature, context)
            if '2p' in form_feature:
                return pluralize(info, form_feature)
            return form
    elif "A" in info.LexicalFeature and "sg" in form_feature and "n" not in form_feature:
        if "brev" in form_feature and "sg" in form_feature and "n" not in form_feature:
            return conditioned_pluralize(info, form, form_feature, context)
        return form
    elif "APRO" in info.LexicalFeature or "SPRO" in info.LexicalFeature:
        if info.Lemma == u"ты":
            return generate_form(YOU, form_feature)
        elif info.Lemma == u"твой":
            return generate_form(YOURS, form_feature)
        elif info.Lemma == u"сам":
            if "nom" in form_feature and "sg" in form_feature and "n" not in form_feature:
                return conditioned_pluralize(info, form, form_feature, context)
    return form


def convert_word(word, context=[]):
    infos = AnalyzeWord(word)
    answer = None
    for info in infos:
        if info.Weight < FREQUENCY_THRESHOLD:
            continue
        for form_feature in info.FormFeature:
            variant = change_form(info, word, form_feature, context)
            if isinstance(variant, Failure):
                return variant
            if variant is None:
                continue
            if answer is not None and answer != variant:
                return Failure(u"multform", u"multform" + word)
            answer = variant
    return word if answer is None else answer


def in_all_forms(info, feature):
    for form_feature in info.FormFeature:
        if feature not in form_feature:
            return False
    return True


def check_context(word):
    infos = AnalyzeWord(word)
    context = None
    for info in infos:
        current_context = tuple()
        if info.Weight < FREQUENCY_THRESHOLD:
            continue
        if "S" in info.LexicalFeature or "SPRO" in info.LexicalFeature:
            if in_all_forms(info, "nom"):
                current_context = ("you",) if info.Lemma == u"ты" else ("not_you",)
        elif in_all_forms(info, "2p"):
            current_context =  ("you",)
        elif in_all_forms(info, "1p") or in_all_forms(info, "3p"):
            current_context =  ("not_you",)
        if context is not None and context != current_context:
            return ["bad"]
        context = current_context
    return list(context) if context is not None else []


def convert_sentence(model, text):
    words = tokenize(text)
    failure = Failure()
    flag = False
    context = None
    for i in xrange(len(words)):
        if context is None:
            context = set()
            for j in range(i, len(words)):
                if not u"А" <= words[j][0] <= u"я":
                    break
                context.update(check_context(words[j]))
                # print >> sys.stderr, context
        if u"А" <= words[i][0] <= u"я":
            if words[i].lower() == u"тебе":
                if len(words) == 1:
                    new_word = u"вам"
                else:
                    new_word, proba = model.predict(words, i)
                    if proba < PROBA_THRESHOLD:
                        new_word = Failure("tebe")
            else:
                new_word = convert_word(words[i], context)
            if isinstance(new_word, Failure):
                flag = True
                failure.add(new_word)
            elif new_word != words[i]:
                flag = True
                words[i] = new_word
        else:
            context = None
    if not failure.tags and flag:
        text = " ".join(words)
    # print >> sys.stderr, text, failure.format()
    return text, not failure.tags, flag, failure


class RespectReplacer(BaseReplacer):
    def __init__(self, args):
        self.model_file = args.respect_catboost_model
        self.banlist_file = args.respect_banlist
        self.model = None
        self.set_not_you = args.respect_set_not_you

    def start(self, local=False):
        self.model = RespectClassifierModel()
        if not local:
            self.model_file = os.path.basename(self.model_file)
            self.banlist_file = os.path.basename(self.banlist_file)
        self.model.load_model(self.model_file)
        self.banlist_regexp = None
        if self.banlist_file:
            with codecs.open(self.banlist_file, encoding="utf-8") as f:
                self.banlist_regexp = re.compile(u"|".join([line.strip() for line in f.readlines()]))


    def get_yt_extra_args(self):
        return dict(local_files=[self.model_file, self.banlist_file])

    def process(self, reply):
        if self.banlist_regexp is not None and self.banlist_regexp.match(reply.strip()):
            return reply, True, False, Failure()
        return convert_sentence(self.model, reply)

    def process_row(self, row):
        rewritten_reply, success, needs, _skip = self.process(row['rewritten_reply'])
        row['disrespect_reply'] = row['rewritten_reply'] if row['rewritten_reply'] != rewritten_reply else ""
        row['rewritten_reply'] = rewritten_reply
        if self.set_not_you and "you" in row and success and needs:
            row["you"] = "NO"
        row["respect_failed"] = int(not success)
        return row

    @staticmethod
    def register_args(parser):
        parser.add_argument('--respect-catboost-model', required=True)
        parser.add_argument('--respect-banlist', required=False)
        parser.add_argument('--respect-set-not-you', default=False, action="store_true")
