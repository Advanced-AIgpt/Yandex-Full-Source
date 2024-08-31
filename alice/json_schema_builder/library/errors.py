class ParserUsageError(Exception):
    """JSONSchemaParser is likely misused so the parsing can't continue.
    """


class InvalidAllOfError(Exception):
    """Invalid value for the "allOf" key.
    """


class InvalidReferenceError(Exception):
    """Tried parsing an invalid $ref.
    """


class DictMergeError(Exception):
    """Failed to recursively merge dictionaries.
    """


class NodeInitializationError(Exception):
    """Improper AST node initialization.
    """


class UnknownNodeError(Exception):
    """Got a raw schema that we don't know how to parse.
    """


class InvalidAnyOfError(Exception):
    """anyOf schema expects references as its alternatives.
    """


class InvalidAdditionalPropertiesError(Exception):
    """Got additionalProperties = true on a non-object schema.
    """


class InvalidObjectError(Exception):
    """Something's wrong with an object schema.
    """


class InvalidArrayError(Exception):
    """Something's wrong with an array schema.
    """


class InvalidFieldTypeError(Exception):
    """A schema field failed a type check.
    """


class UnexpectedBuiltinTypeError(Exception):
    """A schema with a built-in format has unexpected type.
    """


class InvalidEnumError(Exception):
    """Invalid enumeration schema encountered.
    """


class InvalidStringError(Exception):
    """Invalid string schema encountered.
    """


class UnsupportedBuiltinError(Exception):
    """Code generator can't handle a built-in.
    """


class UnsupportedVariantError(Exception):
    """Unsupported variant type found.
    """


class UnsupportedArrayError(Exception):
    """Unsupported array property.
    """
