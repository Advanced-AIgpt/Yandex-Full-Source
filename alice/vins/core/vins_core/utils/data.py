# coding: utf-8
from __future__ import unicode_literals, absolute_import

import abc
import errno
import hashlib
import json
import logging
import os
import shutil
import stat
import tarfile
import tempfile
import uuid
from urlparse import urlparse
from copy import copy, deepcopy
from io import BytesIO, TextIOWrapper

import jsonschema
import ujson
import yaml
from library.python import func
from library.python import resource

from vins_core.utils.config import get_setting

logger = logging.getLogger(__name__)


def ensure_dir(path):
    try:
        os.makedirs(path)
    except OSError as e:
        if e.errno != errno.EEXIST or not os.path.isdir(path):
            raise


@func.memoize()
def vins_temp_dir():
    temp_dir = os.path.join(tempfile.gettempdir(), get_setting('TEMP_DIR_SUFFIX', 'vins'))
    if get_setting('UNIQUE_TMPDIR', False):
        temp_dir += '_' + str(uuid.uuid4())
    if not os.path.exists(temp_dir):
        logger.debug('Creating VINS tmp directory %s', temp_dir)
        ensure_dir(temp_dir)
    return temp_dir


def _res_full_path(path):
    return os.path.join('resfs/file', path)


def is_resource_exists(path):
    res_path = _res_full_path(path)
    return resource.find(res_path) is not None


def open_resource_file(path, encoding='utf-8'):
    resource_data = resource.find(_res_full_path(path))
    if not resource_data:
        raise RuntimeError('Resource {} is not found'.format(path))
    if encoding:
        return TextIOWrapper(BytesIO(resource_data), encoding=encoding)
    else:
        return BytesIO(resource_data)


def load_data_from_file(path, yaml_loader=None):
    _, ext = os.path.splitext(path)

    try:
        with open_resource_file(path) as f:
            if ext in ['.js', '.json']:
                return ujson.loads(f.read(), precise_float=True)
            elif ext in ['.yaml']:
                return yaml.load(f.read(), yaml_loader)
            else:
                raise ValueError('Unknown extension {}'.format(ext))

    except Exception:
        logger.error("Can't load data from file %s", path)
        raise


def delete_keys_from_dict(data, blacklist):
    """Remove keys from blacklist and return new dict.

    Blacklist key could be dot separated string like "a.b.c", where
    each part is a subkey in data.

    >>> delete_keys_from_dict({'a': {'b': 1, 'c': 2}}, ['a.b'])
    {'a': {'c': 2}}

    """

    if not isinstance(data, dict):
        raise TypeError('Type of data must be dict, not %s' % type(data).__name__)

    data = deepcopy(data)

    for key in blacklist:
        subdict = data
        parts = key.split('.')
        for part in parts[:-1]:
            if part in subdict and isinstance(subdict[part], dict):
                subdict = subdict[part]
            else:
                break
        else:
            subdict.pop(parts[-1], None)

    return data


def get_resource_full_path(relative_path):
    resource_path = get_setting('RESOURCES_PATH', default='/tmp/vins/sandbox')

    full_path = relative_path
    if relative_path.startswith('resource://'):
        full_path = os.path.join(resource_path, full_path.replace('resource://', ''))

    return full_path


def find_vinsfile(package_name, vinsfile='Vinsfile.json'):
    """
    Try find <vinsfile> at <package_name> and return full path to it,
    otherwise raise ValueError

    """
    return os.path.join(package_name, 'config/Vinsfile.json')


def list_resource_files_with_prefix(prefix):
    result = []
    for key, _ in resource.iteritems(os.path.join('resfs/file', prefix), strip_prefix=True):
        result.append(key.strip('/'))
    return result


def get_vinsfile_data(path=None, package=None):
    vinsfile = find_vinsfile(package)
    return load_data_from_file(vinsfile)


class Archive(object):
    __metaclass__ = abc.ABCMeta

    def __init__(self, **kwargs):
        self._tmp = []
        self._base = ''
        self._root = True

    @abc.abstractmethod
    def add(self, name, file_path):
        pass

    @abc.abstractmethod
    def get_by_name(self, name):
        pass

    @abc.abstractmethod
    def save_by_name(self, arch_name, filepath):
        pass

    @abc.abstractmethod
    def list(self):
        pass

    def close(self, error=None):
        if not self._root:
            return

        for filename in self._tmp:
            safe_remove(filename)

    def add_files(self, fnames):
        for fname in fnames:
            name = os.path.basename(fname)
            self.add(name, fname)

    def get_tmp_file(self, delete=True):
        fd, path = tempfile.mkstemp(dir=vins_temp_dir())
        os.close(fd)
        if delete:
            self._tmp.append(path)
        return path

    def get_tmp_dir(self, delete=True):
        path = tempfile.mkdtemp(dir=vins_temp_dir())
        if delete:
            self._tmp.append(path)
        return path

    def nested(self, name):
        clone = copy(self)
        clone._base = self._get_name(name) + '/'
        clone._root = False
        return clone

    def _get_name(self, name):
        return os.path.join(self._base, name)

    def __enter__(self):
        return self

    def __exit__(self, *exc):
        self.close(error=exc[1])

    @property
    def base(self):
        return self._base


