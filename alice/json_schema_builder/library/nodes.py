"""Defines nodes for JSON Schema AST, or really a shallow syntax forest,
because we replace nested object schemas with references to ease code generation.
"""

from alice.json_schema_builder.library import errors
from collections import OrderedDict


class Node:
    """Base class for AST nodes.
    Defines attrs, and extras; both will be used for reflection,
    but extras are considered less important and aren't meant to be overridden.
    """
    attrs = ()
    extras = ('location',)

    def __init__(self, **kwargs):
        attrs = self.attrs
        extras = Node.extras  # shouldn't override these

        for attr in attrs:
            setattr(self, attr, None)

        for attr in extras:
            setattr(self, attr, None)

        for attr, value in kwargs.items():
            if attr not in attrs and attr not in extras:
                raise errors.NodeInitializationError('Invalid kwarg: {!r}'.format(attr))
            setattr(self, attr, value)

    def to_json(self):
        """util.JSONEncoder uses this method to convert nodes into stuff
        that can be serialized as JSON directly.
        """
        result = OrderedDict([(':type', type(self).__name__)])
        result.update(self._attr_values())
        return result

    def __repr__(self):
        return '{}({})'.format(
            type(self).__name__,
            ', '.join(
                '{}={!r}'.format(key, value)
                for key, value in self._attr_values().items()
            )
        )

    def __eq__(self, other):
        if type(self) != type(other):
            return False

        # ignore extras, we view them as unimportant to the node's identity
        return all(
            getattr(self, attr) == getattr(other, attr)
            for attr in self.attrs
        )

    def __hash__(self):
        return hash(tuple(self._attr_values(include_extras=False).items()))

    def _attr_values(self, include_extras=True):
        """Dumps attrs and extras into an ordered dictionary for serialization.
        """
        attrs = self.attrs
        extras = Node.extras  # shouldn't override these

        result = OrderedDict()
        for attr in attrs:
            result[attr] = getattr(self, attr)
        if include_extras:
            for extra in extras:
                result[extra] = getattr(self, extra)
        return result


class Ref(Node):
    """JSON schema reference, also used as a location extra.
    It usually represents a schema pointer or a nested object schema:
    - `filename` is a string,
    - `path` is a tuple, its elements are either strings or ints
    Relative pointers cannot be represented by this class.
    """
    attrs = ('filename', 'path')

    def child(self, key):
        return Ref(filename=self.filename, path=(self.path or ()) + (key,), location=self.location)

    def __str__(self):
        if not self.path:
            return self.filename or '#'

        return '{}#{}'.format(
            self.filename or '',
            ''.join('/' + str(part) for part in self.path)
        )

    def __repr__(self):
        return super().__repr__()  # so that super().to_json() is used

    def to_json(self):
        return str(self)  # allows using refs as JSON keys


class Builtin(Node):
    attrs = ('fmt',)


class Variant(Node):
    attrs = ('alternatives',)


class JsonPayload(Node):
    pass


class Object(Node):
    attrs = ('properties', 'required', 'constants')


class Array(Node):
    attrs = ('items', 'min_items')


class Number(Node):
    attrs = ('constraint',)


class Integer(Node):
    attrs = ('constraint',)


class String(Node):
    attrs = ('min_length', 'max_length', 'pattern')


class StringEnum(Node):
    attrs = ('options',)
