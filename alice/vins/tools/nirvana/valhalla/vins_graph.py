# coding: utf-8

from urlparse import urljoin

import click

import vh

from alice.vins.tools.nirvana.valhalla import op
from alice.vins.tools.nirvana.valhalla import vh_arcadia_build
from alice.vins.tools.nirvana.valhalla import yql
from alice.vins.tools.nirvana.valhalla.global_options import vins_global_options


VINS_HAMSTER_URL = 'http://vins.hamster.alice.yandex.net'
LIGHT_VINS_HAMSTER_URL = 'http://vins-qa.hamster.alice.yandex.net'
FEATURES_URI = '/qa/pa/features/'
LIGHT_FEATURES_URI = '/qa/pa/features_light/'

DEFAULT_HW_PARAMS_KWARGS = {
    'cpu_guarantee': 1,
    'max_ram': 100 * 1024,  # 100 Gb
    'max_disk': 100 * 1024,
}
DEFAULT_HW_PARAMS = {k.replace('_', '-'): v for k, v in DEFAULT_HW_PARAMS_KWARGS.items()}


def get_vins_mr_table_with_dataset(dataset_yt_path, resources, revision, patch, intents_to_train_classifiers,
                                   intents_to_train_tagger, yt_cluster):
    if dataset_yt_path:
        mr_table_with_dataset = op.GET_MR_TABLE_OP(
            _options={
                'cluster': yt_cluster,  # FIXME: fix working with global options
                'table': dataset_yt_path,
            }
        )
    else:
        dataset_dumper = vh_arcadia_build.build_arcadia_binary(
            'alice/vins/tools/nirvana/dataset_dumper',
            'alice/vins/tools/nirvana/dataset_dumper/dataset_dumper',
            revision,
            patch,
        )

        opts = {
            'yt-token': vins_global_options.yt_token,
            'mr-account': vins_global_options.mr_account,
            'mr-output-ttl': vins_global_options.mr_output_ttl,
            'intents-to-train-classifiers': intents_to_train_classifiers,
            'intents-to-train-tagger': intents_to_train_tagger,
        }
        opts.update(DEFAULT_HW_PARAMS)
        mr_table_with_dataset = op.DUMP_DATASET_OP(
            _inputs={
                'dataset_dumper': dataset_dumper,
                'archive_with_resources': resources,
            },
            _options=opts,
        )
    splitted_words_mr_table_with_dataset = op.YQL_2_OP(
        _inputs={
            'input1': mr_table_with_dataset
        },
        _options={
            'request': yql.SPLIT_WORDS,
            'mr-account': vins_global_options.mr_account,
            'mr-output-ttl': vins_global_options.mr_output_ttl,
            'use_account_tmp': True,
            'yt-pool': vins_global_options.yt_pool,
            'yt-token': vins_global_options.yt_token,
            'yql-token': vins_global_options.yql_token,
            'syntax_version': 'v1',
        },
        _name='Split words',
    )['output1']
    normalized_mr_table_with_dataset = op.NORMALIZE_OP(
        _inputs={
            'input': splitted_words_mr_table_with_dataset,
        },
        _options={
            'yt-token': vins_global_options.yt_token,
            'mr-account': vins_global_options.mr_account,
            'mr-output-ttl': vins_global_options.mr_output_ttl,
        },
    )
    joined_mr_table_with_dataset = op.YQL_2_OP(
        _inputs={
            'input1': normalized_mr_table_with_dataset,
            'input2': mr_table_with_dataset
        },
        _options={
            'request': yql.JOIN_WORDS,
            'mr-account': vins_global_options.mr_account,
            'mr-output-ttl': vins_global_options.mr_output_ttl,
            'use_account_tmp': True,
            'yt-pool': vins_global_options.yt_pool,
            'yt-token': vins_global_options.yt_token,
            'yql-token': vins_global_options.yql_token,
            'syntax_version': 'v1',
        },
        _name='Join words',
    )['output1']
    return joined_mr_table_with_dataset


