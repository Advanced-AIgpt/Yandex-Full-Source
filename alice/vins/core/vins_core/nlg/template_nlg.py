# coding: utf-8
from __future__ import unicode_literals

import logging
import re
import random
import attr
import json
from StringIO import StringIO
from datetime import datetime

from dateutil.parser import parse as parse_dt
from jinja2 import Environment, TemplateAssertionError, nodes

from vins_core.utils.datetime import parse_tz, create_date_safe, timestamp_to_datetime
from vins_core.utils.data import validate_json

from vins_core.nlg.nlg_extension import (
    NLGExtension, create_chooseline, create_nlgimport,
    NLGTemplateLoader, Block, ONLY_VOICE_MARK, ONLY_TEXT_MARK,
    create_branched_chooseline
)
from vins_core.nlg import filters, tests

logger = logging.getLogger(__name__)

_filters_extension = {
    'insert_spaces_between_chars': filters.insert_spaces_between_chars,
    'insert_comas_between_chars': filters.insert_comas_between_chars,
    'split_long_numbers': filters.split_long_numbers,
    'percent_cases': filters.percent_cases,
    'format_datetime': filters.format_datetime,
    'seconds_diff': filters.seconds_diff,
    'geo_city_address': filters.geo_city_address,
    'city_prepcase': filters.city_prepcase,
    'format_weekday': filters.format_weekday,
    'inflect': filters.inflect,
    'parse_dt': parse_dt,
    'choose_tense': filters.choose_tense,
    'render_datetime_raw': filters.render_datetime_raw,
    'safe_render_datetime_raw': filters.safe_render_datetime_raw,
    'human_date': filters.human_date,
    'human_day_rel': filters.human_day_rel,
    'human_time': filters.human_time,
    'human_seconds': filters.human_seconds,
    'human_seconds_short': filters.human_seconds_short,
    'human_meters': filters.human_meters,
    'human_meters_short': filters.human_meters_short,
    'human_month': filters.human_month,
    'decapitalize_first': filters.decapitalize_first,
    'decapitalize_all': filters.decapitalize_all,
    'capitalize_first': filters.capitalize_first,
    'capitalize_all': filters.capitalize_all,
    'singularize': filters.singularize,
    'pluralize': filters.pluralize,
    'shuffle': filters.shuffle,
    'join': filters.join,
    'emojize': filters.emojize,
    'trim_with_ellipsis': filters.trim_with_ellipsis,
    'markup': filters.markup,
    'get_item': filters.get_item,
    'hostname': filters.hostname,
    'to_json': filters.to_json,
    'html_escape': filters.html_escape,
    'json_escape': filters.json_escape,
    'render_units_time': filters.render_units_time,
    'render_weekday_type': filters.render_weekday_type,
    'render_short_weekday_name': filters.render_short_weekday_name,
    'human_time_raw': filters.human_time_raw,
    'human_time_raw_text_and_voice': filters.human_time_raw_text_and_voice,
    'tts_domain': filters.tts_domain,
    'number_of_readable_tokens': filters.number_of_readable_tokens,
    'human_age': filters.human_age,
    'opinions_count': filters.opinions_count,
    'cut_voice_tags': filters.cut_voice_tags,
    'cut_text_tags': filters.cut_text_tags,
    'cut_voice_tags_without_text': filters.cut_voice_tags_without_text,
    'cut_text_tags_without_text': filters.cut_text_tags_without_text,
    'only_voice': filters.only_voice,
    'only_text': filters.only_text,
    'end_with_dot': filters.end_with_dot,
    'end_without_dot': filters.end_without_dot,
    'end_without_terminal': filters.end_without_terminal,
    'end_without_dot_smart': filters.end_without_dot_smart,
    'remove_angle_brackets': filters.remove_angle_brackets
}

_globals_extension = {
    'randuniform': random.uniform,
    'parse_tz': parse_tz,
    'parse_dt': parse_dt,
    'datetime': datetime,
    'create_date_safe': create_date_safe,
    'timestamp_to_datetime': timestamp_to_datetime,
}

_tests_extension = {
    'relative_datetime_raw': tests.relative_datetime_raw,
}


class VinsAppLogicError(Exception):
    pass


def register_filter(name, func):
    assert name not in _filters_extension
    _filters_extension[name] = func


def register_global(name, func):
    assert name not in _globals_extension
    _globals_extension[name] = func


def register_test(name, func):
    assert name not in _tests_extension
    _tests_extension[name] = func


def get_env(nlg_templates=None, add_globals=None):
    env = Environment(
        extensions=[NLGExtension, 'jinja2.ext.do'],
        line_comment_prefix='#',
        cache_size=1000,
        auto_reload=False,
    )
    env.filters.update(_filters_extension)
    env.globals.update(_globals_extension)
    if add_globals is not None:
        env.globals.update(add_globals)

    env.tests.update(_tests_extension)
    paths = [
        'vins_core/nlg/macros',
    ]
    if nlg_templates and isinstance(nlg_templates, basestring):
        paths.append(nlg_templates)
    elif nlg_templates and isinstance(nlg_templates, list):
        paths += nlg_templates
    env.loader = NLGTemplateLoader(paths)
    return env