class TarArchive(Archive):
    def __init__(self, path, mode='r:*', sha256=None, base_dir=None, **kwargs):
        super(TarArchive, self).__init__(**kwargs)

        if base_dir is not None:
            path = os.path.join(base_dir, path)

        if sha256 is not None:
            if not check_sum(path, sha256):
                raise RuntimeError(
                    "Model's %s archive checksum is invalid. Please rebuild model "
                    "and update Vinsfile" % path
                )

        self._read_mode = mode.startswith('r')
        self._tmp_path = self._path = path
        if not self._read_mode:
            self._tmp_path = path + '.tmp'

        self._tar = tarfile.open(name=self._tmp_path, mode=mode)

    def add(self, name, file_path):
        self._tar.add(file_path, self._get_name(name))

    def get_by_name(self, name):
        return self._tar.extractfile(self._get_name(name))

    def save_by_name(self, name, filepath):
        self._tar.extract(self._get_name(name), filepath)

    def extract_all(self, path, dirname=None):
        members = None
        if dirname:
            dirname = dirname if dirname.endswith('/') else dirname + '/'
            members = [
                tarinfo for tarinfo in self._tar.getmembers()
                if tarinfo.name.startswith(dirname)
            ]
        self._tar.extractall(path, members=members)

    def _list_all(self):
        base = self._base
        for name in self._tar.getnames():
            if os.path.commonprefix([name, base]) == base:
                yield name[len(base):]

    def list(self):
        def dirname(d):
            idx = d.find('/')
            if idx >= 0:
                return d[:idx]
            else:
                return d

        return list({dirname(name) for name in self._list_all()})

    def close(self, error=None):
        super(TarArchive, self).close()
        if self._root:
            self._tar.close()

            if not self._read_mode and error is None:
                shutil.move(self._tmp_path, self._path)

    @property
    def path(self):
        return self._path


class WebArchive(TarArchive):
    _api = None

    def __init__(self, filename, mode='r:*', retries=3, *args, **kwargs):
        self._api = self._get_client()
        self._filename = urlparse(unicode(filename)).path.lstrip('/')
        self._read_mode = mode.startswith('r')
        self._retries = retries

        path = self.get_tmp_file(delete=False)

        if self._read_mode:
            self._download_retry(self._filename, path)

        super(WebArchive, self).__init__(path, mode, *args, **kwargs)

    def _download_retry(self, filename, path):
        for attempt in xrange(self._retries):
            try:
                self._download(filename, path)
            except Exception:
                logger.exception("Can't download archive %s", filename)
                safe_remove(path)
                continue
            else:
                return

        raise RuntimeError("Can't download archive %s after %s attempts" % (
            filename, self._retries
        ))

    def _get_client(self):
        raise NotImplementedError

    def _download(self, filename, path):
        raise NotImplementedError

    def _upload(self, filename):
        raise NotImplementedError

    def close(self, error=None):
        super(WebArchive, self).close(error=error)

        if self._root:
            try:
                if not error and not self._read_mode:
                    self._upload(self._filename)
            finally:
                safe_remove(self._path)


class S3Archive(WebArchive):
    @classmethod
    def _get_client(cls):
        # avoiding circular dependency
        from vins_core.ext import s3

        if cls._api is None:
            cls._api = s3.S3Bucket(get_setting('S3_ACCESS_KEY_ID'), get_setting('S3_SECRET_ACCESS_KEY'))

        return cls._api

    def _download(self, filename, path):
        self._api.download_file(filename, path)

    def _upload(self, filename):
        self._api.put(filename, open(self._path, 'rb'))

    @property
    def path(self):
        return self._api.get_url(self._filename)


def _get_file_checksum(file_):
    blocksize = 65536
    hasher = hashlib.sha256()

    with open(file_, 'rb') as f:
        for buf in iter(lambda: f.read(blocksize), b''):
            hasher.update(buf)

    return hasher.hexdigest()


def get_checksum(file_or_dir):
    file_hashes = []
    if os.path.isdir(file_or_dir):
        for root, _, files in os.walk(file_or_dir):
            for filename in files:
                full_path = os.path.join(root, filename)
                rel_path = os.path.relpath(full_path, file_or_dir)
                file_hashes.append((rel_path, _get_file_checksum(full_path)))
    else:
        file_hashes.append(('.', _get_file_checksum(file_or_dir)))

    hasher = hashlib.sha256()
    for filename, hashsum in sorted(file_hashes):
        hasher.update(filename)
        hasher.update(hashsum)
    return hasher.hexdigest()


def check_sum(file_or_dir, checksum):
    return get_checksum(file_or_dir) == checksum


def validate_json(dict_obj, schema):
    if isinstance(schema, (jsonschema.Draft3Validator, jsonschema.Draft4Validator)):
        schema.validate(dict_obj)
    elif isinstance(schema, dict):
        jsonschema.validate(dict_obj, schema)
    else:
        raise TypeError('schema must be instance of Draft3Validator, Draft4Validator or dict')


