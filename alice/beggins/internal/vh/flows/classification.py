import functools
from typing import NamedTuple, Optional, Literal, Sequence, Union

import vh3
from vh3 import (
    String, Factory, context as ctx, Expr, Binary, MRTable, block_args, JSON, Connections, HTML, Text, Number,
    File, Enum
)
from vh3.decorator import graph

import alice.beggins.internal.vh.flows.yql as yql
from alice.beggins.internal.vh.flows.common import (
    static_name, ArcContext, SandboxContext, CacheContext, YqlContext, PulsarContext, SoyContext, TolokaContext,
    NirvanaContext, eval_catboost_model, EmbedderType
)
from alice.beggins.internal.vh.flows.evaluation import (
    find_thresholds, eval_metrics, ThresholdSelectionStrategyType
)
from alice.beggins.internal.vh.flows.report import (
    make_report
)
from alice.beggins.internal.vh.flows.scrapper import scrape_queries_with_cache
from alice.beggins.internal.vh.operations import ext
from alice.beggins.internal.vh.scripts.python import (
    collect_match_stats, commit_draft
)
from alice.beggins.internal.vh.scripts.scripter import Scripter

CLASSIFICATION_FOLDER = 'alice/beggins/data/classification'
RUN_COMMAND = (
    'PYTHONPATH=$SOURCE_CODE_PATH:$PYTHONPATH '
    'python3 $SOURCE_CODE_PATH/beggins/cmd/train_classifier.py -c $SOURCE_CODE_PATH/config.yaml'
)
SUB_GRAPHS_WORKFLOW_ID = 'https://nirvana.yandex-team.ru/flow/edd405d9-ed2d-48ac-8f30-9d75f0b3df60'

PREPARE_CATBOOST_DATASET_REQUEST = """
$Stringify = ($embedding) -> {
    RETURN String::JoinFromList(ListMap($embedding, ($v) -> {
        RETURN CAST($v AS string);
    }), '\t');
};

INSERT INTO {{output1}}
SELECT CAST(target AS String) AS key, $Stringify(sentence_embedding) AS value FROM {{input1}};
"""

GET_SANDBOX_META_CODE = """
def main(in1, in2, in3, mr_tables, token1=None, token2=None, param1=None, param2=None, html_file=None):
    resource_id = in1[0]['resource_id']
    return {
        'resource_id': resource_id,
        'resource_link': f'https://sandbox.yandex-team.ru/resource/{resource_id}/view',
        'download_link': f'https://proxy.sandbox.yandex-team.ru/{resource_id}',
    }
"""

PACK_VAL_SCORES = """
def main(in1, in2, in3, mr_tables, token1=None, token2=None, param1=None, param2=None, html_file=None):
    return {
        'classifier': param1,
        'val': mr_tables[0],
     }
"""

PACK_TEST_SCORES = """
def main(in1, in2, in3, mr_tables, token1=None, token2=None, param1=None, param2=None, html_file=None):
    return {
        'test': mr_tables[0],
        **in1[0],
     }
"""

ADD_SCHEMA_SCRIPT = """
PRAGMA yt.InferSchema = '99';
INSERT INTO {{output1}}
SELECT
Yson::ConvertToDoubleList(`sentence_embedding`) as sentence_embedding,
`target` as target,
`text` as text,
`normalized_text` as normalized_text
FROM {{input1}};
"""

CLEAR_TABLE = """
PRAGMA yt.InferSchema = '99';
INSERT INTO {{output1}}
SELECT * FROM {{input1}}
WHERE false;
"""