def load_features_to_dataset(mr_table_with_dataset, vins_url, heavy, yt_cluster):
    if vins_url is None:
        vins_url = VINS_HAMSTER_URL if heavy else LIGHT_VINS_HAMSTER_URL
    vins_url = urljoin(vins_url, FEATURES_URI) if heavy else urljoin(vins_url, LIGHT_FEATURES_URI)
    mr_table_with_responses = op.DOWNLOADER_OP(
        _inputs={'input': mr_table_with_dataset},
        _options={
            'vins_url': vins_url,
            'oauth': vins_global_options.yaplus_token,
            'yt_token': vins_global_options.yt_token,
            'yt-pool': vins_global_options.yt_pool,
            # FIXME: fix working with global options
            'input_cluster': yt_cluster,
            'ScraperOverYtPool': vins_global_options.scraper_pool,
            'mr-account': vins_global_options.mr_account,
            'mr-output-ttl': vins_global_options.mr_output_ttl,
            'MaxRPS': 300,
            'use_account_tmp': True,
            'scraper-timeout': '48h',
        },
    )
    return op.YQL_2_OP(
        _inputs={
            'input1': mr_table_with_dataset,
            'input2': mr_table_with_responses['output'],
        },
        _options={
            'request': yql.MERGE_REQUEST_RESULT,
            'mr-account': vins_global_options.mr_account,
            'mr-output-ttl': vins_global_options.mr_output_ttl,
            'use_account_tmp': True,
            'yt-pool': vins_global_options.yt_pool,
            'yt-token': vins_global_options.yt_token,
            'yql-token': vins_global_options.yql_token,
        },
    )['output1']


def get_hardware_params(cpu=False, metric_learning=False, fst=False):
    hw_params = {}
    hw_params.update(DEFAULT_HW_PARAMS_KWARGS)
    if not cpu:
        hw_params.update({
            'gpu_type': vh.GPUType.CUDA_5_2,
            'gpu_count': 1,
            'gpu_max_ram': 10 * 1024,
        })
    if metric_learning:
        hw_params.update({
            'max_ram': 150 * 1024,
            'ttl': 48 * 60,  # 48 hours
        })
    if fst:
        hw_params.update({'cpu_guarantee': 20})
    return vh.HardwareParams(**hw_params)


def get_fst_recipe(fst):
    if fst:
        return [
            'export VINS_NUM_PROCS=20',
            'export OMP_NUM_THREADS=1',
            'export FSTCONVERT_PATH=./{{ fstconvert_binary }}',
            './{{ nlu_tools_binary }} compile_app_model --app personal_assistant --fst-only',
        ]
    else:
        return []


def get_vins_train_setup_recipe(cpu):
    cuda_device = -1 if cpu else 0
    return [
        'export YT_TOKEN={{ yt_token }}',
        'export YT_PROXY={{ cluster }}',
        'export VINS_NUM_PROCS=1',
        'export OMP_NUM_THREADS=1',
        'export CUDA_VISIBLE_DEVICES=%s' % cuda_device,
    ]


def get_metric_learning_recipe(metric_learning):
    if metric_learning:
        return [
            (
                'cat $VINS_RESOURCES_PATH/personal_assistant_model_directory.metric_learning/'
                'metric_learning/metric_learning/metric_learning_info.json'
            ), (
                './{{ train_tools_binary }} train_metric_learning --load-custom-entities --custom-intents '
                'personal_assistant/config/scenarios/scenarios.mlconfig.json '
                'personal_assistant/config/handcrafted/handcrafted.mlconfig.json '
                'personal_assistant/config/stroka/stroka.mlconfig.json '
                'personal_assistant/config/navi/navi.mlconfig.json '
                '--feature-cache {{ yt_dataset }}'
            ), (
                'cat $VINS_RESOURCES_PATH/personal_assistant_model_directory.metric_learning/'
                'metric_learning/metric_learning/metric_learning_info.json'
            ),
        ]
    else:
        return []


def get_knn_and_taggers_recipe(intents_to_train_classifiers, intents_to_train_tagger):
    if not intents_to_train_classifiers and not intents_to_train_tagger:
        # no intents to train in knn and no intents to train in tagger
        return []

    if intents_to_train_classifiers:
        classifier_str = 'scenarios --intents-to-train-classifiers %s' % intents_to_train_classifiers
    else:
        classifier_str = 'None'
    if intents_to_train_tagger:
        tagger_str = '--taggers --intents-to-train-tagger %s' % intents_to_train_tagger
    else:
        tagger_str = ''
    return [(
        './{{ nlu_tools_binary }} compile_app_model '
        '--app personal_assistant --feature-cache {{ yt_dataset }} --load-custom-entities '
        '--classifiers %s %s') % (classifier_str, tagger_str)
    ]


