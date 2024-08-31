# -*- coding: utf-8 -*-
"""
Module implements different resources to use for ASR analyzer and more.
"""
import collections
import io
import enum
import json
import logging
import os
import subprocess
import sys
import time
import numpy as np


class ResourceType(enum.Enum):
    CLUSTER_REFERENCES = 1
    REFERENCES = 2
    WER_ENGINE = 3
    LANGUAGE_MODEL = 4
    LEXICON = 5
    G2P_POSDICT = 6
    DECODER_RESULT = 7


def string_to_resource_name_enum_value(string):
    """
    Converts string to enum value
    :param string: string to convert
    :return: ResourceType enum value
    """
    if string == 'cluster_references':
        return ResourceType.CLUSTER_REFERENCES
    if string == 'references':
        return ResourceType.REFERENCES
    if string == 'wer_engine':
        return ResourceType.WER_ENGINE
    if string == 'language_model':
        return ResourceType.LANGUAGE_MODEL
    if string == 'lexicon':
        return ResourceType.LEXICON
    if string == 'pos.dict':
        return ResourceType.G2P_POSDICT
    if string == 'decoder_result':
        return ResourceType.DECODER_RESULT

    raise KeyError('Unknown resource name: ' + string)


def enum_value_resource_name_to_string(enum_value):
    """
    Converts enum value to string
    :param enum_value: ResourceName enum value to convert
    :return: string
    """
    if enum_value == ResourceType.CLUSTER_REFERENCES:
        return 'cluster_references'
    if enum_value == ResourceType.REFERENCES:
        return 'references'
    if enum_value == ResourceType.WER_ENGINE:
        return 'wer_engine'
    if enum_value == ResourceType.LANGUAGE_MODEL:
        return 'language_model'
    if enum_value == ResourceType.G2P_POSDICT:
        return 'pos.dict'
    if enum_value == ResourceType.LEXICON:
        return 'lexicon'
    if enum_value == ResourceType.DECODER_RESULT:
        return 'decoder_result'

    raise KeyError('Unknown resource name: {}'.format(enum_value))


def enum_value_to_class(enum_value):
    """
    Gets class by enum value
    :param enum_value: ResourceName enum value
    :return: Resource class
    """
    if enum_value == ResourceType.CLUSTER_REFERENCES:
        return ClusterReference
    if enum_value == ResourceType.REFERENCES:
        return ReferenceResource
    if enum_value == ResourceType.WER_ENGINE:
        return CachingWEREngine
    if enum_value == ResourceType.LANGUAGE_MODEL:
        return CachingLMResource
    if enum_value == ResourceType.DECODER_RESULT:
        return DecoderResult
    if enum_value == ResourceType.G2P_POSDICT:
        return PosDictResource
    if enum_value == ResourceType.LEXICON:
        return LexiconResource

    raise KeyError('Unknown resource name: {}'.format(enum_value))


class ReferenceResource:
    """
    Resource for holding references
    """
    def __init__(self, path):
        """
        Resource that parses references from text file
        :param fn: path to text
        :return: None
        """
        self.data = {}
        # todo support empty references
        self.skipped_data = {}
        self._load(path)

    def _load(self, path_to_file):
        """
        Loads text format (utt_id/tutt_reference) to memory
        :param path_to_file: path to text
        :return: None
        """
        with io.open(path_to_file, 'r', encoding='utf-8') as fin:
            for entry in fin:
                indx, value = list(map(str.strip, entry.split('\t')))
                processed_value = value.lower().strip()

                if not processed_value:
                    self.skipped_data[indx] = processed_value
                    sys.stderr.write('Empty references are not '
                                     'supported, skipping {}\n'.format(indx))
                    continue
                self.data[indx] = processed_value
        if self.skipped_data:
            sys.stderr.write('Total references skipped: '
                             '{}\n'.format(len(self.skipped_data)))

    def __getitem__(self, item):
        if item not in self.data:
            raise KeyError('item {} not found in resource'.format(item))
        return self.data[item]


class BaseG2PResource:
    """
    Base G2P Resource class
    """
    def __init__(self, path):
        """
        Resource that parses either pos.dict or lexicon
        :param fn: path to lexicon/pos.dict
        :return: None
        """
        self.g2p_per_word = collections.defaultdict(set)
        self.g2p_per_tag = {}
        self._load(path)

    def _load(self, path):
        with io.open(path, 'r', encoding='utf-8') as fin:
            for line in fin:
                self._parse_g2p_line(line)

    def _parse_g2p_line(self, line):
        raise NotImplementedError('Base class does not provide implementation')

    def __getitem__(self, word):
        if word not in self.g2p_per_word:
            raise KeyError('item {} not found in g2p resource'.format(word))
        return self.g2p_per_word[word]

    def contains_word(self, word):
        """
        Checks if word exists in dict
        :param word: word to search
        :return: bool
        """
        return word in self.g2p_per_word

    def contains_word_by_tag(self, tag, word):
        """
        Checks if word exists in by tag transcription dict
        :param tag: tag to search
        :param word: word to search in per tag transcriptions
        :return: bool
        """
        return tag in self.g2p_per_tag and word in self.g2p_per_tag[tag]