MAKE_COMMIT_CODE = '''
model_name = v['model_name']
classifier_name = ''.join([part.title() for part in model_name.split('_')])
threshold = v['threshold']
resource_id = v['resource_id']
embedder = v['embedder']
manifest = '\\n' + '\\n'.join(w)

return [f"""
#################################################
# Add to alice/nlu/data/ru/config/frames.pb.txt #
#################################################

{{
    Name: "<your frame name>"
    Rules: [{{
        Experiments: ["bg_beggins_{model_name}"]
        Classifier: {{
            Source: "AliceBinaryIntentClassifier"
            Intent: "Alice{embedder}{classifier_name}"
            Threshold: {threshold}
            Confidence: 1
        }}
        Tagger: {{
            Source: "Always"
        }}
    }}]
}}

#################################################

""", f"""
###########################################################################
# Add to alice/nlu/data/ru/models/alice_binary_intent_classifier/data.inc #
###########################################################################

FROM_SANDBOX(
    {resource_id}
    RENAME
    {model_name}/model.cbm
    OUT_NOAUTO
    ${{PREFIX}}{embedder.lower()}_models/Alice{embedder}{classifier_name}/model.cbm
)

###########################################################################

""", f"""
###########################################################################
# Add to alice/nlu/data/ru/models/alice_binary_intent_classifier/data.inc #
# ~~~~~~~~~~~~~~~~~~~~~~~ to the bottom of file ~~~~~~~~~~~~~~~~~~~~~~~~~~#
###########################################################################

${{PREFIX}}{embedder.lower()}_models/Alice{embedder}{classifier_name}/model.cbm

###########################################################################

""", f"""
###########################################################################
# Add to alice/beggins/data/classification/{model_name}/manifest.yaml #
###########################################################################

{manifest}

###########################################################################

"""]
'''

DESCRIPTION_TEMPLATE = '''
<a href="{{ context.workflow_url }}">{{ context.workflow_url }}</a>
{% if context.metrics %}
    <table>
        <tbody>
            {% for key, value in context.metrics.items() %}
                <tr>
                    <td>{{ key }}</td>
                    <td>{{ value }}</td>
                </tr>
            {% endfor %}
        </tbody>
    </table>
{% endif %}
'''

GENERATE_POOL_DESCRIPTION = '''
-- calculate embedding length
$dims = SELECT ListLength(String::SplitToList(value, '\t')) AS dims
        FROM {{input1}}
        LIMIT 1;

-- add label description
INSERT INTO {{output1}} WITH TRUNCATE
SELECT "0" AS key, "Label" AS value;

-- add embedding description
INSERT INTO {{output1}}
SELECT * FROM (
    SELECT CAST(ListFromRange(1, Unwrap($dims) + 1) AS List<String>) AS key,
           "Num" AS value
) FLATTEN LIST BY key;
'''

TOM_CONVERSION_SCRIPT = '''
INSERT INTO {{output1}}
SELECT text AS text,
       1 - is_negative_query AS target,
FROM {{input1}}
WHERE is_negative_query IS NOT NULL;
'''


class Context(ArcContext, SandboxContext, CacheContext, YqlContext, PulsarContext, SoyContext, TolokaContext,
              NirvanaContext):
    model_name: String
    week: String = 'latest'
    threshold_selection_strategy: ThresholdSelectionStrategyType = 'max_precision_condition_recall'
    embedder: EmbedderType = 'Zeliboba'
    st_ticket: Optional[String]
    catboost_params: String = '{"loss_function": "Logloss", "iterations": 1000}'


subgraph = functools.partial(graph, workflow_id=SUB_GRAPHS_WORKFLOW_ID)


class ClassifierMeta(NamedTuple):
    model_package: Binary
    model_package_resource_meta: JSON
    tensorboard_logs: Binary


class Dataset(NamedTuple):
    train: MRTable
    val: MRTable
    test: MRTable


class LoadManifestResult(NamedTuple):
    manifest: Text
    train: MRTable
    accept: MRTable


class CatBoostTrainingResult(NamedTuple):
    model: Binary
    tensorboard_logs: Binary
    graphs: HTML


