import base64
import json
from abc import ABC, abstractmethod

from search.tunneller.libs.protocol_decl.scraper_over_yt_info_pb2 import TScraperOverYtMessage


class Checker(ABC):
    @abstractmethod
    def process_row(self, row):
        pass

    @abstractmethod
    def dump_result(self):
        pass


class TunnellerChecker(Checker):
    NAME = 'tunneller_checker'
    DESCRIPTION = 'checks emptiness, base64 correctness of tunneller raw responses'
    MAX_DIFF = 0.02

    class Counters:
        def __init__(self):
            self.responses = 0
            self.tunneller_responses = 0
            self.good_tunneller_responses = 0
            self.empty_tunneller_responses = 0
            self.undecodable_tunneller_responses = 0

        def __add__(self, other):
            counters = TunnellerChecker.Counters()
            counters.responses = self.responses + other.responses
            counters.tunneller_responses = self.tunneller_responses + other.tunneller_responses
            counters.good_tunneller_responses = self.good_tunneller_responses + other.good_tunneller_responses
            counters.empty_tunneller_responses = self.empty_tunneller_responses + other.empty_tunneller_responses
            counters.undecodable_tunneller_responses = self.undecodable_tunneller_responses + \
                                                       other.undecodable_tunneller_responses
            return counters

        def __eq__(self, other):
            eq = True
            eq = eq and self.responses == other.responses
            eq = eq and self.tunneller_responses == other.tunneller_responses
            eq = eq and self.good_tunneller_responses == other.good_tunneller_responses
            eq = eq and self.empty_tunneller_responses == other.empty_tunneller_responses
            eq = eq and self.undecodable_tunneller_responses == other.undecodable_tunneller_responses
            return eq

        def __ne__(self, other):
            return not self.__eq__(other)

    def __init__(self):
        self._test_counters = self.Counters()
        self._stable_counters = self.Counters()

    @staticmethod
    def _get_all_tunneller_responses(tunneller_raw_responses_dict):
        tunneller_raw_responses = []
        if not isinstance(tunneller_raw_responses_dict, dict):
            return tunneller_raw_responses
        for _, responses in tunneller_raw_responses_dict.items():
            responses = responses.get('responses', [])
            if responses:
                tunneller_raw_responses.extend(responses)
        return tunneller_raw_responses

    @staticmethod
    def _check_base64(s):
        try:
            message = TScraperOverYtMessage()
            message.ParseFromString(base64.b64decode(s))
            return True
        except Exception:
            return False

    def _calc_counters(self, vins_response):
        counters = self.Counters()
        counters.responses += 1
        tunneller_raw_responses_dict = vins_response.get('directive', {}).get('payload', {}).get(
            'megamind_analytics_info', {}).get('tunneller_raw_responses', {})
        tunneller_raw_responses = TunnellerChecker._get_all_tunneller_responses(tunneller_raw_responses_dict)
        for tunneller_raw_response in tunneller_raw_responses:
            counters.tunneller_responses += 1
            if tunneller_raw_response == '':
                counters.empty_tunneller_responses += 1
            elif not TunnellerChecker._check_base64(tunneller_raw_response):
                counters.undecodable_tunneller_responses += 1
            else:
                counters.good_tunneller_responses += 1
        return counters

    def process_row(self, row):
        verdict = True
        message = None
        if row.get('test.VinsResponse') and row.get('stable.VinsResponse'):
            row_test_counters = self._calc_counters(json.loads(row['test.VinsResponse']))
            self._test_counters += row_test_counters
            row_stable_counters = self._calc_counters(json.loads(row['stable.VinsResponse']))
            self._stable_counters += row_stable_counters
            if row_test_counters != row_stable_counters:
                verdict = False
                if row_test_counters.tunneller_responses != row_stable_counters.tunneller_responses:
                    message = ('amount of tunneller responses ({}) in test response not equals to'
                               ' amount ({}) in stable response'.format(row_test_counters.tunneller_responses,
                                                                        row_stable_counters.tunneller_responses))
                elif row_test_counters.good_tunneller_responses != row_stable_counters.good_tunneller_responses:
                    message = ('amount of good tunneller responses ({}) in test response not equals to amount'
                               ' ({}) in stable response'.format(row_test_counters.good_tunneller_responses,
                                                                 row_stable_counters.good_tunneller_responses))
        elif row.get('test.VinsResponse'):
            self._test_counters += self._calc_counters(json.loads(row['test.VinsResponse']))
            message = 'vins response from stable downloading is empty'
        elif row.get('stable.VinsResponse'):
            self._stable_counters += self._calc_counters(json.loads(row['stable.VinsResponse']))
            message = 'vins response from test downloading is empty'
        return verdict, message

    @staticmethod
    def _calc_diff(test_val, stable_val):
        return (test_val - stable_val) / stable_val

    @staticmethod
    def _calc_mean_tunneller_responses(counters):
        return counters.tunneller_responses / counters.responses

    @staticmethod
    def _check_count_tunneller(test_counters, stable_counters):
        mean_test_tunneller_responses = TunnellerChecker._calc_mean_tunneller_responses(test_counters)
        mean_stable_tunneller_responses = TunnellerChecker._calc_mean_tunneller_responses(stable_counters)
        return abs(TunnellerChecker._calc_diff(mean_test_tunneller_responses,
                                               mean_stable_tunneller_responses)) < TunnellerChecker.MAX_DIFF

    def dump_result(self):
        verdict = True
        message = ''
        for counters, name in ((self._test_counters, 'test'), (self._stable_counters, 'stable')):
            message += 'stats of {} downloading:\n'.format(name)
            message += 'vins responses: {}\n'.format(counters.responses)
            if counters.responses > 0:
                message += 'tunneller responses: {}\n'.format(counters.tunneller_responses)
                message += 'empty tunneller responses: {}\n'.format(counters.empty_tunneller_responses)
                message += 'undecodable tunneller responses: {}\n'.format(counters.undecodable_tunneller_responses)
                message += 'good tunneller responses: {}\n'.format(counters.good_tunneller_responses)
                message += 'mean tunneller responses {:.2f}\n'.format(self._calc_mean_tunneller_responses(counters))
            message += '\n'

        if (self._test_counters.tunneller_responses == self._test_counters.good_tunneller_responses) and (
                self._test_counters.responses > 0) and (self._stable_counters.responses > 0):
            if self._check_count_tunneller(self._test_counters, self._stable_counters):
                message += 'test downloading is ok'
                if self._test_counters.tunneller_responses != self._test_counters.good_tunneller_responses:
                    message += ' but stable downloading has bad tunneller responses'
            else:
                verdict = False
                message += ('mean of tunneller responses of test downloading ({:.2f})'
                            ' differs more than {:.2f} comparing to stable downloading ({:.2f}) by abs value').format(
                    self._calc_mean_tunneller_responses(self._test_counters), self.MAX_DIFF,
                    self._calc_mean_tunneller_responses(self._stable_counters))
            message += '\n'
        if self._test_counters.empty_tunneller_responses > 0:
            verdict = False
            message += 'amount empty tunneller responses of test downloading is not 0'
            if self._stable_counters.empty_tunneller_responses > 0:
                message += ' but stable downloading has empty responses too'
            message += '\n'
        if self._test_counters.undecodable_tunneller_responses > 0:
            verdict = False
            message += 'amount undecodable tunneller responses of test downloading is not 0'
            if self._stable_counters.undecodable_tunneller_responses > 0:
                message += ' but stable downloading has undecodable responses too'
            message += '\n'

        return {
            'name': self.NAME,
            'description': self.DESCRIPTION,
            'verdict': verdict,
            'message': message
        }