class LexiconResource(BaseG2PResource):
    """
    Resource for parsing and holding Lexicon
    """

    def _parse_g2p_line(self, line):
        """
        parses lexicon entry according to parsing rules
        :param line: string
        :return: None
        """
        splitted = line.strip().split('\t')
        word = splitted[0]
        transcription = splitted[1]

        self.g2p_per_word[word].update(tuple([transcription]))


class PosDictResource(BaseG2PResource):
    """
    Resource for parsing and holding PosDict
    """
    def _parse_g2p_line(self, line):
        """
        parses pos.dict entry according to parsing rules
        :param line: string
        :return: None
        """
        splitted = line.strip().split('|')
        word = splitted[0]
        transcription = splitted[1]
        if len(splitted) > 2:
            tag_splitted = splitted[2].split()[1:]
            for tag in tag_splitted:
                if tag not in self.g2p_per_tag:
                    self.g2p_per_tag[tag] = collections.defaultdict(set)
                self.g2p_per_tag[tag][word].update(tuple([transcription]))
        self.g2p_per_word[word].update(tuple([transcription]))


class ResourcesPool:
    """
    singleton class to hold all of the resources
    and give the access to them for every statistic
    """
    class InnerResourcesPool:
        """
        Inner Resource Pool
        """
        def __init__(self):
            self.resources_dict = {}

        def add_resource(self, resource_type, *args):
            """
            Adds resource to holder
            :param name: name of resource
            :param resource_type: type of resource
            :param args: args
            :return: None
            """
            if resource_type not in self.resources_dict:
                self.resources_dict[resource_type] = enum_value_to_class(resource_type)(*args)

        def get_resource(self, item):
            """
            Gets a resource from holder
            :param item: resource enum
            :return:
            """
            if item not in self.resources_dict:
                raise KeyError('resource {} not found in Resource Pool'.format(item))

            return self.resources_dict[item]

    # singletone instance
    instance = None

    def __init__(self):
        if not ResourcesPool.instance:
            ResourcesPool.instance = ResourcesPool.InnerResourcesPool()

    def __getattr__(self, name):
        return getattr(self.instance, name)