@static_name('train catboost')
@graph(workflow_id='https://nirvana.yandex-team.ru/flow/be707d07-1085-4502-b9fd-ab0b7432d6b1')
def train_catboost_model(train_data: MRTable, val_data: MRTable,
                         params: String = Factory(lambda: ctx.catboost_params)) -> CatBoostTrainingResult:
    pool_description = ext.yql_2(
        input1=[train_data],
        request=GENERATE_POOL_DESCRIPTION,
        **block_args(name='get pool description'),
    ).output1
    result = ext.catdevboost_train(
        gpu_type='CUDA_8_0',
        use_best_model=True,
        learn=train_data,
        test=val_data,
        create_tensorboard=True,
        cd=pool_description,
        params_file=ext.single_option_to_json_output(
            input=params,
            **block_args(name='get catboost params')
        )
    )
    return CatBoostTrainingResult(
        model=result.model_bin,
        tensorboard_logs=result.tensorboard_log,
        graphs=result.plots_html,
    )


class ExportModelResult(NamedTuple):
    package: Binary
    sandbox_meta: JSON


@static_name('export catboost model')
@graph(workflow_id='https://nirvana.yandex-team.ru/flow/e8b4fc67-911e-4dad-8544-bf7031c62a50')
def export_catboost_model(model: Binary,
                          model_type: String,
                          metrics: Optional[JSON] = None,
                          model_name: String = Factory(lambda: ctx.model_name),
                          embedder: EmbedderType = Factory(lambda: ctx.embedder)) -> ExportModelResult:
    archive = ext.add_to_tar(
        path=Expr(f'{model_name:expr}/model.cbm'),
        file=model,
        **block_args(name='pack catboost model'),
    )

    context = ext.merge_json_dicts(
        input_dicts=[
            ext.single_option_to_json_output(
                Expr('{"workflow_url": "${meta.workflow_url}"}'),
                **block_args(name='load workflow url'),
            ),
            metrics,
        ],
        **block_args(name='create context'),
    )
    description = ext.jinja2_template(
        context=context,
        template=ext.single_option_to_text_optional_output(
            text=DESCRIPTION_TEMPLATE,
            **block_args(name='load description template'),
        ),
        **block_args(name='create description')
    ).output_text

    end_tag = ext.ya_upload_with_attrs_and_desc(
        resource=archive,
        resource_type='OTHER_RESOURCE',
        resource_owner='VINS',
        resource_filename='data',
        resource_attributes=(
            Expr(f'model_name={model_name:expr}'),
            Expr(f'model_type={model_type:expr}'),
            Expr(f'embedder={embedder:expr}'),
        ),
        resource_description=description.as_option,
        do_not_remove=True,
        **block_args(name='upload to sandbox'),
    )

    return ExportModelResult(
        package=archive,
        sandbox_meta=ext.python3_json_process(
            in1=[end_tag],
            code=GET_SANDBOX_META_CODE,
            **block_args(name='get package meta'),
        ).out1
    )


@graph(workflow_id='https://nirvana.yandex-team.ru/flow/c294fc1d-33c3-4719-9e5a-b40457a5d936')
def export_pulsar_instance(
    tensorboard_logs: Binary,
    manifest: File,
    sandbox_package_meta: JSON,
    val_metrics: JSON,
    model_name: String = 'beggins_classifier',
    dataset_name: String = Factory(lambda: ctx.model_name),
    st_ticket: String = Factory(lambda: ctx.st_ticket),
) -> ext.PulsarUpdateInstanceOutput:
    instance = ext.pulsar_add_instance(
        model_name=model_name,
        dataset_name=dataset_name,
        **block_args(name='add instance'),
    )
    metrics = ext.light_python2_transform_json(
        body_function="return {'val_{}'.format(key): value for key, value in v['metrics'].items()}",
        input=val_metrics,
        **block_args(name='select metrics', dynamic_options=instance.info),
    )
    model = ext.pulsar_add_model(
        name=dataset_name,
        artifacts=Connections(
            manifest=ext.convert_any_to_binary_data(
                file=manifest,
                **block_args(name='cast manifest as binary'),
            ),
            sandbox_package_meta=ext.convert_any_to_binary_data(
                file=sandbox_package_meta,
                **block_args(name='cast sandbox package meta as binary'),
            ),
        ),
        description=Expr(f'st ticket: {st_ticket:expr}'),
        **block_args(name='add info', dynamic_options=instance.info),
    )
    produced_model = ext.light_python2_transform_json(
        body_function="return dict(produced_model_uid=v['model_uid'])",
        input=model.info,
        **block_args(name='produced model to params', dynamic_options=instance.info),
    )
    return ext.pulsar_update_instance(
        metrics=metrics,
        tfevents=[tensorboard_logs],
        **block_args(name='add tensorboard logs', dynamic_options=[instance.info, produced_model]),
    )


