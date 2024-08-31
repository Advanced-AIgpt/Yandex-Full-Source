# coding: utf-8

from __future__ import unicode_literals

import os
import pytest
import yatest.common

from alice.nlg.library.python.codegen import errors
from alice.nlg.library.python.codegen_tool import compile_cpp as do_compile_cpp
from jinja2 import exceptions as jinja_exc

TEMPLATES_DIR = yatest.common.source_path(
    'alice/nlg/library/python/codegen/ut/failed_templates'
)

TEST_PARAMS = [
    (['autoescape.nlg'], errors.UnsupportedFeatureError),
    (['block.nlg'], errors.UnsupportedFeatureError),
    (['call_block_caller.nlg'], errors.UnsupportedFeatureError),
    (['call_block_dict.nlg'], errors.CallTransformationError),
    (['call_block_namespace.nlg'], errors.CallTransformationError),
    (['call_block_range.nlg'], errors.CallTransformationError),
    (['call_duplicate_kwarg.nlg'], errors.CallResolutionError),
    (['call_invalid_kwarg.nlg'], errors.CallResolutionError),
    (['call_too_few_args.nlg'], errors.CallResolutionError),
    (['call_too_many_args.nlg'], errors.CallResolutionError),
    (['caller_args.nlg'], errors.UnsupportedFeatureError),
    (['caller_params.nlg'], errors.UnsupportedFeatureError),
    (['dict_int.nlg'], errors.UnsupportedFeatureError),
    (['dict_tuple.nlg'], errors.UnsupportedFeatureError),
    (['extends.nlg', 'dummy.nlg'], errors.UnsupportedFeatureError),
    (['for_if.nlg'], errors.UnsupportedFeatureError),
    (['from_import_collision.nlg', 'dummy.nlg', 'dummy2.nlg'], errors.ImportError),
    (['from_import_int.nlg'], errors.ImportError),
    (['from_import_var.nlg', 'dummy.nlg'], errors.UnsupportedFeatureError),
    (['if_from_import.nlg', 'dummy.nlg'], errors.ImportError),
    (['if_import.nlg', 'dummy.nlg'], errors.ImportError),
    (['if_macro.nlg'], errors.MacroDefinitionError),
    (['import_collision.nlg', 'dummy.nlg', 'dummy2.nlg'], errors.ImportError),
    (['import_from_import_collision.nlg', 'dummy.nlg', 'dummy2.nlg'], errors.UnsupportedFeatureError),
    (['import_int.nlg'], errors.ImportError),
    (['import_var.nlg', 'dummy.nlg'], errors.UnsupportedFeatureError),
    (['import_var_collision.nlg', 'dummy.nlg'], errors.UnsupportedFeatureError),
    (['include.nlg', 'dummy.nlg'], errors.UnsupportedFeatureError),
    (['invalid_default_definition.nlg'], errors.MacroDefinitionError),
    (['invalid_filter_args.nlg'], errors.CallResolutionError),
    (['invalid_foreign_from_import.nlg'], errors.ImportError),
    (['invalid_foreign_import.nlg'], errors.ImportError),
    (['invalid_foreign_nlgimport.nlg'], errors.ImportError),
    (['invalid_inner_from_import.nlg', 'dummy.nlg'], errors.ImportError),
    (['invalid_inner_import.nlg', 'dummy.nlg'], errors.ImportError),
    (['invalid_inner_nlgimport.nlg', 'dummy.nlg'], errors.ImportError),
    (['invalid_subscript.nlg'], errors.UnsupportedFeatureError),
    (['invalid_subscript_tuple.nlg'], errors.UnsupportedFeatureError),
    (['kwargs_usage.nlg'], errors.CallTransformationError),
    (['kwargs_var.nlg'], errors.UnsupportedFeatureError),
    (['loop_assignment.nlg'], errors.UnsupportedFeatureError),
    (['loop_method.nlg'], errors.CallTransformationError),
    (['loop_override.nlg'], errors.UnsupportedFeatureError),
    (['loop_override_attr.nlg'], errors.UnsupportedFeatureError),
    (['loop_unknown_attribute.nlg'], errors.UnsupportedFeatureError),
    (['macro_from_import.nlg', 'dummy.nlg'], errors.ImportError),
    (['macro_import.nlg', 'dummy.nlg'], errors.ImportError),
    (['macro_redefinition.nlg'], errors.MacroDefinitionError),
    (['missing_module.nlg'], errors.CallTransformationError),
    (['non_binary_comparison.nlg'], errors.UnsupportedFeatureError),
    (['range_kwargs.nlg'], errors.RangeArgumentsError),
    (['range_too_many_args.nlg'], errors.RangeArgumentsError),
    (['recursive_for.nlg'], errors.UnsupportedFeatureError),
    (['syntax_error.nlg'], jinja_exc.TemplateSyntaxError),
    (['unknown_call_pattern.nlg'], errors.CallTransformationError),
    (['unknown_filter.nlg'], errors.UnknownBuiltinError),
    (['unknown_macro.nlg'], errors.CallTransformationError),
    (['unknown_macro_in_module.nlg', 'dummy.nlg'], errors.CallResolutionError),
    (['unknown_module.nlg'], errors.CallTransformationError),
    (['unknown_test.nlg'], errors.UnsupportedFeatureError),
    (['varargs_usage.nlg'], errors.CallTransformationError),
    (['varargs_var.nlg'], errors.UnsupportedFeatureError),
]


@pytest.mark.parametrize(
    'nlg_names,exc_class',
    TEST_PARAMS,
    ids=[
        os.path.splitext(templates[0])[0]
        for templates, _ in TEST_PARAMS
    ],
)
def test_failed_template(nlg_names, exc_class):
    with pytest.raises(errors.TemplateCompilationError) as exc_info:
        compile_cpp(nlg_names)

    assert type(exc_info.value.cause) is exc_class


def compile_cpp(nlg_names):
    template_paths = [
        os.path.join(TEMPLATES_DIR, nlg_name)
        for nlg_name in nlg_names
    ]
    out_dir = yatest.common.test_output_path('generated')
    do_compile_cpp(template_paths, out_dir, TEMPLATES_DIR, TEMPLATES_DIR, False)
