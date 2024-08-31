# coding: utf-8
import json
import logging
import os
import shutil
from functools import partial
from collections import defaultdict

import click
import fasteners.process_lock as locks

from vins_core.utils.data import TarArchive, safe_remove
from vins_core.utils.config import get_setting

from vins_tools.nlu.ner.ru.fst_calc import NluFstCalcRuConstructor
from vins_tools.nlu.ner.fst_custom import NluFstCustomConstructor
from vins_tools.nlu.ner.ru.fst_date import NluFstDateRuConstructor
from vins_tools.nlu.ner.ru.fst_datetime_range import NluFstDatetimeRangeRuConstructor
from vins_tools.nlu.ner.ru.fst_datetime import NluFstDatetimeRuConstructor
from vins_tools.nlu.ner.ru.fst_fio import NluFstFioRuConstructor
from vins_tools.nlu.ner.ru.fst_float import NluFstFloatRuConstructor
from vins_tools.nlu.ner.ru.fst_geo import NluFstGeoRuConstructor
from vins_tools.nlu.ner.ru.fst_num import NluFstNumRuConstructor
from vins_tools.nlu.ner.ru.fst_time import NluFstTimeRuConstructor
from vins_tools.nlu.ner.ru.fst_units import NluFstUnitsTimeRuConstructor
from vins_tools.nlu.ner.ru.fst_weekdays import NluFstWeekdaysRuConstructor

from vins_tools.nlu.ner.tr.fst_num import NluFstNumTrConstructor
from vins_tools.nlu.ner.tr.fst_datetime import NluFstDatetimeTrConstructor

logger = logging.getLogger(__name__)


def custom_constructor(cls, data_path):
    def _custom_constructor(fst_name):
        with open(data_path) as fi:
            return cls(data=json.load(fi), fst_name=fst_name)

    return _custom_constructor

_data_path_ru = '../vins_tools/nlu/ner/ru/data'


class SandboxFstConstructor(object):
    def __init__(self, fst_name, lang, resource):
        self.fst_name = fst_name
        self.lang = lang
        self.resource = resource
        self.resource_path = None

    def compile(self):
        resources_path = get_setting('RESOURCES_PATH', default='/tmp/vins/sandbox')
        self.resource_path = os.path.join(resources_path, self.resource)
        self.save_file_from_sandbox()
        return self

    def save(self, directory):
        if self.resource_path is None:
            raise ValueError('Trying to save not compiled entity %s' % self.fst_name)
        with TarArchive(self.resource_path) as archive:
            tmp_dir = archive.get_tmp_dir()
            archive.extract_all(tmp_dir, os.path.join('fst', self.lang, self.fst_name))
            if not os.path.isdir(directory):
                os.makedirs(directory)
            for fname in os.listdir(os.path.join(tmp_dir, 'fst', self.lang, self.fst_name)):
                shutil.move(os.path.join(tmp_dir, 'fst', self.lang, self.fst_name, fname), directory)

    def save_to_archive(self, archive):
        with archive.nested(self.fst_name) as nested:
            tmpdir = nested.get_tmp_dir()
            self.save(tmpdir)
            files = os.listdir(tmpdir)
            nested.add_files([os.path.join(tmpdir, fname) for fname in files])

    def save_file_from_sandbox(self, check_md5=True):
        # avoiding circular dependencies
        from vins_core.ext.sandbox import SandboxApi

        def download_func(res, path):
            with open(path, 'wb') as f:
                return SandboxApi().download_resource(res, f, check_md5=check_md5)

        resource_filename_lock = self.resource_path + '.lock'
        resource_dir = os.path.dirname(self.resource_path)

        # avoid concurrent read while resource downloading
        with locks.InterProcessLock(resource_filename_lock):
            if not os.path.exists(resource_dir):
                logging.debug('Creating tmp directory for sandbox cache %s', resource_dir)
                os.makedirs(resource_dir)

            if not os.path.exists(self.resource_path):
                logging.info('Downloading sandbox resource %s', self.resource)
                try:
                    download_func(self.resource, self.resource_path)
                except Exception:
                    logger.info('Removing broken file %s', self.resource_path)
                    safe_remove(self.resource_path)
                    raise

                logging.info('Resource has been downloaded %s', self.resource_path)
            else:
                logging.info('Resource %s already exists on disk, skipping.', self.resource)


def sandbox_fst_constructor(lang, resource):
    return partial(SandboxFstConstructor, lang=lang, resource=resource)


