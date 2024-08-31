#!/usr/bin/env python
# encoding: utf-8
import uuid
import sys
from os.path import join, realpath, commonprefix, relpath, splitext, dirname, split
from os import listdir
from collections import namedtuple


PRJ_ROOT = realpath(join(__file__, '../../..'))


def get_util_dep():
    from nile.api.v1.files import DevelopPackage
    return DevelopPackage(PRJ_ROOT)


def list_deps(neighbours_for=None, neighbour_names=None, include_utils=False):
    """
    Вычисление списка зависимостей для nile-джоба
    :param str|None neighbours_for: Путь к файлу, рядом с которым нужно искать модули
        Обычным методом является передача сюда переменной __file__ из запускаемого скрипта
    :param list[str]|None neighbour_names: Названия соседних файлов.
        Если не указан, то будут включены все соседние "*.py"
    :param bool include_utils: Включать ли в зависимости пакет utils
    :rtype: list[nile.api.v1.files.ArchivableFile]
    """
    from nile.api.v1.files import LocalFile

    deps = []
    if include_utils:
        deps.append(get_util_dep())

    if neighbours_for:
        neighbours_for = realpath(neighbours_for)
        folder, script = split(neighbours_for)
        if neighbour_names is None:
            for fn in listdir(folder):
                if fn.endswith('.py') and fn != script:
                    deps.append(LocalFile(join(folder, fn)))
        else:
            for fn in neighbour_names:
                deps.append(LocalFile(join(folder, fn)))
    return deps


def hahn_with_deps(pool=None,
                   use_yql=False,
                   templates=None,
                   neighbours_for=None,
                   neighbour_names=None,
                   custom_cluster_params=None,
                   custom_env_params=None,
                   include_utils=False):
    """
    Создание nile-прокси для hahn, с добавлением в неё модулей, требуемых для работы джоба.
    Описание переменных см. в list_deps
    :param str|None pool: вычислительный пул в YT
    :param bool use_yql: использовать ли nile-over-yql? По-умолчанию — nile-over-yt
    :param dict|None templates: шаблоны подстановки nile
    :rtype: nile.api.v1.clusters.yt.YT
    """
    from nile.api.v1 import clusters

    cluster_params = dict(pool=pool)
    if custom_cluster_params:
        cluster_params.update(custom_cluster_params)

    if use_yql:
        cluster = clusters.yql.Hahn(**cluster_params)
    else:
        cluster = clusters.yt.Hahn(**cluster_params)

    env_params = {
        'files': list_deps(neighbours_for=neighbours_for,
                           neighbour_names=neighbour_names,
                           include_utils=include_utils),
        'templates': dict(
            tmp_root='tmp/nile-{}'.format(uuid.uuid4()),
        )
    }
    if templates:
        env_params['templates'] = templates
    if custom_env_params:
        env_params.update(custom_env_params)
    return cluster.env(**env_params)


# Автоматический поиск зависимостей (пока не используется)

def is_subpath(path, root):
    return commonprefix([path, root]) == root


Module = namedtuple('Module', ['name', 'full_path', 'relative_path'])


def modules_in_roots(roots):
    """
    :param list[str] roots: Корневые пути модулей. Будут выбраны только модули находящиеся внутри этих путей
    :rtype: Iterator[Module]
    """
    for name, module in sys.modules.items():
        # Не iteritems - чтобы избежать гонок при выполнении в режиме джоба
        if name == '__main__':
            continue

        try:
            fn = module.__file__
        except AttributeError:  # Некоторые модули живут только в памяти
            continue

        for root in roots:
            if is_subpath(fn, root):
                base, ext = splitext(fn)
                if ext == '.pyc':
                    fn = base + '.py'
                yield Module(name=name,
                             full_path=fn,
                             relative_path=relpath(fn, root))
                break


def list_py_deps(main_path):
    """
    Вычисляет список зависимостей, исходя из импортированных на данный момент модулей
    :param str main_path: Путь к основному выполняющемуся скрипту
    :return:
    """
    from nile.api.v1.files import LocalFile

    cur_dir = dirname(realpath(main_path))
    return [LocalFile(path=m.full_path,
                      filename=m.relative_path)
            for m in modules_in_roots([cur_dir, PRJ_ROOT])]
