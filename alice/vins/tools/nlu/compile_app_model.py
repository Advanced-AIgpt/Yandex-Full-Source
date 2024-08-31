# coding: utf-8

import argparse
import logging
import time
import os
import sys
import attr
import shutil
import re

from functools import partial

from vins_core.nlu.custom_entities_tools import CUSTOM_ENTITIES_DIRNAME
from vins_core.utils import archives
from vins_core.utils.data import get_resource_full_path, load_data_from_file
from vins_core.app_utils import compile_nlu
from vins_core.nlu.neural.metric_learning.metric_learning import TrainMode

from vins_tools.nlu.ner.compile_custom_entities import compile_custom_entities_for_app, save_parsers_to_archive

from vins_tools.nlu.inspection.nlu_processing_on_dataset import APP_ALIASES, create_app, get_vinsfile_for_app


logger = logging.getLogger(__name__)

CLASSIFIERS_DIRNAME = 'classifiers'
TAGGERS_DIRNAME = 'tagger'


@attr.s
class SaveResult(object):
    app_name = attr.ib()
    archive = attr.ib()
    vinsfile_data = attr.ib()
    total_time = attr.ib()


def main():
    parser = argparse.ArgumentParser(add_help=True)
    parser.add_argument(
        '--app', metavar='APP', dest='apps', action='append',
        help="The name of the app to compile models for, multiple names can be specified.",
    )
    parser.add_argument(
        '--fst-only', action='store_true',
        help="Only update custom entities in the model file",
    )
    parser.add_argument(
        '--features-only', action='store_true',
        help='Runs only features extraction step',
    )
    parser.add_argument(
        '--classifiers', nargs='+', dest='classifiers',
        help='Specify the list of classifiers to update'
    )
    parser.add_argument(
        '--taggers', action='store_true',
        help='Whether to retrain taggers'
    )
    parser.add_argument(
        '--skip-train', action='store_true',
        help='If set, skip training classifiers & taggers'
    )
    parser.add_argument(
        '--all', action='store_true', default=False,
        help="Compile all apps. (%s)" % ', '.join(APP_ALIASES.keys()),
    )
    parser.add_argument(
        '--save-dir', dest='save_dir',
        help='Specifies path to directory to save models and configs (app directory by default)', default=''
    )
    parser.add_argument(
        '--feature-cache', dest='feature_cache',
        help='File path to store / retrieve precomputed train features'
    )
    parser.add_argument(
        '--custom-intents', dest='custom_intents', nargs='+',
        help='Filepath with VinsProject-like formatted custom intents'
    )
    parser.add_argument(
        '--metric-learning', dest='metric_learning', default=TrainMode.NO_METRIC_LEARNING,
        choices=list(TrainMode), type=lambda mode: TrainMode[mode],
        help='Modes for metric learning'
    )
    parser.add_argument(
        '--metric-learning-logdir', dest='metric_learning_logdir', metavar='METRIC_LEARNING_LOGDIR',
        help='If metric model is presented, specify directory to store TFEvents. You can launch'
             '`tensorboard --logdir METRIC_LEARNING_LOGDIR` to investigate training details'
    )
    parser.add_argument(
        '--load-custom-entities', dest='load_custom_entities', action='store_true', default=False,
        help='Use this flag for loading custom entities from archive'
    )
    parser.add_argument(
        '--intents-to-train-tagger', dest='intents_to_train_tagger',
        help='Specify regexp to filter intents for which we should train taggers'
    )
    parser.add_argument(
        '--intents-to-train-classifiers', dest='intents_to_train_classifiers',
        help='Specify regexp to filter intents for which we should train classifiers'
    )
    parser.add_argument(
        '--keep-vinsfile-projects', dest='keep_vinsfile_projects', action='store_true',
        help='If custom intents are specified this option will keep the projects from Vinsfile.json'
    )
    parser.add_argument(
        '--filter-features', dest='filter_features', action='store_true',
        help='Will collect only features for intents-to-train-something'
    )
    args = parser.parse_args()

    if args.all:
        apps = APP_ALIASES
    else:
        apps = args.apps

    if apps is None:
        parser.print_help()
        return 1

    logging.basicConfig(level=logging.DEBUG, format='[%(asctime)s] [%(name)s] [%(levelname)s] %(message)s')

    train_single_model(apps, args)
    return 0


def train_single_model(apps, args):
    res = map(partial(
        save_model, only_entities=args.fst_only, save_dir=args.save_dir,
        classifiers=args.classifiers, taggers=args.taggers,
        feature_cache=args.feature_cache, custom_intents=args.custom_intents,
        metric_learning=args.metric_learning, metric_learning_logdir=args.metric_learning_logdir,
        load_custom_entities=args.load_custom_entities, features_only=args.features_only,
        intents_to_train_tagger=args.intents_to_train_tagger,
        intents_to_train_classifiers=args.intents_to_train_classifiers,
        keep_vinsfile_projects=args.keep_vinsfile_projects,
        skip_train=args.skip_train,
        fall_on_update_error=True,
        filter_features=args.filter_features
    ), apps
    )
    for item in res:
        print "Model %s: %.1fs" % (item.app_name, item.total_time)


def copy_between_archives(archive_from, archive_to, path):
    logger.info('copying {} from {} to {}'.format(path, archive_from, archive_to))
    path_from = os.path.join(archive_from, path)
    path_to = os.path.join(archive_to, path)
    if not os.path.exists(path_from):
        return
    if not os.path.exists(path_to):
        return
    if os.path.islink(path_to):
        os.remove(path_to)
    elif os.path.isdir(path_to):
        shutil.rmtree(path_to)
    elif os.path.isfile(path_to):
        os.remove(path_to)
    shutil.copytree(path_from, path_to)
    logger.info('Copied OK')