class MegamindAnalyticsInfoChecker(Checker):
    NAME = 'megamind_analytics_info_checker'
    DESCRIPTION = 'checks presence of megamind_analytics_info and original_utterance'

    class Counters:
        def __init__(self):
            self.responses = 0
            self.megamind_analytics_info = 0
            self.original_utterance = 0

        def __add__(self, other):
            counters = MegamindAnalyticsInfoChecker.Counters()
            counters.responses = self.responses + other.responses
            counters.megamind_analytics_info = self.megamind_analytics_info + other.megamind_analytics_info
            counters.original_utterance = self.original_utterance + other.original_utterance
            return counters

        def __eq__(self, other):
            eq = True
            eq = eq and self.responses == other.responses
            eq = eq and self.megamind_analytics_info == other.megamind_analytics_info
            eq = eq and self.original_utterance == other.original_utterance
            return eq

        def __ne__(self, other):
            return not self.__eq__(other)

    def __init__(self):
        self._test_counters = self.Counters()
        self._stable_counters = self.Counters()

    def _calc_counters(self, vins_response):
        counters = self.Counters()
        counters.responses += 1
        megamind_analytics_info_dict = vins_response.get('directive', {}).get('payload', {}).get(
            'megamind_analytics_info')
        if megamind_analytics_info_dict is not None:
            counters.megamind_analytics_info += 1
            if 'original_utterance' in megamind_analytics_info_dict:
                counters.original_utterance += 1
        return counters

    def process_row(self, row):
        verdict = True
        message = None
        if row.get('test.VinsResponse') and row.get('stable.VinsResponse'):
            row_test_counters = self._calc_counters(json.loads(row['test.VinsResponse']))
            self._test_counters += row_test_counters
            row_stable_counters = self._calc_counters(json.loads(row['stable.VinsResponse']))
            self._stable_counters += row_stable_counters
            if row_test_counters != row_stable_counters:
                verdict = False
                if row_test_counters.megamind_analytics_info != row_stable_counters.megamind_analytics_info:
                    present, absent = 'test', 'stable'
                    if row_test_counters.megamind_analytics_info == 0:
                        present, absent = absent, present
                    message = ('megamind analytics info is present in {} response but absent in {}'.format(present,
                                                                                                           absent))
                elif row_test_counters.original_utterance != row_stable_counters.original_utterance:
                    present, absent = 'test', 'stable'
                    if row_test_counters.original_utterance == 0:
                        present, absent = absent, present
                    message = ('original utterance is present in {} response but absent in {}'.format(present,
                                                                                                      absent))
        elif row.get('test.VinsResponse'):
            self._test_counters += self._calc_counters(json.loads(row['test.VinsResponse']))
            message = 'vins response from stable downloading is empty'
        elif row.get('stable.VinsResponse'):
            self._stable_counters += self._calc_counters(json.loads(row['stable.VinsResponse']))
            message = 'vins response from test downloading is empty'
        return verdict, message

    def dump_result(self):
        verdict = True
        message = ''
        for counters, name in ((self._test_counters, 'test'), (self._stable_counters, 'stable')):
            message += 'stats of {} downloading:\n'.format(name)
            message += 'vins responses: {}\n'.format(counters.responses)
            message += 'megamind analytics info: {}\n'.format(counters.megamind_analytics_info)
            message += 'original utterance: {}\n'.format(counters.original_utterance)
            message += '\n'

        if (self._test_counters.responses == self._test_counters.megamind_analytics_info) and (
                self._test_counters.responses == self._test_counters.original_utterance):
            message += 'test downloading is ok'
            if (self._stable_counters.responses != self._stable_counters.megamind_analytics_info) or (
                    self._stable_counters.responses != self._stable_counters.original_utterance):
                message += (' but some stable downloading vins responses has no megamind analytics info'
                            ' or original utterance')
            message += '\n'
        if self._test_counters.responses != self._test_counters.megamind_analytics_info:
            verdict = False
            message += 'megamind analytics info is not present in every response of test downloading'
            message += '\n'

        if self._test_counters.responses != self._test_counters.original_utterance:
            verdict = False
            message += 'original utterance is not present in megamind analytics info of test downloading'
            message += '\n'

        return {
            'name': self.NAME,
            'description': self.DESCRIPTION,
            'verdict': verdict,
            'message': message
        }