class AnalyzeModelResult(NamedTuple):
    notebook: HTML


@static_name('analyze model')
@subgraph()
def analyze_model(val_data: MRTable, test_data: MRTable) -> AnalyzeModelResult:
    result = ext.beggins_analyze_thresholds(val=val_data, test=test_data)
    return AnalyzeModelResult(
        notebook=result.notebook_html,
    )


class TrainingDataset(NamedTuple):
    train: MRTable
    val: MRTable


@static_name('prepare catboost dataset')
@graph(workflow_id='https://nirvana.yandex-team.ru/flow/74c9798c-ff59-4874-95f2-42ed11f71888')
def prepare_catboost_dataset(dataset: TrainingDataset) -> TrainingDataset:
    def prepare(table: MRTable, split: str) -> MRTable:
        result = ext.yql_2(
            input1=[table],
            request=PREPARE_CATBOOST_DATASET_REQUEST,
            **block_args(name=f'prepare {split} data'),
        )
        return result.output1

    return TrainingDataset(
        train=prepare(dataset.train, 'train'),
        val=prepare(dataset.val, 'val'),
    )


@static_name('train catboost classifier')
@graph(workflow_id='https://nirvana.yandex-team.ru/flow/7565e99b-dc71-49b2-ab2a-e338135d3480')
def train_catboost_classifier(dataset: TrainingDataset) -> CatBoostTrainingResult:
    training_dataset = prepare_catboost_dataset(dataset)
    return train_catboost_model(training_dataset.train, training_dataset.val)


class ScoresMeta(NamedTuple):
    meta: JSON


@static_name('get scores meta')
@subgraph()
def get_scores_meta(val_score: MRTable, test_scores: MRTable, classifier: String) -> ScoresMeta:
    meta = ext.python3_json_process(
        code=PACK_VAL_SCORES,
        mr=[val_score],
        param1=classifier,
        **block_args(name='add val'),
    ).out1
    meta = ext.python3_json_process(
        code=PACK_TEST_SCORES,
        mr=[test_scores],
        in1=[meta],
        param1=classifier,
        **block_args(name='add test'),
    ).out1
    return ScoresMeta(meta=meta)


@static_name('train zeliboba classifier')
@graph(workflow_id='https://nirvana.yandex-team.ru/flow/b7591039-8c25-48f8-b453-093329ccea47')
def train_zeliboba_classifier(training_dataset: TrainingDataset, accept: MRTable) -> None:
    def prepare_dataset(table: MRTable):
        data = ext.add_zeliboba_embeddings(
            input=table,
        )
        return ext.yql_2(
            input1=(data,),
            request=ADD_SCHEMA_SCRIPT,
            **block_args(name='add schema'),
        ).output1

    train = prepare_dataset(training_dataset.train)
    val = prepare_dataset(training_dataset.val)
    accept = prepare_dataset(accept)

    classifier = train_catboost_classifier(TrainingDataset(train, val))
    val_scores = eval_catboost_model(classifier.model, val)
    accept_scores = eval_catboost_model(classifier.model, accept)
    threshold = find_thresholds(accept_scores)
    eval_metrics(
        val_scores,
        threshold=0.5,
        **block_args(name='get val metrics', dynamic_options=threshold.threshold_param),
    )
    eval_metrics(
        accept_scores,
        threshold=0.5,
        **block_args(name='get accept metrics', dynamic_options=threshold.threshold_param),
    )


