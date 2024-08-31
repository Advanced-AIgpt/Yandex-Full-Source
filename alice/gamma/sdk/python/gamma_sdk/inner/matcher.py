# coding: utf-8

import bisect
from functools import partial

import attr
import re

from gamma_sdk.inner.parser import parse


class EntityType:
    _MAX_GROUPS_NUMBER = 100

    def __init__(self, values):
        self.values = []
        self.patterns = []
        patterns = []
        for i, (value, examples) in enumerate(values.items()):
            self.values.append(value)
            pattern = r'|'.join(re.escape(example) for example in examples)
            # hack to fix default regex bound on named groups number
            if i > 0 and i % self._MAX_GROUPS_NUMBER == 0:
                self.patterns.append(self._compile_pattern(patterns))
                patterns = []
            patterns.append(r'(?P<{}>{})'.format(value, pattern))
        if patterns:
            self.patterns.append(self._compile_pattern(patterns))

    @staticmethod
    def _compile_pattern(patterns):
        return re.compile(r'\b(?:{})\b'.format('|'.join(patterns)), flags=re.UNICODE)

    def find_values(self, string):
        result = []
        for pattern in self.patterns:
            for match in pattern.finditer(string):
                value, _ = next(filter(lambda x: x[1] is not None, match.groupdict().items()))
                result.append((match.start(), match.end(), value))
        return result


@attr.s
class Entity:
    type = attr.ib(type=str)
    value = attr.ib()
    begin = attr.ib(type=int)
    end = attr.ib(type=int)


_NOT_SPACES = re.compile('[^ ]+', flags=re.UNICODE)


def get_tokens_starts(string):
    return [token.start() for token in _NOT_SPACES.finditer(string)]


class EntityExtractor:
    def __init__(self, entities):
        self.entities = {}
        for entity_class, entity_values in entities.items():
            self.entities[entity_class] = EntityType(entity_values)

    def get_entities(self, string):
        result = {}
        token_starts = get_tokens_starts(string)
        for entity_class, entity in self.entities.items():
            per_class_result = []
            values = entity.find_values(string)
            if values:
                result_values = []
                for begin, end, value in values:
                    begin = bisect.bisect_left(token_starts, begin)
                    end = bisect.bisect_left(token_starts, end)
                    result_values.append(Entity(type=entity_class, begin=begin, end=end, value=value))
                per_class_result.extend(result_values)
            if per_class_result:
                result[entity_class] = per_class_result
        return result

    def get_entities_as_list(self, string):
        result = []
        token_starts = get_tokens_starts(string)
        for entity_class, entity in self.entities.items():
            values = entity.find_values(string)
            if values:
                for begin, end, value in values:
                    begin = bisect.bisect_left(token_starts, begin)
                    end = bisect.bisect_left(token_starts, end)
                    result.append(Entity(type=entity_class, begin=begin, end=end, value=value))
        return result


def external_entities(entities):
    result = {}
    for entity in entities:
        _type = entity['type']
        if _type not in result:
            result[_type] = []
        result[_type].append(Entity(
            type=_type,
            value=entity['value'],
            begin=entity['tokens']['start'],
            end=entity['tokens']['end'],
        ))
    return result


@attr.s(frozen=True)
class Template:
    pattern = attr.ib(converter=partial(re.compile, flags=re.UNICODE))
    variables = attr.ib(type=list, factory=list)

    def match(self, string):
        return self.pattern.match(string)


def _find_in_entities(begin, end, entities):
    for entity in entities:
        if begin == entity.begin and end == entity.end:
            return entity
    return None


@attr.s(frozen=True)
class Variable:
    type = attr.ib(type=str)
    value = attr.ib()


class SimpleMatcher:
    def __init__(self, templates):
        self.templates = {name: self._template(template) for name, template in templates.items()}

    @staticmethod
    def _template(pattern):
        parsed_pattern = parse(pattern)
        template = Template(pattern=parsed_pattern.to_regex(), variables=parsed_pattern.variables)
        return template

    def match(self, string, entities=None):
        # todo: check it on gamma?
        string = string.lower()
        token_starts = get_tokens_starts(string)
        entities = entities or {}
        for name, template in self.templates.items():
            match = template.match(string)
            if match:
                skip = False
                variables = {}
                for variable in template.variables:
                    if not match.group(variable.name):
                        continue
                    begin = bisect.bisect_left(token_starts, match.start(variable.name))
                    end = bisect.bisect_left(token_starts, match.end(variable.name))
                    entity = _find_in_entities(begin, end, entities.get(variable.type, []))
                    if entity:
                        if variable.value is not None and variable.value != entity.value:
                            skip = True
                        else:
                            variables[variable.name] = Variable(value=entity.value, type=entity.type)
                    else:
                        skip = True
                if not skip:
                    yield name, variables
        yield None, {}
