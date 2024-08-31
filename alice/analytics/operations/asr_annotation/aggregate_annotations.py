# -*-coding: utf8 -*-
#!/usr/bin/env python

import argparse
import json
import re
import logging
import sys
import nirvana.job_context as nv
from collections import defaultdict, Counter
from pylev import wfi_levenshtein
from transliterate import translit

reload(sys)
sys.setdefaultencoding('utf-8')

TAGS = {
    "?": "<SPN>",  # spoken noise
    ";": "<EOS>"   # end of sentence
}

SPOTTER_WORDS = {u"алиса", u"алис", u"яндекс", u"дальше", u"назад", u"ниже", u"выше", u"отмена", u"поехали", u"yandex", u"alisa", u"alice"}

MISC_PHONEMES = {" ", "SIL1", "SIL2", "SIL3", "SIL4", ".", "schwa"}

DI_PHONEMES = {"zh", "sh", "ch"}

TRANS_LANGS = {
    "ru": "ru",
    "tr": "l1"  # latin1
}

TEXT_NODES = {"text", "full_text"}

ctx = nv.context()
parameters = json.loads(ctx.get_parameters().get('kwargs', '{}'))
inputs = ctx.get_inputs()
outputs = ctx.get_outputs()


def setup_logger():
    root_logger = logging.getLogger('')
    file_logger = logging.StreamHandler()
    file_logger.setFormatter(logging.Formatter('%(asctime)s %(levelname)-8s %(filename)s:%(lineno)d %(message)s'))
    root_logger.addHandler(file_logger)
    root_logger.setLevel(logging.DEBUG)


def write_result(name, result):
    logging.debug("{} results: {}".format(name, len(result)))
    with open(outputs.get(name), 'w') as f:
        json.dump(result, f, indent=4, ensure_ascii=False)


