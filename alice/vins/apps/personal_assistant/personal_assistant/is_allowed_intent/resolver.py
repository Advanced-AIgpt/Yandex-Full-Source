import logging

from collections import namedtuple, defaultdict

logger = logging.getLogger(__name__)

Predicate = namedtuple('Predicate', ['name', 'inverse'])


class Resolver(object):
    def __init__(self, precalc_parameter, precalc_values, predicates, config):
        self._precalc_parameter = precalc_parameter
        self._predicates = predicates
        self._precalc_parameter_to_predicates = defaultdict(list)
        self._structure = []
        for precalc_predicate_name, predicate_names in config['structure'].iteritems():
            precalc_predicate = Resolver._generate_predicate(precalc_predicate_name)
            predicates = [Resolver._generate_predicate(predicate_name) for predicate_name in predicate_names]
            self._structure.append((precalc_predicate, predicates))

            for precalc_value in precalc_values:
                if self._evalute_predicate(precalc_predicate, {precalc_parameter: precalc_value}):
                    self._precalc_parameter_to_predicates[precalc_value].extend(predicates)

    def _evalute_predicate(self, predicate, arguments):
        return bool(self._predicates[predicate.name]['func'](
            **{arg: arguments[arg] for arg in self._predicates[predicate.name]['params']})) ^ predicate.inverse

    @staticmethod
    def _generate_predicate(predicate):
        inverse = False
        name = predicate
        if predicate.startswith('not '):
            inverse = True
            name = predicate[4:]
        return Predicate(name, inverse)

    def _process(self, arguments, predicates):
        for predicate in predicates:
            if self._evalute_predicate(predicate, arguments):
                logger.debug('Intent %s is not allowed by predicate %r', arguments[self._precalc_parameter], predicate)
                return False
        return True

    def process(self, arguments):
        if arguments[self._precalc_parameter] in self._precalc_parameter_to_predicates:
            predicates = self._precalc_parameter_to_predicates[arguments[self._precalc_parameter]]
            return self._process(arguments, predicates)
        else:
            for precalc_predicate, predicates in self._structure:
                if self._evalute_predicate(precalc_predicate, arguments):
                    if not self._process(arguments, predicates):
                        return False
            return True