@static_name('train/val split')
@graph(workflow_id='https://nirvana.yandex-team.ru/flow/2c40683a-f7a1-4f6b-aef0-c334e387154e')
def train_val_split(data: MRTable, share: Number = 0.7) -> TrainingDataset:
    result = ext.yql_2(
        input1=[data],
        param=[Expr(f'share={share:expr}')],
        request=yql.TRAIN_VAL_SPLIT,
        **block_args(name='split'),
    )
    return TrainingDataset(
        train=result.output1,
        val=result.output2,
    )


class CommitDraft(NamedTuple):
    draft: Text


@graph(workflow_id='https://nirvana.yandex-team.ru/flow/12b044b1-1b8c-47ca-a6cd-75d96f6e4f96')
def make_commit_draft(sandbox_package_meta: JSON, threshold_meta: JSON, manifest: File,
                      model_name: String = Factory(lambda: ctx.model_name),
                      embedder: EmbedderType = Factory(lambda: ctx.embedder),
                      positive_honeypots: vh3.String = vh3.Factory(lambda: ctx.positive_honeypots),
                      negative_honeypots: vh3.String = vh3.Factory(lambda: ctx.negative_honeypots),
                      classification_task_question: vh3.String = vh3.Factory(lambda: ctx.classification_task_question),
                      classification_project_instructions: vh3.String = vh3.Factory(lambda: ctx.classification_project_instructions)) -> CommitDraft:
    manifest = vh3.cast(manifest, vh3.Text)

    meta = ext.merge_json_dicts(
        input_dicts=[
            ext.single_option_to_json_output(
                input=Expr(f'{{'
                           f'"model_name": "{model_name:expr}",'
                           f'"embedder": "{embedder:expr}"'
                           f'}}'),
                **block_args(name='get model name'),
            ),
            sandbox_package_meta,
            threshold_meta,
        ],
        **block_args(name='get meta'),
    )

    def escape_string(string: vh3.String) -> vh3.Expr:
        return vh3.Expr(f'${{{string:var}?j_string}}')

    toloka_option = ext.single_option_to_json_output(
        input=Expr(f'{{'
                   f'"positive_honeypots": "{escape_string(positive_honeypots):expr}",'
                   f'"negative_honeypots": "{escape_string(negative_honeypots):expr}",'
                   f'"classification_task_question": "{escape_string(classification_task_question):expr}",'
                   f'"classification_project_instructions": "{escape_string(classification_project_instructions):expr}"'
                   f'}}'),
        **block_args(name='get toloka option'),
    )

    draft = ext.python3_any_any_any_to_txt(
        input0_type='json-mem',
        input1_type='txt-mem',
        input2_type='json-mem',
        body=Scripter.to_string(commit_draft.commit_draft),
        input0=meta,
        input1=manifest,
        input2=toloka_option,
        **block_args(name='make commit draft impl'),
    )
    return CommitDraft(draft)


class GetLogsMatchesResult(NamedTuple):
    matches: MRTable
    intent_stats: JSON
    match_stats: JSON


