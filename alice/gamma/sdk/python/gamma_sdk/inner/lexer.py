# coding: utf-8

import attr
import re

from ply import lex


@attr.s(frozen=True)
class Variable:
    name = attr.ib(type=str)
    type = attr.ib(type=str)
    value = attr.ib(type=str, default=None)


@attr.s(frozen=True)
class Text:
    value = attr.ib(type=str)
    inflect = attr.ib(type=bool, default=False)
    prefix = attr.ib(type=bool, default=False)
    suffix = attr.ib(type=bool, default=False)


@attr.s(frozen=True)
class Token:
    type = attr.ib(type=str)
    value = attr.ib(type=str, default=None)


tokens = (
    'TEXT',
    'VARIABLE',
    'LPAREN', 'RPAREN',
    'LBRACKET', 'RBRACKET',
    'STAR',
    'OR',
)

t_LPAREN = r'\('
t_RPAREN = r'\)'
t_LBRACKET = r'\['
t_RBRACKET = r'\]'
t_STAR = r'\*'
t_OR = r'\|'


t_ignore = ' \t'


def t_error(t):
    print('Illegal character \'%s\'' % t.value[0])
    t.lexer.skip(1)


_escaped_characters = re.compile(r'\\([$\[\]()*|])')


def t_TEXT(t):
    r'(\*|~)?(?:\\[$\[\]()*|~]|[^*|$\\ \t\n()\[\]~])+\*?'
    value = t.value
    inflect = False
    suffix = False
    prefix = False
    if value[0] == '*':
        value = value[1:]
        prefix = True
    elif value[0] == '~':
        value = value[1:]
        inflect = True
    if value[-1] == '*':
        value = value[:-1]
        suffix = True
    value = _escaped_characters.sub(r'\g<1>', value)
    t.value = Text(value=value, inflect=inflect, prefix=prefix, suffix=suffix)
    return t


def t_VARIABLE(t):
    r'\$(?P<name>[a-zA-Z_][0-9a-zA-Z_]*)(?P<type>:[a-zA-Z_][0-9a-zA-Z_]*)?(?P<value>\.[0-9a-zA-Z_]*)?'
    var_name = t.lexer.lexmatch.group('name')
    var_type = t.lexer.lexmatch.group('type')
    if not var_type:
        var_type = var_name
    else:
        var_type = var_type[1:]
    var_value = t.lexer.lexmatch.group('value')
    if var_value:
        var_value = var_value[1:]
    t.value = Variable(var_name, var_type, var_value)
    return t


_lexer = lex.lex(reflags=lex.re.UNICODE | re.VERBOSE)


def lexer(string):
    _lexer.input(string)
    while True:
        token = _lexer.token()
        if not token:
            break
        yield Token(token.type, token.value)
