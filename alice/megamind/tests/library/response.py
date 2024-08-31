import json


class ItemPath:
    def __init__(self, node):
        self.name = None
        self.node = node

    def remove(self):
        try:
            self.node.pop(self.name)
        except TypeError:
            self.node.pop(int(self.name))

    def process(self, chunk):
        self.name = chunk
        if isinstance(self.node, dict):
            if chunk in self.node:
                return ItemPath(self.node[chunk])
        elif isinstance(self.node, list):
            try:
                idx = int(chunk)
                if idx < len(self.node):
                    return ItemPath(self.node[idx])
            except:
                pass

        return None


class JsonRead(object):
    def modify(self, content):
        return json.loads(content)


class JsonWrite(object):
    def __init__(self, indent=None):
        self.indent = indent

    def modify(self, content):
        return json.dumps(content, ensure_ascii=False, sort_keys=True, indent=self.indent)


class JsonPathAction(object):
    def __init__(self, path, *nested):
        if isinstance(path, list):
            self.path = path
        else:
            self.path = path.split('/')
        self.nested = nested

    def modify(self, content):
        storage = [ItemPath(content)]

        for chunk in self.path:
            top = storage[-1]

            new_item = top.process(chunk)
            if not new_item:
                storage = None
                break

            storage.append(new_item)

        if storage and len(storage) > 1:
            found_node = storage[-2]
            node_value = found_node.node[found_node.name]

            for nested in self.nested:
                node_value = nested.modify(node_value)

            found_node.node[found_node.name] = node_value

        return content


class JsonFieldRemover(object):
    def __init__(self, *args):
        self._remove_paths = args

    def modify(self, content):
        # TODO (petrk) Refactor it.
        # Patch json (remove nodes by path).
        for path in self._remove_paths:
            to_remove = [ItemPath(content)]

            if not isinstance(path, list):
                path = path.split('/')

            for chunk in path:
                top = to_remove[-1]

                new_item = top.process(chunk)
                if not new_item:
                    to_remove = None
                    break

                to_remove.append(new_item)

            if to_remove and len(to_remove) > 1:
                to_remove[-2].remove()

        return content


class JsonFieldRecursiveRemove(object):
    def __init__(self, *args):
        self._remove_names = args

    def modify(self, content):
        for name in self._remove_names:
            self._remove_name(name, content)
        return content

    def _remove_name(self, name, content):
        if isinstance(content, dict):
            try:
                content.pop(name)
            except KeyError:
                pass
            for value in content.values():
                self._remove_name(name, value)
        elif isinstance(content, list):
            for value in content:
                self._remove_name(name, value)


class JsonRepack(JsonPathAction):
    def __init__(self, path):
        super().__init__(path, JsonRead(), JsonWrite())