def create_simple_phrases(phrases, includes=(), mode=None):
    """ Create NLG Template with phrases

    phrases - sequence of tuples [(phrase_id, [phrases_lines]), ...]
    includes - files that should be included into NLG
    """
    result = StringIO()
    for filename in includes:
        result.write(create_nlgimport(filename))

    for phrase_id, phrase_lines in phrases:
        result.write(create_chooseline(phrase_id, phrase_lines, mode))

    return result.getvalue()


def create_branched_phrase(phrase_id, blocks, includes=(), mode=None):
    result = StringIO()
    for filename in includes:
        result.write(create_nlgimport(filename))

    result.write(create_branched_chooseline(phrase_id, blocks, mode))

    return result.getvalue()


@attr.s
class TemplateRenderResult(object):
    voice = attr.ib()
    text = attr.ib()
    raw_output = attr.ib()


@attr.s
class JinjaTemplateWrapper(object):
    template = attr.ib()
    macro = attr.ib()

    def render(self, *args, **kwargs):
        vars = dict(*args, **kwargs)
        module = self.template.make_module(vars)
        return getattr(module, self.macro)()


class Template(object):
    """ One Template per intent """

    def __init__(self, string=None, filename=None, env=None):
        assert string or filename

        self._env = env or get_env()
        self._filename = filename
        self._phrases, self._cards, self._cardtemplates = self._compile(string, filename)

    @classmethod
    def from_file_list(cls, files, env=None):
        phrases = {}
        cards = {}
        t = None

        for file_ in files:
            t = cls(filename=file_, env=env)
            phrases.update(t._phrases)
            cards.update(t._cards)

        if t is not None:
            t._filename = '<globals>'
            t._phrases = phrases
            t._cards = cards

        return t

    def __unicode__(self):
        return '<%s: %s>' % (self.__class__.__name__, self._filename)

    def __repr__(self):
        return unicode(self).encode('utf-8')

    def _compile(self, string, filename):
        if string is None:
            ast = self._env.loader.get_ast(self._env, filename)
            default_template = None
            template = filename

            # fast fix: it commit template to Jinja2's cache when nlg doesn't contains phrases and cards
            # (e.g. nlg contains only macros)
            # TODO(ikorobtsev): correctly fix needed
            self._env.get_template(filename)
        else:
            ast = self._env.parse(string)
            code = self._env.compile(ast, filename=filename)
            # TODO(ikorobtsev): fix - impl doesn't use Jinja's cache
            default_template = self._env.template_class.from_code(self._env, code, self._env.globals)
            template = None

        blocks = []
        for node in ast.find_all((nodes.Import, nodes.Macro)):
            if getattr(node, 'import_type', None) == 'nlgimport':
                blocks.extend(node.exports)
            elif getattr(node, 'block_type', None) is not None:
                blocks.append(Block(node, template))

        phrases = {}
        cards = {}
        cardtemplates = {}
        for block in blocks:
            if block.type == 'phrase':
                dct = phrases
            elif block.type == 'card':
                dct = cards
            elif block.type == 'cardtemplate':
                dct = cardtemplates
            else:
                raise TemplateAssertionError(
                    "Unknown block type %s %s:%s" % (block.type, block.template or filename, block.lineno)
                )

            if block.template:
                template = self._env.get_template(block.template)
            else:
                template = default_template

            dct[block.name] = JinjaTemplateWrapper(template, block.macro_name)

        return phrases, cards, cardtemplates

    def postprocess(self, s):
        # delete spaces before punctuation
        s = re.sub(r'\s+(?=[.,:!?;»])', '', s, flags=re.UNICODE)
        # collapse spaces
        s = re.sub(r'\s+', ' ', s, flags=re.UNICODE)
        # unquote newline
        return re.sub(r'\s*\\n\s*', '\n', s, flags=re.UNICODE).strip()

    def render_phrase(self, phrase_id, **context):
        rv = self._phrases[phrase_id].render(**context)

        # To get voice message we erase all content inside <vins_only_text>...</vins_only_text> tags (including tags
        # themselves) and keep <vins_only_voice>...</vins_only_voice> content (erasing tags themselves)
        # To get text message we do the exact opposite thing

        voice = re.sub(r'<%s>((?!</%s>).)*</%s>' % ((ONLY_TEXT_MARK,) * 3), '', rv, flags=re.UNICODE | re.DOTALL)
        voice = re.sub(r'</?%s>' % ONLY_VOICE_MARK, '', voice, flags=re.UNICODE | re.DOTALL)
        text = re.sub(r'<%s>((?!</%s>).)*</%s>' % ((ONLY_VOICE_MARK,) * 3), '', rv, flags=re.UNICODE | re.DOTALL)
        text = re.sub(r'</?%s>' % ONLY_TEXT_MARK, '', text, flags=re.UNICODE | re.DOTALL)

        # In both cases we do the post processing (spaces, newlines etc.)
        return TemplateRenderResult(self.postprocess(voice), self.postprocess(text), rv)

    def render_cardtemplate(self, template_id, **context):
        rendered = self._cardtemplates[template_id].render(**context)
        rendered = self.postprocess(rendered)

        try:
            json_obj = json.loads(rendered)
        except ValueError:
            logger.error('Failed to render card template due to malformed json: %s', rendered)
            raise

        return json_obj

    def render_card(self, card_id, schema=None, **context):
        rendered = self._cards[card_id].render(**context)
        rendered = self.postprocess(rendered)

        try:
            json_obj = json.loads(rendered)
        except ValueError:
            logger.error('Failed to render card due to malformed json: %s', rendered)
            raise

        if schema:
            validate_json(json_obj, schema)
        return json_obj

    def get_phrase(self, phrase_id):
        return self._phrases.get(phrase_id)

    def get_card(self, card_id):
        return self._cards.get(card_id)

    def get_cardtemplate(self, template_id):
        return self._cardtemplates.get(template_id)


