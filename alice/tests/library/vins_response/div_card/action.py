import json
from urllib.parse import unquote

from cached_property import cached_property


class _ActionClickMixin(object):
    directives_prefix = 'dialog-action://?directives='

    @cached_property
    def directives(self):
        if self.action_url and self.action_url.startswith(self.directives_prefix):
            return json.loads(unquote(self.action_url[len(self.directives_prefix):]))
        return []
