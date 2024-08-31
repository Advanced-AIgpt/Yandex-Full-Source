import functools

import jsonschema
from jupyterhub.utils import admin_only  # noqa
from tornado import web


NAME_RE = r'(?P<name>[^/]+)'
PATH_RE = r'(?P<path>(?:/[^/]+)+)'


def admin_or_self(method):
    @functools.wraps(method)
    def m(self, name=None, *args, **kwargs):
        current = self.current_user
        name = name or current.name
        if current is None:
            raise web.HTTPError(403)
        if not (current.name == name or current.admin):
            raise web.HTTPError(403, f'only andmins and {name} can access this page')

        # raise 404 if not found
        if not self.find_user(name):
            raise web.HTTPError(404, f'unknown user {name}')
        return method(self, name=name, *args, **kwargs)

    return m


def require_server(method):
    def m(self, name, *args, **kwargs):
        user = self.user_from_username(name)

        if user is None:
            raise web.HTTPError(404, f'unknown user {name}')

        if require_server and user.server is None:
            raise web.HTTPError(400, f'user {name} does not have a server')

        return method(self, name=name, *args, **kwargs)

    return m


def json_body(method):
    method_name = method.__name__

    assert method_name in ('post', 'get')

    @functools.wraps(method)
    def m(self, *args, **kwargs):
        data = self.get_json_body()

        schema = getattr(self, method_name + '_schema', None)
        if schema:
            try:
                jsonschema.validate(data, schema=schema)
            except jsonschema.ValidationError as e:
                raise web.HTTPError(400, e.message)

        kwargs['json_data'] = data
        return method(self, *args, **kwargs)

    return m
