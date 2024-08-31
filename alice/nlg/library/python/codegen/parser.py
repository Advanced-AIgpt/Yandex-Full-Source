# coding: utf-8

from __future__ import unicode_literals

from alice.nlg.library.python.codegen.errors import TemplateCompilationError
from vins_core.nlg import template_nlg


def parse(data, template_path, import_dir=None, env=None):
    try:
        if env is None:
            env = template_nlg.get_env(import_dir)
        return env.parse(data, filename=template_path)
    except Exception as exc:
        TemplateCompilationError.raise_wrapped(exc, template_path)