class Processor(object):
    def __init__(self, assignments, phonemes, parameters):
        self.assignments = assignments
        self.phonemes = {}
        for item in phonemes:
            self.phonemes['@'.join([item['hyp_1'], item['hyp_2']])] = (item['hyp_1_phonemes'], item['hyp_2_phonemes'])

        self.agg_full_text = parameters.get('skip_full_text') == 'false'  # whether to run full_text aggregation, default: skip
        self.threshold = float(parameters.get('threshold'))
        logging.debug("Threshold: {}".format(str(self.threshold)))
        self.lang = parameters.get('lang')  # input language (ru|tr)
        self.platform = parameters.get('platform')  # name of the platform, e.g. \"toloka\"/\"yang\"
        self.is_spotter = parameters.get('is_spotter') == 'true'

        self.groups = {}
        self.clusters = {}  # {"mds_key": {"clusters": [{'hyp_1': 'cnt_1', 'hyp_2': 'cnt_2'}, {}, ...]}}

        self.good = []
        self.confusing_empty = []
        self.recheck = []

    @staticmethod
    def make_pairs(text_list):
        pairs = []
        for i in text_list:
            for j in text_list:
                pairs.append((i, j))
        return pairs

    @staticmethod
    def filter_phonemes(phonemes):
        filtered = []
        for phoneme in phonemes:
            if phoneme not in MISC_PHONEMES:
                phoneme = phoneme if phoneme in DI_PHONEMES else phoneme[0]
                filtered.append(phoneme)
        return ''.join(filtered)

    def get_phonemes(self, text1, text2):
        # phoneme representation for text2 depends on text1
        text1_stripped, text_2_stripped = text1.strip('?; '), text2.strip('?; ')
        try:
            phonemes_list_1, phonemes_list_2 = self.phonemes.get('@'.join([text1, text2])) or \
                                               self.phonemes.get('@'.join([text1_stripped, text2_stripped])) or \
                                               self.phonemes.get('@'.join([text1_stripped, text2])) or \
                                               self.phonemes.get('@'.join([text1, text2_stripped]))
        except:
            phonemes_list_1, phonemes_list_2 = [], []
            if text1 and text2:
                logging.debug("No phonemes for pair: {}@{}".format(text1, text2))
        return self.filter_phonemes(phonemes_list_1) or text1, self.filter_phonemes(phonemes_list_2) or text2

    @staticmethod
    def get_spotter_counts(text):
        return Counter([w for w in text.split() if w in SPOTTER_WORDS])

    @staticmethod
    def compare_normalized(text1, text2):
        return u"".join(text1.split()) == u"".join(text2.split())

    def compare_by_per(self, text1, text2):
        trans_lang = TRANS_LANGS[self.lang]
        equal_transliterated = translit(text1, trans_lang, reversed=True) == translit(text2, trans_lang, reversed=True)
        equal_spotter = (self.get_spotter_counts(text1) == self.get_spotter_counts(text2)) and self.is_spotter
        both_not_empty = (text1 != "" and text2 != "" and text1 is not None and text2 is not None)
        phonemes = self.get_phonemes(text1, text2)
        distance = wfi_levenshtein(*phonemes)
        is_cluster = both_not_empty and (distance <= self.threshold or equal_transliterated) if not self.is_spotter else equal_spotter
        return is_cluster

    def more_useful(self, text1, text2):
        # text1 is more useful than text2
        less_spns = text1.count('?') <= text2.count('?')
        more_tags = text1.count(';') >= text2.count(';')
        not_empty = text1 != "" and text1 is not None
        more_spaces = text1.count(' ') >= text2.count(' ')
        more_latin = len(re.findall(re.compile("[a-z]"), text1)) >= len(re.findall(re.compile("[a-z]"), text2))  # add normalization!
        more_spotter = sum(self.get_spotter_counts(text1).values()) >= sum(self.get_spotter_counts(text2).values())
        more_useful = less_spns and more_tags and not_empty and more_spaces and more_latin if not self.is_spotter else more_spotter
        return more_useful

    def update_votes(self, text, votes):
        if text.count('?') > 0 and not self.is_spotter:
            votes = votes/2.0
        return votes

    def get_main_text(self, cluster):
        main_text = ''
        main_text_votes = 0.0
        cluster_votes = 0.0
        for text, votes in cluster.items():
            votes = self.update_votes(text, votes)
            cluster_votes += votes
            if votes > main_text_votes or (votes == main_text_votes and self.more_useful(text, main_text)):
                main_text = text
                main_text_votes = votes
        return main_text, main_text_votes, cluster_votes

    def get_main_cluster(self, clusters):
        main_cluster = {}
        main_text = ''
        main_cluster_votes = 0.0
        main_text_votes = 0.0
        for cluster in clusters:
            text, votes, cluster_votes = self.get_main_text(cluster)
            if cluster_votes > main_cluster_votes or (cluster_votes == main_cluster_votes and self.more_useful(text, main_text)):
                main_cluster = cluster
                main_cluster_votes = cluster_votes
                main_text = text
                main_text_votes = votes
        return main_cluster, main_text, main_cluster_votes, main_text_votes

    @staticmethod
    def prettify(text):
        if text:
            for tag_before, tag_after in TAGS.items():
                text = text.replace(tag_before, tag_after)
        return text

    def group(self):
        for item in self.assignments:
            url = item['inputValues']['audio']
            annotation = {}
            if url not in self.groups:
                self.groups[url] = {}
                self.groups[url]['raw_assesments'] = []
                self.groups[url]['task_id'] = None
            raw_assesment = item['outputValues'].copy()
            raw_assesment['submitTs'] = item['submitTs']
            raw_assesment['workerId'] = item['workerId']
            raw_assesment['platform'] = self.platform
            self.groups[url]['raw_assesments'].append(raw_assesment)
            self.groups[url]['task_id'] = item['taskId']

            annotation["text"] = item['outputValues'].get('query') or ""
            annotation["full_text"] = item['outputValues'].get('annotation') or ""

            for text_node in TEXT_NODES:
                annotation[text_node] = annotation[text_node].lower().replace(u'ё', u'е')
                if text_node not in self.groups[url]:
                    self.groups[url][text_node] = {}
                if annotation[text_node] not in self.groups[url][text_node]:
                    self.groups[url][text_node][annotation[text_node]] = []
                self.groups[url][text_node][annotation[text_node]].append(item['workerId'])

    def clusterize(self, text_node, check_if_cluster):
        for url in self.groups:
            texts = self.groups[url][text_node]
            pairs = self.make_pairs(texts.keys())
            if url not in self.clusters:
                    self.clusters[url] = {}
            for hyp_1, hyp_2 in pairs:
                cnt_1 = len(texts[hyp_1])
                cnt_2 = len(texts[hyp_2])
                clusters = self.clusters[url].get(text_node, [])
                hyp_1_cluster, hyp_2_cluster = None, None  # list index if hyp already in clusters
                for i, cluster in enumerate(clusters):
                    if hyp_1 in cluster:
                        hyp_1_cluster = i
                    if hyp_2 in cluster:
                        hyp_2_cluster = i
                is_cluster = True if hyp_1 == hyp_2 else check_if_cluster(hyp_1, hyp_2)
                if is_cluster:
                    if hyp_1_cluster is not None and hyp_2_cluster is None:
                        clusters[hyp_1_cluster][hyp_2] = cnt_2
                    elif hyp_2_cluster is not None and hyp_1_cluster is None:
                        clusters[hyp_2_cluster][hyp_1] = cnt_1
                    elif hyp_1_cluster is None and hyp_2_cluster is None:
                        cluster = {hyp_1: cnt_1, hyp_2: cnt_2}
                        clusters.append(cluster)
                    elif hyp_1_cluster != hyp_2_cluster:
                        clusters[hyp_1_cluster].update(clusters[hyp_2_cluster])
                        del clusters[hyp_2_cluster]
                else:
                    if hyp_1_cluster is None:
                        clusters.append({hyp_1: cnt_1})
                    if hyp_2_cluster is None:
                        clusters.append({hyp_2: cnt_2})
                self.clusters[url][text_node] = clusters

    def aggregate(self):
        for url in self.groups:
            clusters = self.clusters[url]
            if not clusters:
                logging.debug("No clusters for url: {}".format(url))

            record = {
                "url": url,
                "raw_assesments": self.groups[url]["raw_assesments"],
                "mds_key": '/'.join(url.split('?')[0].split('/')[-2:]),
                "model_md5": "majority_vote",
                "id": self.groups[url]["task_id"],
                "overlap": len(self.groups[url]["raw_assesments"]),
            }

            for text_node in TEXT_NODES:
                main_cluster, main_text, main_cluster_votes, main_text_votes = self.get_main_cluster(clusters.get(text_node, []))
                record[text_node] = self.prettify(main_text)
                record["cluster_" + text_node] = main_cluster.keys()
                record["clusters_" + text_node] = clusters.get(text_node, [])
                record["votes_" + text_node] = main_text_votes
                record["votes_cluster_" + text_node] = main_cluster_votes

            if record["votes_text"] >= 2 or record["votes_cluster_text"] >= 2:
                if record["text"] != "" and TAGS["?"] not in record["text"]:
                    self.good.append(record)
                else:
                    self.confusing_empty.append(record)
            else:
                self.recheck.append(record)

    def run(self):
        self.group()

        if self.is_spotter:
            logging.debug("Running spotter-specific aggregation...")

        if self.lang == "tr":
            self.clusterize('text', self.compare_normalized)
        else:
            self.clusterize('text', self.compare_by_per)

        if self.agg_full_text:
            logging.debug("Running full_text clusterization...")
            self.clusterize('full_text', self.compare_by_per)

        self.aggregate()


def main():
    setup_logger()

    with open(inputs.get("input1"), "r") as f:
        assignments = json.load(f)

    if inputs.has("input2"):
        with open(inputs.get("input2"), "r") as f:
            phonemes = json.load(f)
    else:
        phonemes = []

    if inputs.has("input3"):
        with open(inputs.get("input3"), "r") as f:
            skills = json.load(f)
    else:
        skills = []

    logging.debug("loaded {} assignments".format(len(assignments)))
    logging.debug("loaded {} unique texts".format(len(phonemes)))

    processor = Processor(assignments, phonemes, parameters)
    processor.run()

    write_result('output1', processor.good)  # good
    write_result('output2', processor.confusing_empty)  # results with confusing words and empty results
    write_result('output3', processor.recheck)  # tasks for recheck


if __name__ == '__main__':
    main()