@graph(workflow_id='https://nirvana.yandex-team.ru/flow/9ee6b18a-f94b-45f7-b4dc-3f88b7f00243')
def get_logs_matches(model: Binary, threshold_meta: JSON,
                     logs_type: Enum[Literal['week', 'year']],
                     embedder: EmbedderType = Factory(lambda: ctx.embedder),
                     week: String = Factory(lambda: ctx.week)) -> GetLogsMatchesResult:
    table = '[#if global["logs_type"] == "year"]' \
            'one_year_logs_v2_1' \
            '[#elseif global["logs_type"] == "week"]' \
            '${global["week"]}' \
            '[/#if]'
    logs_folder = '[#if global["embedder"] == "Beggins"]' \
                  '//home/alice/beggins/scraped_logs/beggins_embedder' \
                  '[#elseif global["embedder"] == "Zeliboba"]' \
                  '//home/alice/beggins/scraped_logs/zeliboba_embedder' \
                  '[/#if]'
    logs_path = f'{logs_folder}/{table}'

    logs = ext.get_mr_table(
        cluster='hahn',
        table=Expr(logs_path),
        creation_mode='CHECK_EXISTS',
        **block_args(name='get logs')
    )
    logs_scores = eval_catboost_model(model, logs)
    logs_matches = ext.yql_2(
        input1=[logs_scores],
        files=Connections(threshold=threshold_meta),
        request=yql.GET_CLASSIFIER_MATCHES,
        **block_args(name='get logs matches')
    ).output1

    intent_stats = ext.yql_2(
        input1=[logs_matches],
        request=yql.GET_INTENT_STATS,
        **block_args(name='get intent stats')
    ).output1

    intent_stats_json = ext.mr_read_json(
        table=intent_stats,
        **block_args(name='to json')
    )

    match_stats = ext.yql_2(
        input1=[logs],
        input2=[logs_matches],
        request=yql.GET_MATCH_STATS,
        **block_args(name='get match stats')
    ).output1

    match_stats_raw_json = ext.mr_read_json(
        table=match_stats,
        **block_args(name='to json')
    )

    match_stats_json = ext.python3_any_to_json(
        input_type='json-mem',
        body=Scripter.to_string(collect_match_stats.collect_match_stats),
        input=match_stats_raw_json,
        **block_args(name='collect match stats'),
    )

    return GetLogsMatchesResult(logs_matches, intent_stats_json, match_stats_json)


class TrainBinaryClassifierResult(NamedTuple):
    manifest: File
    model: Binary
    tensorboard_logs: Binary
    threshold_meta: JSON
    threshold_param: JSON
    year_logs_matches: MRTable
    week_logs_matches: MRTable
    year_logs_matches_stats: JSON
    week_logs_matches_stats: JSON
    custom_basket_metrics: JSON
    accept_basket_metrics: JSON
    report: HTML


@graph(workflow_id='https://nirvana.yandex-team.ru/flow/d41db724-a9d0-4fde-888f-26ac8a0c69bb')
def train_binary_classifier(
    manifest: Union[Binary, File, JSON] = None,
    train: Sequence[MRTable] = (),
    accept: Sequence[MRTable] = (),
    custom_basket: Optional[MRTable] = None,
) -> TrainBinaryClassifierResult:
    dataset = ext.process_manifest(
        manifest=manifest,
        dev=train,
        accept=accept,
        **block_args(name='process manifest'),
    )
    manifest = ext.dataset_storing(train=dataset.train, accept=dataset.accept)

    training_dataset = train_val_split(scrape_queries_with_cache(dataset.train))
    accept_dataset = scrape_queries_with_cache(dataset.accept)

    classifier = train_catboost_classifier(training_dataset)

    train_scores = eval_catboost_model(classifier.model, training_dataset.train)
    val_scores = eval_catboost_model(classifier.model, training_dataset.val)
    accept_scores = eval_catboost_model(classifier.model, accept_dataset)

    accept_threshold = find_thresholds(accept_scores, **block_args(name='find threshold on accept'))

    train_metrics = eval_metrics(
        train_scores,
        **block_args(
            name='get train metrics',
            dynamic_options=accept_threshold.threshold_param,
        ),
    )
    val_metrics = eval_metrics(
        val_scores,
        **block_args(
            name='get val metrics',
            dynamic_options=accept_threshold.threshold_param,
        ),
    )
    accept_metrics = eval_metrics(
        accept_scores,
        **block_args(
            name='get accept metrics',
            dynamic_options=accept_threshold.threshold_param,
        ),
    )

    year_logs = get_logs_matches(
        model=classifier.model,
        threshold_meta=accept_threshold.threshold_meta,
        logs_type='year',
        **block_args(
            name='get year logs matches'
        ),
    )
    week_logs = get_logs_matches(
        model=classifier.model,
        threshold_meta=accept_threshold.threshold_meta,
        logs_type='week',
        **block_args(
            name='get week logs matches'
        ),
    )

    report = make_report(
        threshold=accept_threshold.threshold_param,

        train_metrics=train_metrics.metrics_meta,
        val_metrics=val_metrics.metrics_meta,
        accept_metrics=accept_metrics.metrics_meta,

        threshold_plot=accept_threshold.plot,
        train_matrix=train_metrics.confusion_matrix,
        val_matrix=val_metrics.confusion_matrix,
        accept_matrix=accept_metrics.confusion_matrix,

        year_logs_stats=year_logs.intent_stats,
        week_logs_stats=week_logs.intent_stats,
    )
    custom_basket = ext.mr_optional_table_identity_transform(path=custom_basket)
    scraped_custom_basket = scrape_queries_with_cache(custom_basket, max_rps=1000)
    evaluated_custom_basket = eval_catboost_model(classifier.model, scraped_custom_basket)
    custom_basket_metrics = eval_metrics(
        evaluated_custom_basket,
        **block_args(
            name='get custom basket metrics',
            dynamic_options=accept_threshold.threshold_param,
        ),
    )

    return TrainBinaryClassifierResult(
        manifest=manifest,
        model=classifier.model,
        tensorboard_logs=classifier.tensorboard_logs,
        threshold_meta=accept_threshold.threshold_meta,
        threshold_param=accept_threshold.threshold_param,
        year_logs_matches=year_logs.matches,
        week_logs_matches=week_logs.matches,
        year_logs_matches_stats=year_logs.match_stats,
        week_logs_matches_stats=week_logs.match_stats,
        custom_basket_metrics=custom_basket_metrics.metrics_meta,
        accept_basket_metrics=accept_metrics.metrics_meta,
        report=report
    )