ALL_CONSTRUCTORS = ['units_time', 'datetime', 'date', 'time', 'geo', 'num', 'fio', 'datetime_range',
                    'float', 'calc', 'weekdays', 'poi_category_ru', 'currency', 'soft', 'site',
                    'album', 'artist', 'artist', 'track', 'films_100_750', 'films_50_filtered', 'swear']


FST_CONSTRUCTORS = {
    'ru': {
        'units_time': NluFstUnitsTimeRuConstructor,
        'datetime': NluFstDatetimeRuConstructor,
        'date': NluFstDateRuConstructor,
        'time': NluFstTimeRuConstructor,
        'geo': NluFstGeoRuConstructor,
        'num': NluFstNumRuConstructor,
        'fio': NluFstFioRuConstructor,
        'datetime_range': NluFstDatetimeRangeRuConstructor,
        'float': NluFstFloatRuConstructor,
        'calc': NluFstCalcRuConstructor,
        'weekdays': NluFstWeekdaysRuConstructor,

        'poi_category_ru': custom_constructor(
            NluFstCustomConstructor,
            os.path.join(_data_path_ru, 'poi_category_ru', 'poi_categories.json')
        ),
        'currency': custom_constructor(
            NluFstCustomConstructor,
            os.path.join(_data_path_ru, 'currency', 'currency.json')
        ),
        'soft': custom_constructor(
            NluFstCustomConstructor,
            os.path.join(_data_path_ru, 'soft', 'soft.json')
        ),
        'site': custom_constructor(
            NluFstCustomConstructor,
            os.path.join(_data_path_ru, 'site', 'site.json')
        ),

        # load from sandbox
        'album': sandbox_fst_constructor('ru', '731963354'),
        'artist': sandbox_fst_constructor('ru', '731963354'),
        'track': sandbox_fst_constructor('ru', '731963354'),
        'films_100_750': sandbox_fst_constructor('ru', '731963354'),
        'films_50_filtered': sandbox_fst_constructor('ru', '731963354'),
        'swear': sandbox_fst_constructor('ru', '731963354'),
    },
    'tr': {
        'num': NluFstNumTrConstructor,
        'datetime': NluFstDatetimeTrConstructor,
    }

}


@click.command('do_main')
@click.option('-o', '--output', required=True, type=click.Path())
@click.option('-a', '--archive', default=False, is_flag=True)
@click.option('-r', '--from-resource', required=False)
@click.option('-l', '--lang', required=False)
@click.argument('constructors', nargs=-1, required=False, type=click.Choice(ALL_CONSTRUCTORS))
def do_main(constructors, output, archive, from_resource, lang):
    logging.basicConfig(level=logging.INFO)

    langs = [lang] if lang else FST_CONSTRUCTORS.keys()

    if not constructors:
        constructors = ALL_CONSTRUCTORS

    fst_constructors = defaultdict(list)
    added_constructors = set()
    for lang in langs:
        fst_constructors[lang] = []
        for name in constructors:
            if name in FST_CONSTRUCTORS[lang]:
                fst_constructors[lang].append((name, FST_CONSTRUCTORS[lang][name]))
                added_constructors.add((lang, name))
    if from_resource:
        for lang, constructor_list in FST_CONSTRUCTORS.iteritems():
            for name in constructor_list:
                if (lang, name) not in added_constructors:
                    fst_constructors[lang].append((name, sandbox_fst_constructor(lang, from_resource)))

    fsts = defaultdict(list)

    for lang in fst_constructors.iterkeys():
        for name, constructor in fst_constructors[lang]:
            if (lang, name) not in added_constructors:
                logger.info('Getting %s (%s) from resource %s', name, lang, from_resource)
            else:
                logger.info('Compiling %s (%s)', name, lang)
            fsts[lang].append(constructor(fst_name=name).compile())

    if archive:
        with TarArchive(output, mode='w') as arch:
            with arch.nested('fst') as nested:
                for lang, lang_fsts in fsts.iteritems():
                    with nested.nested(lang) as lang_nested:
                        for fst in lang_fsts:
                            fst.save_to_archive(lang_nested)
        logger.info('Saved to archive %s', output)
    else:
        for lang, lang_fsts in fsts.iteritems():
            for fst in lang_fsts:
                fst.save(os.path.join(output, lang, fst.fst_name))
        logger.info('Saved to directory %s', output)


if __name__ == '__main__':
    do_main()
