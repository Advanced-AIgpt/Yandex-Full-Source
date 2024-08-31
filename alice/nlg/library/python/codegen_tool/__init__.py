# coding: utf-8

import codecs
import logging
import os
import re
import traceback
from collections import OrderedDict

from jinja2.nodes import Node
from vins_core.nlg.template_nlg import TemplateNLG, get_env
from vins_core.utils.strings import smart_unicode

from alice.nlg.library.python.codegen import cpp_compiler, parser
from alice.nlg.library.python.codegen.nodes import Node as TransNode
from alice.nlg.library.python.codegen.scope import DynamicScope, LexicalScope
from alice.nlg.library.python.codegen.transformer import build_library, transform_ast
from alice.nlg.library.python.codegen.keyset_producer import get_keyset


logging.basicConfig()
logger = logging.getLogger(__name__)


class Form(object):
    def __init__(self, name):
        self.name = name

    def __getitem__(self, item):
        raise KeyError(item)


def render_template(template_path, import_dir=None, phrase=None, card=None, context=None, intent=None):
    if context is None:
        context = dict()

    if intent is None:
        intent = os.path.splitext(os.path.basename(template_path))[0]

    logger.info('Compiling %r', template_path)
    with open(template_path) as template_file:
        data = smart_unicode(template_file.read())

        nlg = TemplateNLG(templates_dir=import_dir)
        nlg.add_intent(intent, data)

        if phrase:
            result = nlg.render_phrase(phrase_id=phrase, form=Form(name=intent), context=context)
            return {
                'voice': smart_unicode(result.voice),
                'text': smart_unicode(result.text),
            }
        elif card:
            return nlg.render_card(card_id=card, form=Form(name=intent), context=context)
        else:
            raise ValueError('phrase or card must be specified')


def dump_ast(node, with_localized=True):
    """Dumps the given AST node recursively as a JSON-ready object.
    Adapted from contrib/python/Jinja2/jinja2.
    """
    if not isinstance(node, (Node, TransNode)):
        if isinstance(node, list):
            return [dump_ast(item, with_localized) for item in node]
        if isinstance(node, tuple):
            return tuple(dump_ast(item, with_localized) for item in node)
        if isinstance(node, dict):
            return {name: dump_ast(value, with_localized) for name, value in node.iteritems()}
        if isinstance(node, xrange):
            return list(node)
        if isinstance(node, (DynamicScope, LexicalScope)):
            return repr(node)
        return node  # assume basic type

    if isinstance(node, Node):
        obj = OrderedDict(
            [
                ('_t', node.__class__.__name__),
            ]
        )
    else:
        obj = OrderedDict(
            [
                ('_t', '(T)' + node.__class__.__name__),
            ]
        )

    for attr in node.attributes:
        if attr != 'environment':  # no point in serializing this
            obj[':' + attr] = dump_ast(getattr(node, attr), with_localized)

    fields = list(node.fields)
    if with_localized and hasattr(node, 'localized_node'):
        fields.append('localized_node')
    for field in fields:
        value = getattr(node, field)
        if isinstance(value, list):
            obj[field] = [dump_ast(item, with_localized) for item in value]
        else:
            obj[field] = dump_ast(value, with_localized)

    return obj


def dump_file_ast(template_path, import_dir=None):
    with open(template_path) as template_file:
        data = smart_unicode(template_file.read())

        relative_path = template_path
        if import_dir is not None:
            relative_path = os.path.relpath(template_path, import_dir)
        ast = parser.parse(data, relative_path, import_dir)

        return dump_ast(ast)


def compile_py(template_path, out_file, import_dir=None):
    with open(template_path) as template_file:
        data = smart_unicode(template_file.read())

        env = get_env(import_dir)

        relative_path = template_path
        if import_dir is not None:
            relative_path = os.path.relpath(template_path, import_dir)
        ast = parser.parse(data, relative_path, import_dir, env=env)

        out_file.write(env.compile(ast, raw=True))
        out_file.write('\n')


def dump_trans_ast(template_path, import_dir=None):
    with open(template_path) as template_file:
        data = smart_unicode(template_file.read())

        relative_path = template_path
        if import_dir is not None:
            relative_path = os.path.relpath(template_path, import_dir)
        ast = parser.parse(data, relative_path, import_dir)

        intent = os.path.splitext(os.path.basename(template_path))[0]

        relative_path = template_path
        if import_dir is not None:
            relative_path = os.path.relpath(template_path, import_dir)
        return dump_ast(transform_ast(ast, relative_path, intent))


def get_nlg_files(walk_dir, language):
    filename_pattern = r'_{language}\.nlg$'.format(language=language) if language else r'\.nlg$'

    template_paths = []
    for root, subdirs, files in os.walk(walk_dir):
        for filename in files:
            if re.search(filename_pattern, filename):
                file_path = os.path.join(root, filename)
                template_paths.append(file_path)
    return template_paths


def dump_keyset(import_dir, input, out_file, language=None):
    input_path = os.path.abspath(input)
    if os.path.isdir(input_path):
        template_paths = get_nlg_files(input_path, language)
    else:
        template_paths = [input_path]

    with codecs.open(out_file, 'w+', encoding='utf-8') as file_out:
        for template_path in template_paths:
            logger.info('Preparing %r', template_path)
            try:
                with codecs.open(template_path, "r", encoding="utf-8") as template_file:
                    data = template_file.read()

                    relative_path = template_path
                    if import_dir is not None:
                        relative_path = os.path.relpath(template_path, import_dir)
                    ast = parser.parse(data, relative_path, import_dir)

                    relative_path = template_path
                    if import_dir is not None:
                        relative_path = os.path.relpath(template_path, import_dir)

                    intent = os.path.splitext(os.path.basename(relative_path))[0]
                    ast = transform_ast(ast, relative_path, intent)

                    keyset = get_keyset(data, ast, relative_path, language)
                    if keyset:
                        file_out.write("\n".join(map(unicode, keyset)) + "\n")

            except Exception:
                logger.error("FAILED %s", template_path)
                traceback.print_exc()


def compile_cpp(template_paths, out_dir, import_dir, include_prefix, localized_mode):
    modules = []
    for template_path in template_paths:
        with open(template_path) as template_file:
            data = smart_unicode(template_file.read())

            base_dir = out_dir if template_path.startswith(out_dir) else import_dir
            relative_path = os.path.relpath(template_path, base_dir)
            ast = parser.parse(data, relative_path, import_dir)

            intent = os.path.splitext(os.path.basename(template_path))[0]
            modules.append(transform_ast(ast, relative_path, intent))

    library = build_library(modules, out_dir)
    cpp_compiler.compile_cpp(library, out_dir, include_prefix, localized_mode)