class FormSlotValues(object):
    def __init__(self, form):
        self._form = form

    def __getitem__(self, item):
        return self._form[item]

    @property
    def raw_form(self):
        return self._form


class TemplateNLG(object):
    def __init__(self, global_templates=None, templates_dir=None, add_globals=None):
        self._env = get_env(templates_dir, add_globals)
        self._templates = {}
        if global_templates is not None:
            tmplt = Template.from_file_list(global_templates, self._env)
            if tmplt:
                self._templates[None] = tmplt

    @property
    def env(self):
        return self._env

    def add_intent(self, intent_name, data=None, filename=None):
        assert filename or data
        self._templates[intent_name] = Template(data, filename, self._env)

    def _get_context(self, form=None, context=None, req_info=None, session=None):
        return dict(
            form=form and FormSlotValues(form),
            context=context,
            intent_name=form and form.name,
            req_info=req_info,
            session=session
        )

    def render_phrase(self, phrase_id, form=None, context=None, req_info=None, postprocess_list=None, session=None):
        intent_name = form and form.name
        template = self.get_phrase_template(intent_name, phrase_id)
        render_context = self._get_context(form, context, req_info, session)
        result = template.render_phrase(phrase_id, **render_context)

        if postprocess_list is not None:
            for postprocess_phrase_name in postprocess_list:
                logger.info("Applying postprocessor %s", postprocess_phrase_name)
                render_context["current_phrase"] = result.raw_output
                result = self._templates[None].render_phrase(postprocess_phrase_name, **render_context)

        return result

    def render_cardtemplate(self, template_id, form=None, context=None, req_info=None, schema=None):
        if context is None:
            context = {}

        intent_name = form and form.name

        template = self._get_block(intent_name, template_id, 'cardtemplate')
        context['template_id'] = template_id
        render_context = self._get_context(form, context, req_info)
        return template.render_cardtemplate(template_id, **render_context)

    def render_card(self, card_id, form=None, context=None, req_info=None, schema=None):
        if context is None:
            context = {}

        intent_name = form and form.name

        template = self.get_card_template(intent_name, card_id)
        context['card_id'] = card_id
        render_context = self._get_context(form, context, req_info)
        return template.render_card(card_id, schema=schema, **render_context)

    def get_phrase_template(self, intent_name, phrase_id):
        tmplt = self._get_block(intent_name, phrase_id, 'phrase')
        if tmplt is None:
            raise VinsAppLogicError(
                "missing phrase_id '%s' for intent '%s'" % (phrase_id, intent_name)
            )
        return tmplt

    def get_card_template(self, intent_name, card_id):
        tmplt = self._get_block(intent_name, card_id, 'card')
        if tmplt is None:
            raise VinsAppLogicError(
                "missing card_id '%s' for intent '%s'" % (card_id, intent_name)
            )
        return tmplt

    def _get_block(self, intent, id_, type_):
        assert type_ in ('phrase', 'card', 'cardtemplate')
        intent_tmplt = self._templates.get(intent)
        global_tmplt = self._templates.get(None)

        for tmplt in (intent_tmplt, global_tmplt):
            if tmplt is None:
                continue

            if type_ == 'phrase':
                f = tmplt.get_phrase
            elif type_ == 'card':
                f = tmplt.get_card
            elif type_ == 'cardtemplate':
                f = tmplt.get_cardtemplate

            if f(id_):
                return tmplt

        return None

    def has_phrase(self, phrase_id, intent_name=None):
        tmplt = self._get_block(intent_name, phrase_id, 'phrase')
        return tmplt is not None

    def has_card(self, card_id, intent_name=None):
        tmplt = self._get_block(intent_name, card_id, 'card')
        return tmplt is not None

    def has_cardtemplate(self, template_id, intent_name=None):
        tmplt = self._get_block(intent_name, template_id, 'cardtemplate')
        return tmplt is not None