def _compile_app(app_name, vins_file, archive, feature_cache, compiled_model_cfg=None, load_custom_entities=False,
                 only_entities=False, **kwargs):
    app = create_app(app_name, vins_file, check_feature_cache=feature_cache, **kwargs)
    if not load_custom_entities:
        parsers = compile_custom_entities_for_app(app_name)
        with archive.nested(CUSTOM_ENTITIES_DIRNAME) as nested:
            save_parsers_to_archive(parsers, nested)

        # save custom entities to tmp arch
        tmp_dir = archive.get_tmp_dir()
        tmp_arch_name = os.path.join(tmp_dir, 'compiled_entities.tar')
        with archives.TarArchive(tmp_arch_name, mode='w') as tmp_arch:
            with tmp_arch.nested(CUSTOM_ENTITIES_DIRNAME) as nested:
                save_parsers_to_archive(parsers, nested)
        with archives.TarArchive(tmp_arch_name, mode='r') as tmp_arch:
            with tmp_arch.nested(CUSTOM_ENTITIES_DIRNAME) as nested:
                app.nlu.update_custom_entities(nested)
    # copy the old archive contents (assuming it's a DirectoryView) to a tmp folder, to preserve some components
    old_archive_directory = archive.get_tmp_dir()
    new_archive_directory = archive.path
    if os.path.exists(old_archive_directory):
        shutil.rmtree(old_archive_directory)
    shutil.copytree(new_archive_directory, old_archive_directory)

    if only_entities:
        app.nlu.save(archive)
    else:
        compile_nlu(app.nlu, archive, feature_cache=feature_cache, skip_train=False, **kwargs)

    classifiers_dir = os.path.join(old_archive_directory, CLASSIFIERS_DIRNAME)
    if os.path.exists(classifiers_dir) and (only_entities or kwargs.get('classifiers') is not None):
        for classifier in os.listdir(classifiers_dir):
            classifier_path = os.path.join(CLASSIFIERS_DIRNAME, classifier)
            if only_entities or classifier not in kwargs.get('classifiers'):
                copy_between_archives(old_archive_directory, new_archive_directory, classifier_path)

    taggers_dir = os.path.join(old_archive_directory, TAGGERS_DIRNAME)
    if os.path.exists(taggers_dir):
        if only_entities or not kwargs.get('taggers'):
            copy_between_archives(old_archive_directory, new_archive_directory, TAGGERS_DIRNAME)
        elif kwargs.get('intents_to_train_tagger'):
            old_tagger_basedir = os.path.join(old_archive_directory, TAGGERS_DIRNAME, 'tagger.data')
            new_tagger_basedir = os.path.join(new_archive_directory, TAGGERS_DIRNAME, 'tagger.data')
            for intent_folder in os.listdir(old_tagger_basedir):
                if not re.match(kwargs['intents_to_train_tagger'], intent_folder):
                    copy_between_archives(old_tagger_basedir, new_tagger_basedir, intent_folder)


def _compile_features(app_name, vins_file, classifiers, taggers, feature_cache, **kwargs):
    app = create_app(app_name, vins_file, check_feature_cache=feature_cache, **kwargs)
    app.nlu.extract_features_on_train(
        classifiers=classifiers,
        taggers=taggers,
        feature_cache=feature_cache,
        validation=None
    )


def save_model(
    app_name, vinsfile=None, only_entities=False,
    features_only=False, skip_train=False, **kwargs
):
    vinsfile, vinsfile_res = get_vinsfile_for_app(app_name)
    vinsfile_data = load_data_from_file(vinsfile)

    archiver = getattr(archives, vinsfile_data['nlu']['compiled_model']['archive'])
    model_dir = get_resource_full_path(vinsfile_data['nlu']['compiled_model']['path'])
    start = time.time()
    if not skip_train:
        if features_only:
            _compile_features(app_name, vinsfile_res, **kwargs)
        else:
            with archiver(model_dir, mode='w') as archive:
                _compile_app(
                    app_name, vinsfile_res, archive,
                    compiled_model_cfg=vinsfile_data['nlu']['compiled_model'],
                    only_entities=only_entities,
                    **kwargs
                )

    tot_time = time.time() - start
    return SaveResult(app_name, model_dir, vinsfile_data, tot_time)


def do_main():
    import tensorflow as tf
    tf.logging.set_verbosity(tf.logging.ERROR)

    # take external_skill custom entity from platform beta until the official launch
    os.environ['VINS_SKILLS_ENV_TYPE'] = 'beta'

    # feature extractor uses tensorflow loaders, therefore lazy initialization is needed  to avoid hunging problem
    os.environ['VINS_LOAD_TF_ON_CALL'] = '1'

    # misspell settings for model training
    os.environ['VINS_MISSPELL_TIMEOUT'] = '0.5'

    logging.getLogger('requests').setLevel(logging.INFO)
    logging.getLogger('urllib3.connectionpool').setLevel(logging.INFO)
    logging.getLogger('yt').setLevel(logging.INFO)
    logging.getLogger('vins_core.utils.decorators').setLevel(logging.INFO)
    logging.getLogger('vins_core.utils.metrics').setLevel(logging.WARNING)
    logging.getLogger('vins_core.utils.lemmer').setLevel(logging.WARNING)
    logging.getLogger('vins_core.ext.base').setLevel(logging.WARNING)
    logging.getLogger('vins_core.ner.wizard').setLevel(logging.ERROR)
    logging.getLogger('vins_core.nlu.features.extractor.wizard').setLevel(logging.ERROR)

    sys.exit(main())


if __name__ == '__main__':
    do_main()