class ResfsSchemaResolver(jsonschema.RefResolver):
    def resolve_from_url(self, uri):
        resname = uri
        index = uri.find('#/')
        if index >= 0:
            resname = uri[0:index]
        if resname.startswith('file://'):
            resname = resname[7:]
        res = load_data_from_file(resname)
        if index >= 0:
            parts = uri[index+2:].split('/')
            for part in parts:
                res = res[part]
        return res


def load_jsonschema_from_file(filepath):
    schema = load_data_from_file(filepath)
    jsonschema_validator = jsonschema.Draft4Validator(
        schema,
        resolver=ResfsSchemaResolver('file://%s' % filepath, schema))
    return jsonschema_validator


def safe_remove(path):
    try:
        if os.path.isdir(path):
            shutil.rmtree(path, ignore_errors=True)
        else:
            os.remove(path)
    except OSError:
        logger.warning("Can't delete '%s'", path, exc_info=True)


def make_safe_filename(s):
    def safe_char(c):
        if c.isalnum():
            return c
        else:
            return '_'
    return ''.join(safe_char(c) for c in s).rstrip('_')


class JSONEncoder(json.JSONEncoder):
    def default(self, obj):
        if isinstance(obj, uuid.UUID):
            # if the obj is uuid, we simply return the value of uuid
            return obj.hex
        if hasattr(obj, 'to_dict'):
            return json.JSONEncoder.encode(self, obj.to_dict())
        return json.JSONEncoder.default(self, obj)


def to_json_str(obj, *args, **kwargs):
    return json.dumps(obj, cls=JSONEncoder, *args, **kwargs)


def uuid_to_str(v):
    return '__uuid:' + str(v) if isinstance(v, uuid.UUID) else v


def uuid_from_str(v):
    return uuid.UUID(v[len('__uuid:'):]) if isinstance(v, basestring) and v.startswith('__uuid') else v


def remove_file_or_dir(path):
    if not os.path.exists(path):
        return
    if os.path.isfile(path):
        os.chmod(path, stat.S_IRWXU | stat.S_IRWXG | stat.S_IRWXO)
        os.remove(path)
    elif os.path.islink(path):
        os.unlink(path)
    elif os.path.isdir(path):
        if len(os.listdir(path)) > 0:
            clean_dir(path)
        shutil.rmtree(path)


def clean_dir(location):
    """ This is like rmdir, but handles symlinks as well """
    filenames = os.listdir(location)
    for filename in filenames:
        remove_file_or_dir(os.path.join(location, filename))


class DirectoryView(Archive):
    def __init__(self, path, sha256=None, **kwargs):
        super(DirectoryView, self).__init__(**kwargs)
        self._base_dir = path

        if sha256 is not None:
            if not check_sum(path, sha256):
                raise RuntimeError(
                    "Model's %s archive checksum is invalid. Please rebuild model "
                    "and update Vinsfile" % path
                )

    def _full_name(self, name):
        return os.path.join(self._base_dir, name)

    def add(self, name, path):
        full_path = self._full_name(self._get_name(name))
        basedir = os.path.dirname(full_path)
        if os.path.exists(basedir) and not os.path.isdir(basedir):
            os.remove(basedir)
        if not os.path.exists(basedir):
            os.makedirs(basedir)
        if os.path.exists(full_path):
            if os.path.islink(full_path):
                os.unlink(full_path)
            else:
                remove_file_or_dir(full_path)
        temp_dir = os.path.join(vins_temp_dir(), str(uuid.uuid4()))
        temp_path = os.path.join(temp_dir, name)
        temp_basedir = os.path.dirname(temp_path)
        if not os.path.exists(temp_basedir):
            os.makedirs(temp_basedir)
        if os.path.isdir(path):
            shutil.copytree(path, temp_path)
        else:
            shutil.copy(path, temp_path)
        os.symlink(temp_path, full_path)

    def get_by_name(self, name):
        return open(self._full_name(self._get_name(name)), 'r')

    def save_by_name(self, arch_name, filepath):
        try:
            full_path = os.path.join(filepath, self._get_name(arch_name))
            basedir = os.path.dirname(full_path)
            if not os.path.exists(basedir):
                os.makedirs(basedir)
            shutil.copy(self._full_name(self._get_name(arch_name)), os.path.join(filepath, self._get_name(arch_name)))
        except Exception:
            raise RuntimeError("Failed to copy from {} to {}".format(
                self._full_name(self._get_name(arch_name)), os.path.join(filepath, self._get_name(arch_name)))
            )

    def list(self):
        fulldir = os.path.join(self._base_dir, self._base)
        if not os.path.exists(fulldir):
            return []
        return os.listdir(fulldir)

    def extract_all(self, path, dirname=None):
        for subdir in self.list() if not dirname else [dirname]:
            src_path = self._full_name(subdir)
            dst_path = os.path.join(path, subdir)
            if src_path != dst_path:
                shutil.copytree(src=src_path, dst=dst_path)

    @property
    def path(self):
        return self._base_dir