class CachingLMResource:
    """
    Resource to hold language model and able to score and cache scored results.
    """
    def __init__(self, lm_path):
        """
        Resource that holds lm handle in memory, and can query language model using ngram
        :param lm_path: path to lm
        """
        cmd = ['ngram', '-lm', lm_path, '-ppl', '-', '-debug', '2']
        self.lm_path = lm_path
        self.cache = {}

        # process that does querying
        self.process = subprocess.Popen(cmd,
                                        stdout=subprocess.PIPE,
                                        stdin=subprocess.PIPE,
                                        stderr=subprocess.PIPE)
        self._init()

    def _init(self):
        """
        Initializes the language model
        :return: None
        """
        start_time = time.time()
        # todo: refactor it into using logging
        sys.stderr.write('LM initialization started\n')

        # unfortunately srilm doesn't flush
        # so we need to determine order with this crazy way
        order = -1

        # just read a few line of language model is arpa format and determine the order
        with open(self.lm_path, 'r', encoding='utf-8') as fout:
            for i, line in enumerate(fout):
                stripped_line = line.strip()
                if not stripped_line and i > 0:
                    break
                if i == 0 or stripped_line == '\data\\':
                    continue
                # split string like
                # 'ngram 4=88888'
                order = int(line.split('=')[0].split()[1])
        if order < 0:
            raise ValueError('Cannot read lm, are you sure it is in arpa format?')

        # now, start the initializing process
        for i, line in enumerate(iter(self.process.stderr.readline, b'')):
            if (i + 1) == order:
                break

        # srilm does not flush, so it is not really loaded, until we can read from stdout
        self.score('test phrase')
        sys.stderr.write('LM successfully initialized, '
                         'took {0:4.3f} s\n'.format(time.time() - start_time))

    def _parse_entry(self, entry, sentence):
        """
        Parses srilm result
        :param entry: result of srilm, list of line by line stdout
        :param sentence: scored string
        :return:
        """
        if not sentence:
            return {'overall': {'ppl': np.inf, 'logprob': np.inf, 'oovs': 0, 'n_words': 0},
                    'per_word': [('</s>',
                                  {'ngram_order': 0,
                                   'probability': np.inf,
                                   'logprobability': np.inf,
                                   'real_context': ''
                                   })]
                    }

        contexts = ['<s>'] + sentence.split() + ['</s>']

        # determine per word values
        per_word_result = []
        for i, single_entry in enumerate(entry):
            if single_entry[0] == 'p':
                per_word_result.append((contexts[i + 1],
                                        self._parse_single_probability_entry(single_entry,
                                                                             contexts[:(i + 1)])))
        # calc sum of logprobs, # of oovs and perplexity
        oovs = [i for i, res in enumerate(per_word_result) if res[1]['ngram_order'] == 0]
        oovs_count = len(oovs)
        logprob_sum = np.sum([res[1]['logprobability'] * np.log(10)
                              for i, res in enumerate(per_word_result) if i not in oovs])

        existing_words = len(contexts) - oovs_count - 1
        ppl = 10 ** (-logprob_sum / (len(contexts) - oovs_count - 1))
        overall = {'ppl': ppl, 'logprob': logprob_sum,
                   'oovs': oovs_count, 'n_words': existing_words}
        return {'overall': overall, 'per_word': per_word_result}

    def _parse_single_probability_entry(self, entry, context):
        """
        Prase single srilm entry,
        e.g. "p( включи | <s> ) = [2gram] 0.00921279 [ -2.03561 ]"
        :param entry: string, single srilm entry
        :param context: context, length of lm order - 1
        :return: dict, containing lm statistics
        """

        _, right = map(str.strip, entry.split('='))
        ngram_as_string, prob_as_string, _, logprob_as_string, _ = right.split()

        ngram = int(ngram_as_string.split('gram')[0][1:]) if 'OOV' not in ngram_as_string else 0
        prob = float(prob_as_string)
        logprob = float(logprob_as_string)
        real_context = context[-(ngram - 1):] if ngram > 1 else ['']
        output = {'ngram_order': ngram, 'probability': prob, 'logprobability': logprob,
                  'real_context': ' '.join(real_context)}
        return output

    def score(self, sentence):
        """
        Scores sentence using language model
        :param sentence: string, sentence to score
        :return: srilm result, dict containing both per word information and overall
        """

        # first, look into the cache
        if sentence in self.cache:
            return self.cache[sentence]
        raw_output = []

        if sentence:
            # send it to process
            self.process.stdin.write('{}\n'.format(sentence).encode('utf-8'))
            self.process.stdin.flush()

            # read result
            for i, line in enumerate(iter(self.process.stdout.readline, b'')):
                decoded_line = line.decode('utf-8').strip()
                if i > 0 and decoded_line:
                    raw_output.append(decoded_line)
                elif not decoded_line:
                    break

        value = self._parse_entry(raw_output, sentence)
        self.cache[sentence] = value
        return value


class CachingWEREngine:
    """
    WER Engine, holding special dictionary and able to align utterances
    It has queries cache
    """
    def __init__(self):
        self.resources_singleton = ResourcesPool()
        self.cache = {}

    def __call__(self, hypothesis, reference, cr_resource):
        """
        Calculates WER (# mistakes, #total in reference)
            and aligning info of pair (hypothesis, reference)
            using cr_resource
        :param hypothesis: string
        :param reference: string
        :param cr_resource: CRResource, may be None
        :return: tuple, #mistakes, #total in reference, alignment result
        """
        current_hash = str(hypothesis) + '_' + str(reference) + str(hash(cr_resource))
        if current_hash in self.cache:
            return self.cache[current_hash]

        result = align_hypo_and_ref_wrapper(hypothesis, reference, cr_resource)
        self.cache[current_hash] = result
        return result


class Entry:
    def __init__(self, raw_entry):
        """
        Class represents decoder result per single utterance
        :param raw_entry: dict, utterance result info
        """
        self.data = raw_entry
        if len(self.data['chunks']) > 1:
            logging.warn('Multiple chunks in the entry, note that only one utterance will be analyzed')

    def id(self):
        """
        return id of utterance
        :return: string, id of utterance
        """
        # os.path.splitext is needed,
        # because some of the indices in custom texts often come with ".wav" ending
        return os.path.splitext(self.data['id'])[0]

    def nbest(self):
        """
        return results per nbest
        :return: list of dicts, features per each hypo
        """
        return self.data['final']['variants']

    def iter_nbest(self):
        """
        yields hypotheses strings
        """
        for hypothesis in self.data['final']['variants']:
            yield hypothesis['string']

    def iter_nbest_rescoring_features(self):
        """
        Yields rescoring features for every hypothesis
        """
        for entry in self.data['final']['variants']:
            yield entry.get('rescorer_features', {})

    def _extract_basic_scores_from_hypothesis(self, hyp):
        """
        Extracts some basic score from hypothesis, those must exists in every hypothesis
        :param hyp: dict
        :return: dict, containing named basic scores
        """
        return {'AcousticScore': hyp['acoustic_score'], 'GraphScore': hyp['graph_score'],
                'TotalScore': hyp['acoustic_score'] + hyp['graph_score']}

    def nbest_basic_scores(self):
        """
        return basic scores (AcousticScore, GraphScore, TotalScore) for each sentence
        :return: list of di1cts
        """
        # todo: make ASR dump result by word
        return [self._extract_basic_scores_from_hypothesis(variant) for variant in self.nbest()]

    def __len__(self):
        return len(self.nbest())


