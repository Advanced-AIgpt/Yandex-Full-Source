from alice.beggins.internal.vh.operations import ext

import vh3


WEEK_DAYS = ('Monday', 'Tuesday', 'Wednesday', 'Thursday', 'Friday', 'Saturday', 'Sunday')


def get_table(logs_directory: vh3.String, week_beginning: vh3.String, shift: int) -> vh3.Expr:
    return vh3.Expr(f'''
[#setting date_format="yyyy-MM-dd"]
[#function add_days date shift]
  [#assign milliseconds = 24 * 60 * 60 * 1000]
  [#return (date?long + shift * milliseconds)?number_to_date]
[/#function]
[#assign date = add_days({week_beginning:var}?date, {shift})]
${{{logs_directory:var} + date}}
''')


def get_merging_and_filtering_script() -> vh3.Expr:
    return vh3.Expr('''
PRAGMA yt.InferSchema = '99';

INSERT INTO {{output1}} WITH TRUNCATE
SELECT query AS text, intent AS intent
FROM {{concat_input1}}
WHERE query IS NOT NULL;
''')


def get_unicalization_script() -> vh3.Expr:
    return vh3.Expr('''
PRAGMA yt.InferSchema = '99';

INSERT INTO {{output1}} WITH TRUNCATE
SELECT DISTINCT text
FROM {{input1}};
''')


def get_normalization_script() -> vh3.Expr:
    return vh3.Expr('''
PRAGMA yt.InferSchema = '99';

PRAGMA File(
    'libbert_models_udf.so',
    'yt://hahn/home/ranking/prod_build_artifacts_storage/latest_libbert_models_udf.so'
);
PRAGMA udf('libbert_models_udf.so');

$NormalizeBert = ($x) -> {
    return SearchRequest::NormalizeBert(CAST($x AS Utf8)) ?? ""
};

INSERT INTO {{output1}}
SELECT text AS text,
       $NormalizeBert(text) AS normalized_text,
FROM {{input1}};
''')


def get_grouping_script() -> vh3.Expr:
    return vh3.Expr('''
PRAGMA yt.InferSchema = '99';

INSERT INTO {{output1}} WITH TRUNCATE
SELECT text, intent,
       COUNT(*) AS cnt
FROM {{input1}}
GROUP BY text, intent;
''')


def get_save_script(date: vh3.String) -> vh3.Expr:
    return vh3.Expr(f'''
PRAGMA yt.InferSchema = '99';

INSERT INTO `//home/alice/beggins/scraped_logs/zeliboba_embedder/{date:expr}` WITH TRUNCATE
SELECT queries.text AS text,
       queries.intent AS intent,
       queries.cnt AS cnt,
       Yson::ConvertToDoubleList(processed.sentence_embedding) AS sentence_embedding,
       processed.normalized_text AS normalized_text,
FROM {{{{input1}}}} AS queries
JOIN {{{{input2}}}} AS processed USING(text);
''')


@vh3.decorator.graph(workflow_id='https://nirvana.yandex-team.ru/flow/87c362cd-04ae-4e05-b575-1a2bcade37d9')
def zeliboba_week_logs(logs_directory: vh3.String, week_beginning: vh3.String):
    days = []
    for shift, name in enumerate(WEEK_DAYS):
        days.append(ext.get_mr_table(
            table=get_table(logs_directory, week_beginning, shift),
            **vh3.block_args(name=f'Get {name}'),
        ))

    merged = ext.yql_2(
        input1=days,
        request=get_merging_and_filtering_script(),
        **vh3.block_args(name='Merge and Filter'),
    ).output1
    unified = ext.yql_2(
        input1=[merged, ],
        request=get_unicalization_script(),
        **vh3.block_args(name='Unicalization'),
    ).output1

    normalized = ext.yql_2(
        input1=[unified, ],
        request=get_normalization_script(),
        **vh3.block_args(name='Normalize'),
    ).output1

    model = ext.get_mr_file(
        path='//home/gena/sevakon/alice/embeddings/base_pretrained_wo_part_emb512_v5/model.npz',
        **vh3.block_args(name='Get Model'),
    )
    embedded = ext.zeliboba_inference_embed(
        input_table=normalized,
        finetuned_model=model,
        **vh3.block_args(name='Zeliboba Embed'),
    )

    grouped = ext.yql_2(
        input1=[merged, ],
        request=get_grouping_script(),
        **vh3.block_args(name='Grouping'),
    ).output1

    ext.yql_2(
        input1=[grouped, ],
        input2=[embedded, ],
        request=get_save_script(week_beginning),
        **vh3.block_args(name='Save'),
    )
