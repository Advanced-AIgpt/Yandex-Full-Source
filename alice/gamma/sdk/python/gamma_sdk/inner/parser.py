# coding: utf-8

import re

import attr

from gamma_sdk.inner.lexer import lexer


class ParserSyntaxError(Exception):
    def __init__(self, cause):
        self.cause = cause
        super(Exception, self).__init__()


@attr.s(frozen=True)
class ParsedTree:
    expression = attr.ib()
    variables = attr.ib(factory=list)

    def to_regex(self):
        return r'^' + self.expression.to_regex() + r'$'


@attr.s(frozen=True)
class OrNode:
    alternatives = attr.ib(factory=list)

    def to_regex(self):
        return r'(?:{})'.format('|'.join(alternative.to_regex() for alternative in self.alternatives))


@attr.s(frozen=True)
class SequenceNode:
    sequence = attr.ib(factory=list)

    def to_regex(self):
        return r'(?:\b| )'.join(element.to_regex() for element in self.sequence)


@attr.s(frozen=True)
class MaybeNode:
    expression = attr.ib()

    def to_regex(self):
        if isinstance(self.expression, OrNode):
            return r'{}?'.format(self.expression.to_regex())
        return r'(?:{})?'.format(self.expression.to_regex())


@attr.s(frozen=True)
class AnyNode:
    def to_regex(self):
        return r'.*'


@attr.s(frozen=True)
class TextNode:
    value = attr.ib(type=str)
    inflect = attr.ib(type=bool, default=False)
    prefix = attr.ib(type=bool, default=False)
    suffix = attr.ib(type=bool, default=False)

    def to_regex(self):
        value = re.escape(self.value)
        if self.prefix:
            value = r'.*' + value
        if self.suffix:
            value = value + r'.*'
        return value


@attr.s(frozen=True)
class VariableNode:
    name = attr.ib(type=str)
    type = attr.ib(type=str)
    value = attr.ib(type=str, default=None)

    def to_regex(self):
        return r'(?P<{}>[^ ]+?)'.format(self.name)


# Recursive descent LL(1) parser
class Parser:

    def __init__(self, stream):
        self._stream = stream
        self._peek = None
        self.stop = False
        self.variables = []

    def peek(self):
        try:
            if self._peek is None:
                self._peek = next(self._stream)
            return self._peek
        except StopIteration:
            self.stop = True
            return None

    def move(self):
        try:
            if self._peek is None:
                next(self._stream)
            else:
                self._peek = None
        except StopIteration:
            self.stop = True
            return None

    # EXPRESSION := SUB_EXPRESSION | SUB_EXPRESSION <OR> EXPRESSION
    # SUB_EXPRESSION := ELEMENT | ELEMENT SUB_EXPRESSION
    # ELEMENT := <TEXT> | <VARIABLE> | <STAR> |
    #            <LPAREN> EXPRESSION <RPAREN> | <LBRACKET> EXPRESSION <RBRACKET>

    def parse(self):
        tree = ParsedTree(expression=self.expression(), variables=self.variables)
        if not self.stop:
            raise SyntaxError("Unknown symbol %s" % self.peek())
        return tree

    def expression(self):
        result = []
        while not self.stop:
            sub_expression = self.sub_expression()
            result.append(sub_expression)
            token = self.peek()
            if self.stop or token.type != 'OR':
                break
            self.move()

        if len(result) == 1:
            return result[0]
        return OrNode(alternatives=result)

    def sub_expression(self):
        result = []
        while not self.stop:
            element = self.element()
            if element is None:
                break
            result.append(element)
        if len(result) == 1:
            return result[0]
        return SequenceNode(sequence=result)

    def element(self):
        token = self.peek()
        if self.stop:
            return None
        if token.type == 'TEXT':
            self.move()
            return TextNode(
                value=token.value.value,
                inflect=token.value.inflect,
                prefix=token.value.prefix,
                suffix=token.value.suffix,
            )
        if token.type == 'STAR':
            self.move()
            return AnyNode()
        if token.type == 'VARIABLE':
            self.move()
            variable = VariableNode(name=token.value.name, type=token.value.type, value=token.value.value)
            self.variables.append(variable)
            return variable
        if token.type == 'LPAREN':
            self.move()
            expression = self.expression()
            if self.peek().type != 'RPAREN':
                raise ParserSyntaxError('No closing parenthesis')
            self.move()
            return expression
        if token.type == 'LBRACKET':
            self.move()
            expression = self.expression()
            if self.peek().type != 'RBRACKET':
                raise ParserSyntaxError('No closing bracket')
            self.move()
            return MaybeNode(expression=expression)
        return None


def parse(string):
    return Parser(lexer(string)).parse()