class DecoderResult:
    """
    Class that holdes decoder result of ASR
    """
    def __init__(self, resources):
        """
        Class containing ASR result
        :param config: configuration dict
        """
        self.id_to_entry = {}

        # we need decoder_result
        if ResourceType.DECODER_RESULT not in resources:
            raise KeyError('Specify decoder_result in "resources" field')

        self.resources_config = resources
        self.resource_pool = ResourcesPool()
        self.utterances_to_skip = set()
        self._maybe_initialize_references()
        self._parse_asr_result(resources[ResourceType.DECODER_RESULT])

    @staticmethod
    def parse_and_yield(filename):
        with io.open(filename, encoding='utf-8') as fin:
            for line in fin:
                yield Entry(json.loads(line))

    def _maybe_initialize_references(self):
        """
        initialize references, if they are present.
        :return: None
        """
        # todo remove it and support empty references
        if ResourceType.REFERENCES in self.resources_config:
            self.resource_pool.add_resource(ResourceType.REFERENCES,
                                            self.resources_config[ResourceType.REFERENCES])
            for indx in self.resource_pool.get_resource(
                    ResourceType.REFERENCES).skipped_data:
                self.utterances_to_skip.add(indx)

    def _parse_asr_result(self, filename):
        """
        Parses ASR result from --extended_output of yaldi public test
        :param filename:
        :return:
        """
        with io.open(filename, encoding='utf-8') as fin:
            asr_output = [json.loads(s) for s in fin.readlines()]
        for single_entry in asr_output:
            entry = Entry(single_entry)
            indx = entry.id()
            if indx not in self.utterances_to_skip:
                self.id_to_entry[entry.id()] = entry
            else:
                sys.stderr.write('{} has an empty reference, empty references are '
                                 'currently not supported, skipping\n'.format(indx))

    def __iter__(self):
        for _, entry in self.id_to_entry.items():
            yield entry

    def iter_nbest(self, entry_id):
        """
        Yields hypotheses
        :param entry_id: entry id to iterate over
        """
        # todo: get rid of this method, change it to Entry's method
        for entry in self.id_to_entry[entry_id].nbest():
            yield entry['string']

    def nbest_length(self, entry_id):
        """
        Gets length of entry_id th nbest
        :param entry_id: entry id get nbest from
        :return:
        """
        return len(self.id_to_entry[entry_id])

    def get_reference(self, entry_id):
        """
        Returns reference according to entry_id
        :param entry_id: id
        :return: string, reference by entry_id
        """
        if ResourceType.REFERENCES not in self.resources_config:
            raise KeyError('Specify "references" in "resources" field')
        self.resource_pool.add_resource(ResourceType.REFERENCES,
                                        self.resources_config[ResourceType.REFERENCES])
        return self.resource_pool.get_resource(ResourceType.REFERENCES)[entry_id]

    def __getitem__(self, item):
        if item not in self.id_to_entry:
            raise KeyError('{} not found in ReferenceResource'.format(item))
        return self.id_to_entry[item]

    def __len__(self):
        return len(self.id_to_entry)


class ClusterReference:
    """
    Cluster references holder, parses and supports querying
    """
    def __init__(self, cr_file=None, cr_as_string=None):
        """
        Class to hold and apply cluster references
        :param cr_file: file to initialize cr from
        :param cr_as_string: string to initialize cr from
        """
        if cr_file is None and cr_as_string is None:
            return

        if cr_file is not None:
            with open(cr_file, 'r', encoding='utf-8') as fin:
                data = json.load(fin)
        else:
            data = json.loads(cr_as_string)
        self.item_to_centers = collections.defaultdict(list)
        for center, others in data.items():

            # the center of the cluster if the "right" substituion term for the cluster
            # e.g. counter strike is a center of [(counterstrike), (кантер, страйк), ...]
            center = tuple(center.split())
            for value in others:
                self.item_to_centers[tuple(value.split(' '))].append(center)

    def get(self, value, default_value):
        """
        Try to get a center value from cluster references,
        if it doesn't exists, returns default value
        """
        return self.item_to_centers.get(value, default_value)

    def __repr__(self):
        string = ''
        for center, others in self.item_to_centers.items():
            string += '{} --> {}'.format(center, others) + '\n'
        return string
