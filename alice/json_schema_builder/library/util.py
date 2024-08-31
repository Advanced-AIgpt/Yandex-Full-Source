import json
import sys

from alice.json_schema_builder.library import errors, nodes
from collections import OrderedDict


def filter_nested(obj, remove_pred, default=None):
    """Filters nested JSON-like objects by removing keys according to the given predicate.
    - Filters values recursively for lists and dicts
    - Removes a list or dict iff both it and all of its keys/indices satisfy remove_pred
      - That is, it removes a list/dict if it was marked for removal in the parent node
        and all of its contents were removed recursively
    - Proceeds in DFS post-order so you can rely on objects' identities.
    """
    def _filter_object(obj):
        if isinstance(obj, list):
            results = map(_filter_object, obj)
            filtered_results = [
                item
                for index, item in enumerate(results)
                if not remove_pred(obj, index) or item and isinstance(item, (list, dict))
            ]

            return filtered_results

        if isinstance(obj, dict):
            results = map(_filter_object, obj.values())
            filtered_results = OrderedDict(
                (key, value)
                for key, value in zip(obj.keys(), results)
                if not remove_pred(obj, key) or value and isinstance(value, (list, dict))
            )
            return filtered_results

        return obj

    return _filter_object(obj)


def recursive_merge(values):
    def _merge(result, obj):
        if not isinstance(obj, dict):
            raise errors.DictMergeError('Expected a dict, got {!r}'.format(type(obj).__name__))

        for key, value in obj.items():
            result_value = result.get(key)
            if isinstance(result_value, dict):
                _merge(result_value, value)
                continue
            if isinstance(result_value, list):
                if not isinstance(value, list):
                    raise errors.DictMergeError('Expected a list, got {!r}'.format(type(obj).__name__))
                result_value.extend(value)
                continue
            result[key] = value

    result = OrderedDict()
    for value in values:
        _merge(result, value)
    return result


def jsonize_objects(obj):
    """Transforms nested objects into JSON, applying the to_json method when available.
    """
    if isinstance(obj, dict):
        return OrderedDict(
            (jsonize_objects(key), jsonize_objects(value))
            for key, value in obj.items()
        )

    if isinstance(obj, (list, tuple)):
        return list(map(jsonize_objects, obj))

    to_json = getattr(obj, 'to_json', None)
    if to_json:
        return jsonize_objects(to_json())
    return obj


def dump_objects(objs, out_file=sys.stdout):
    """Dumps objects as JSON, uses the to_json method to transform them when appropriate.
    """
    json.dump(jsonize_objects(objs), out_file, ensure_ascii=False, indent=4)


def iter_json_objects(contents, filename=None, stop=None):
    """Iterates over parsed JSON and yields all JSON objects (meaning dicts)
    it finds in a DFS preorder.
    """
    json_stack = []

    def _location_factory():
        """Constructs a current location reference with the immutable version
        of the current JSON path stack.
        """
        return nodes.Ref(filename=filename, path=tuple(json_stack))

    def _iterate(obj):
        # NOTE(a-square): we could use coroutines instead, but
        # being able to iterate over objects in a for loop is
        # more convenient by comparison
        if isinstance(obj, dict):
            yield _location_factory, obj

            if stop is not None and stop(obj):
                return

            for key, value in obj.items():
                json_stack.append(key)
                yield from _iterate(value)
                json_stack.pop()
        elif isinstance(obj, list):
            for index, value in enumerate(obj):
                json_stack.append(index)
                yield from _iterate(value)
                json_stack.pop()

    yield from _iterate(contents)


def dump_graphvis(graph, out=sys.stdout):
    def _quote(value):
        return json.dumps(jsonize_objects(value), ensure_ascii=False)

    print('strict digraph {', file=out)

    for vertex, deps in graph.items():
        print('    {} -> {{{}}}'.format(_quote(vertex), ' '.join(map(_quote, deps))), file=out)

    print('}', file=out)
