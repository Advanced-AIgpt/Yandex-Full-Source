import os
import pathlib
import re

from slugify import slugify


PATH_ALLOWED_CHARS_RE = re.compile(r'[^a-zA-Z0-9_\- .~,()\[\]\{\}+=#$@!]+')


def get_safe_path(*parts):
    path = pathlib.PurePath('.')

    all_parts = []
    for part in parts:
        if isinstance(part, pathlib.PurePath):
            all_parts.extend(part.parts)
        else:
            all_parts.append(str(part))

    for part in all_parts:
        path /= slugify(
            part, lowercase=False, regex_pattern=PATH_ALLOWED_CHARS_RE,
        ).strip()

    return path


def get_arcadia_root():
    env_root = os.environ.get('ARCADIA_ROOT')
    if env_root:
        return env_root

    path = pathlib.Path.cwd()

    while path.parents:
        if (path / '.arcadia.root').exists():
            return path

        path = path.parent

    raise RuntimeError("can't find Arcadia root: program runs outside Arcadia")