class TrainAndExportBinaryClassifierResult(NamedTuple):
    manifest: File
    year_logs_matches: MRTable
    week_logs_matches: MRTable
    custom_basket_metrics: JSON
    accept_basket_metrics: JSON
    commit_draft: Text
    report: HTML
    catboost_model: vh3.Binary
    threshold_param: vh3.JSON
    model_meta_info: vh3.JSON


@graph(workflow_id='https://nirvana.yandex-team.ru/flow/31bf0feb-edd5-4e01-855b-41e2889a4044')
def train_and_export_binary_classifier(
    manifest: Union[Binary, File, JSON] = None,
    train: Sequence[MRTable] = (),
    accept: Sequence[MRTable] = (),
    custom_basket: Optional[MRTable] = None,
) -> TrainAndExportBinaryClassifierResult:
    classifier = train_binary_classifier(
        manifest=manifest,
        train=train,
        accept=accept,
        custom_basket=custom_basket,
    )

    exported_model = export_catboost_model(
        model=classifier.model,
        metrics=classifier.accept_basket_metrics,
        model_type='catboost'
    )

    export_pulsar_instance(
        tensorboard_logs=classifier.tensorboard_logs,
        manifest=classifier.manifest,
        sandbox_package_meta=exported_model.sandbox_meta,
        val_metrics=classifier.accept_basket_metrics,
        **block_args(name='export pulsar instance'),
    )

    model_meta_info = ext.merge_json_dicts(
        input_dicts=[
            ext.single_option_to_json_output(
                input=Expr(f'{{'
                           f'"model_name": "{ctx.model_name:expr}",'
                           f'"embedder": "{ctx.embedder:expr}",'
                           f'"process_id": "${{meta.process_uid}}"'
                           f'}}'),
                **block_args(name='get model name'),
            ),
            exported_model.sandbox_meta,
            classifier.threshold_meta,
        ],
        **block_args(name='get meta'),
    )

    commit_draft = make_commit_draft(
        exported_model.sandbox_meta,
        classifier.threshold_meta,
        classifier.manifest
    )

    return TrainAndExportBinaryClassifierResult(
        year_logs_matches=classifier.year_logs_matches,
        week_logs_matches=classifier.week_logs_matches,
        manifest=classifier.manifest,
        custom_basket_metrics=classifier.custom_basket_metrics,
        accept_basket_metrics=classifier.accept_basket_metrics,
        commit_draft=commit_draft.draft,
        report=classifier.report,
        catboost_model=classifier.model,
        threshold_param=classifier.threshold_param,
        model_meta_info=model_meta_info,
    )