def get_upload_model_recipe():
    return [
        (
            './{{ train_tools_binary }} split_model_into_resources --chunks new_chunks.yaml '
            '--yamake resources/ya.make --sandbox-token {{ sandbox_token }} '
            '--owner {{ owner }}'
        ),
        'cp resources/ya.make new_ya.make',
    ]


def train_vins_scenarios(intents_to_train_classifiers, intents_to_train_tagger,
                         yt_dataset, resources, revision, patch, cpu=False, metric_learning=False, fst=False):
    yt_token = vins_global_options.yt_token                 # noqa: UnusedVariable
    cluster = vins_global_options.yt_cluster                # noqa: UnusedVariable
    sandbox_token = vins_global_options.sandbox_token       # noqa: UnusedVariable
    owner = vins_global_options.sandbox_resources_owner     # noqa: UnusedVariable

    cuda_flag = False if cpu else True
    nlu_tools_binary = vh_arcadia_build.build_arcadia_binary(
        'alice/vins/tools/nlu',
        'alice/vins/tools/nlu/nlu_tools',
        revision,
        patch,
        cuda=cuda_flag,
    )
    train_tools_binary = vh_arcadia_build.build_arcadia_binary(
        'alice/vins/tools/train',
        'alice/vins/tools/train/train_tools',
        revision,
        patch,
        cuda=cuda_flag,
    )
    if fst:
        fstconvert_binary = vh_arcadia_build.build_arcadia_binary(
            'devtools/experimental/umbrella/contrib/openfst/src/bin/fstconvert',
            'devtools/experimental/umbrella/contrib/openfst/src/bin/fstconvert/fstconvert',
            5092789,  # FIXME: fix this after discuss with devtools
        )
    else:
        fstconvert_binary = None
    recipe = [
        'tar -xzf {{ resources }}',
        'export VINS_RESOURCES_PATH=$PWD/resources/',
    ]
    recipe += get_fst_recipe(fst)
    recipe += get_vins_train_setup_recipe(cpu)
    recipe += get_metric_learning_recipe(metric_learning)
    recipe += get_knn_and_taggers_recipe(intents_to_train_classifiers, intents_to_train_tagger)
    recipe += get_upload_model_recipe()
    print 'Learning recipe:\n    %s' % '\n    '.join(recipe)
    tgt_args = [nlu_tools_binary, train_tools_binary, fstconvert_binary, yt_dataset, resources]
    tgt_args = filter(lambda arg: arg is not None, tgt_args)
    return vh.tgt(
        ('new_chunks.yaml', 'new_ya.make'),
        *tgt_args,
        recipe=' && '.join(recipe),
        hardware_params=get_hardware_params(cpu, metric_learning, fst)
    )


@click.command('do_main')
# ------ Loading features options
@click.option('--vins-url', default=None, help='VINS url without URIpath')
@click.option('--heavy', default=False, is_flag=True, help='VINs only features')
@click.option('--dataset-yt-path', default=None, help='YT path with prepared dataset')
@click.option('--loaded-dataset-yt-path', default=None, help='YT path with loaded dataset')
#
# ------ YT options
@click.option('--yt-token', required=True, help='Nirvana secret name with YT token')
@click.option('--yql-token', required=True, help='Nirvana secret name with YQL token')
@click.option('--mr-account', default='voice', show_default=True, help=(
    'By default, output tables and directories will be created '
    'in //home/voice/<workflow owner>/nirvana'
))
@click.option('--mr-ttl', default=5, show_default=True, help=(
    'Time to live for intermediate MR tables'
))
@click.option('--yt-pool', default='', show_default=True, help='YT pool of hardware')
@click.option('--yt-cluster', default='hahn', show_default=True,
              type=click.Choice(['hahn', 'arnold']), help='Yt cluster')
