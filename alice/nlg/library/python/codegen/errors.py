# coding: utf-8

from __future__ import unicode_literals

import traceback

from future.utils import raise_from


class CodegenError(Exception):
    """Base class for codegen exceptions.
    """


class TemplateCompilationError(CodegenError):
    """Top-level exception that wraps the actual exception
    and location information.
    """
    def __init__(self, cause, filename, lineno):
        message = 'NLG Compilation failed at {}:{}, cause: {!r}. Original traceback:\n{}'.format(
            filename,
            lineno,
            cause,
            traceback.format_exc()
        )

        super(TemplateCompilationError, self).__init__(message)
        self.cause = cause
        self.filename = filename
        self.lineno = lineno

    @classmethod
    def raise_wrapped(cls, exc, filename='<unknown>', lineno='??'):
        exc_filename = getattr(exc, 'filename', filename)
        exc_lineno = getattr(exc, 'lineno', lineno)
        wrapper_exc = cls(exc, exc_filename, exc_lineno)
        raise_from(wrapper_exc, exc)


class TransformationError(CodegenError):
    """Raised in the AST transformation stage.
    """


class RangeArgumentsError(TransformationError):
    """Raised when a range is improperly constructed.
    """


class CallTransformationError(TransformationError):
    """Static macro resolution failed, unknown built-in macro,
    or *args or **kwargs are used.

    By design, macros are *not* first-class values in our implementation of Jinja2,
    meaning the following template will *fail* to compile:

    {% from "common/macros_ru.nlg" import foo %}
    {% set bar = foo %}
    {{ bar() }}
    """


class MacroDefinitionError(TransformationError):
    """Raised when a macro is defined in a way that we don't support.
    """


class ImportError(TransformationError):
    """Raised when an import is defined in a way that we don't support.
    """


class UnsupportedFeatureError(TransformationError):
    """A feature was encountered that we chose not to support.
    """


class UnexpectedNodeError(TransformationError):
    """Node encountered in a way that's plausible but that we don't support.
    Usually it's because an extension added this node, and we don't support
    this extension.
    """


class CompilationError(CodegenError):
    """Raised in the compilation stage.
    """


class CallResolutionError(CompilationError):
    """The call cannot be resolved at the compiler stage.
    Either the module doesn't have the macro, or arguments passed are invalid.
    """


class UnknownBuiltinError(CompilationError):
    """No argument spec found for the given built-in function, filter, test, or macro.
    """