class MetaChecker(Checker):
    NAME = 'meta_checker'
    DESCRIPTION = 'checks non emptiness of meta'

    class Counters:
        def __init__(self):
            self.responses = 0
            self.not_empty_metas = 0

        def __add__(self, other):
            counters = MetaChecker.Counters()
            counters.responses = self.responses + other.responses
            counters.not_empty_metas = self.not_empty_metas + other.not_empty_metas
            return counters

        def __eq__(self, other):
            eq = True
            eq = eq and self.responses == other.responses
            eq = eq and self.not_empty_metas == other.not_empty_metas
            return eq

        def __ne__(self, other):
            return not self.__eq__(other)

    def __init__(self):
        self._test_counters = self.Counters()
        self._stable_counters = self.Counters()

    def _calc_counters(self, vins_response):
        counters = self.Counters()
        counters.responses += 1
        meta = vins_response.get('directive', {}).get('payload', {}).get('response', {}).get('meta', [])
        counters.not_empty_metas += int(len(meta) > 0)
        return counters

    def process_row(self, row):
        verdict = True
        message = None
        if row.get('test.VinsResponse') and row.get('stable.VinsResponse'):
            row_test_counters = self._calc_counters(json.loads(row['test.VinsResponse']))
            self._test_counters += row_test_counters
            row_stable_counters = self._calc_counters(json.loads(row['stable.VinsResponse']))
            self._stable_counters += row_stable_counters
            if row_test_counters != row_stable_counters:
                verdict = False
                if row_test_counters.not_empty_metas != row_stable_counters.not_empty_metas:
                    present, absent = 'test', 'stable'
                    if row_test_counters.not_empty_metas == 0:
                        present, absent = absent, present
                    message = ('meta is present in {} response but absent in {}'.format(present,
                                                                                        absent))
        elif row.get('test.VinsResponse'):
            self._test_counters += self._calc_counters(json.loads(row['test.VinsResponse']))
            message = 'vins response from stable downloading is empty'
        elif row.get('stable.VinsResponse'):
            self._stable_counters += self._calc_counters(json.loads(row['stable.VinsResponse']))
            message = 'vins response from test downloading is empty'
        return verdict, message

    def dump_result(self):
        verdict = True
        message = ''
        for counters, name in ((self._test_counters, 'test'), (self._stable_counters, 'stable')):
            message += 'stats of {} downloading\n'.format(name)
            message += 'vins responses: {}\n'.format(counters.responses)
            message += 'not empty metas: {}\n'.format(counters.not_empty_metas)
            message += '\n'

        if self._test_counters.responses == self._test_counters.not_empty_metas:
            message += 'test downloading is ok'
            if self._stable_counters.responses != self._stable_counters.not_empty_metas:
                message += ' but some stable downloading vins responses has empty meta'
            message += '\n'
        else:
            verdict = False
            message += 'meta is not present in every response of test downloading'
            message += '\n'

        return {
            'name': self.NAME,
            'description': self.DESCRIPTION,
            'verdict': verdict,
            'message': message
        }