#
# ------ Nirvana run optins
@click.option('--no-exec', default=False, is_flag=True, help='Do not execute graph after creation')
@click.option('--cpu', default=False, is_flag=True, help='Learn vins on cpu')
@click.option('--project', help='Project in Nirvana for runnning graph')
@click.option('--guid', help='Workflow guid in Nirvana for running graph')
@click.option('--nirvana-quota', default='voice-core', help='Quota for run in Nirvana')
#
# ------ Arcadia options
@click.option('--revision', default=vh_arcadia_build.arcadia_revision(), help=(
    'Specify arcadia revision number, by default this value is set from "svn info" command '
    'in arcadia/ directory'
))
@click.option('--build-patch', default=None, help=(
    'Patch for runner (diff file rbtorrent, paste.y-t.ru link or plain text. Doc: https://nda.ya.ru/3QTTV4)'
))
@click.option('--package-patch', default=None, help=(
    'Patch number for resource package (usage like "--build-patch" '
    'option)'
))
@click.option('--sandbox-token', required=True, help='Nirvana secret name with sandbox oauth token')
@click.option('--sandbox-owner', default='VINS', show_default=True, help='Sandbox resources owner')
#
# ------ Learning VINS options
@click.option('--intents-to-train-classifiers', default=None, help=(
    'Regular expression of intents names for which the classifier is trained '
    '(ex: ".*show_route.*" or ".*" - for all intents)'
))
@click.option('--intents-to-train-tagger', default=None, help=(
    'Learning tagger intent\'s name regex (ex: ".*show_route.*" or ".*" - for all intents)'
))
@click.option('--metric-learning', default=False, is_flag=True, help=(
    'Run metric learning, "--intents-to-train-*" options will be ignored'
))
@click.option('--fst', default=False, is_flag=True, help=(
    'Build custom entities'
))
@click.option('--skip-train', default=False, is_flag=True, help='Do not run vins training')
@click.option(
    '--yaplus-token', default='robot-alice-vins_oauth_token',
    help='Nirvana secret with external passport YaPlus token'
)
def do_main(vins_url, heavy, dataset_yt_path, loaded_dataset_yt_path,
            yt_token, yql_token, mr_account, mr_ttl, yt_pool, yt_cluster,
            no_exec, cpu, project, guid, nirvana_quota,
            revision, build_patch, package_patch, sandbox_token, sandbox_owner,
            intents_to_train_classifiers, intents_to_train_tagger, metric_learning, fst, skip_train,
            yaplus_token
            ):
    try:
        revision_arg = int(revision)
    except ValueError:
        print 'Arcadia revision number must be integer'
        raise

    if metric_learning:
        intents_to_train_classifiers = ".*"
        intents_to_train_tagger = ".*"

    if intents_to_train_classifiers:
        intents_to_train_classifiers = '"%s"' % intents_to_train_classifiers
    if intents_to_train_tagger:
        intents_to_train_tagger = '"%s"' % intents_to_train_tagger

    resources = vh_arcadia_build.build_ya_package(
        'alice/vins/packages/vins_resources_only_package.json',
        revision_arg,
        package_patch,
    )
    if metric_learning or intents_to_train_classifiers or intents_to_train_tagger or skip_train:
        # load dataset only for training or only for dataset loading
        if loaded_dataset_yt_path is None:
            mr_table_with_dataset = get_vins_mr_table_with_dataset(
                dataset_yt_path, resources, revision_arg, build_patch,
                intents_to_train_classifiers, intents_to_train_tagger, yt_cluster
            )
            mr_table_with_features = load_features_to_dataset(mr_table_with_dataset, vins_url, heavy, yt_cluster)
        else:
            mr_table_with_features = vh.YTTable(loaded_dataset_yt_path)
    else:
        mr_table_with_features = None

    if not skip_train:
        train_vins_scenarios(
            intents_to_train_classifiers, intents_to_train_tagger,
            mr_table_with_features, resources,
            revision_arg, build_patch, cpu, metric_learning, fst
        )

    start = not no_exec
    global_options = {
        'yt_token': yt_token,
        'yql_token': yql_token,
        'yt_pool': yt_pool,
        'yt_cluster': yt_cluster,
        'mr_account': mr_account,
        'mr_output_ttl': mr_ttl,
        'sandbox_token': sandbox_token,
        'sandbox_resources_owner': sandbox_owner,
        'sandbox_vault_owner_yt_token': 'VINS' if sandbox_owner == 'VINS' else None,
        'sandbox_vault_name_yt_token': 'robot-voiceint_yt_token' if sandbox_owner == 'VINS' else None,
        'scraper_pool': 'alice',
        'yaplus_token': yaplus_token,
    }
    vh.run(
        project=project, workflow_guid=guid, quota=nirvana_quota, start=start, global_options=global_options,
        yt_token_secret=yt_token, yt_proxy=yt_cluster, yt_pool=yt_pool, mr_account=mr_account
    )


if __name__ == '__main__':
    do_main()
