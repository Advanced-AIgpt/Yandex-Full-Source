TRAIN_VAL_SPLIT = """
$share = CAST({{param['share']}} AS Double);
$data = {{input1->table_quote()}};
$random_seed = 42;

$train = (
    SELECT *
    FROM $data
    TABLESAMPLE
    BERNOULLI($share * 100)
    REPEATABLE($random_seed)
);

$val = (
    SELECT *
    FROM $data
    WHERE text NOT IN (
        SELECT text
        FROM $train
    )
);

INSERT INTO {{output1}} SELECT * FROM $train;
INSERT INTO {{output2}} SELECT * FROM $val;
"""

GET_CLASSIFIER_MATCHES = """
$threshold = Yson::ConvertToDouble({{file:/threshold->quote()}}j.threshold);

INSERT INTO {{output1}}
SELECT * FROM {{input1}}
WHERE score > $threshold;
"""

GET_CLASSIFIER_FALSE_POSITIVES = """
$threshold = Yson::ConvertToDouble({{file:/threshold->quote()}}j.threshold);

INSERT INTO {{output1}}
SELECT * FROM {{input1}}
WHERE target == 0 AND score > $threshold;
"""

EVAL_SAVE_RESULT = """

$GetFolder = ($path) -> {
    $parts = String::SplitToList($path, '/');
    $parts = ListTake($parts, ListLength($parts) - 1);
    RETURN String::JoinFromList($parts, '/');
};

$corpus_folder = $GetFolder($corpus);

$is_logs = (
    SELECT
        ListHas(
            ListMap(
                Yson::ConvertToList(Attributes.schema),
                ($column) -> {
                        return Yson::ConvertToString($column.name);
                }),
            'intent'
        )
    FROM
        FOLDER($corpus_folder, 'schema')
    WHERE
        String::EndsWith($corpus, Path)
);

EVALUATE IF (NOT $is_logs)
    DO BEGIN
        INSERT INTO
            {{output1}}
        WITH TRUNCATE
        SELECT
            corpus.*,
            scores.score AS score
        FROM
            $corpus AS corpus
        LEFT JOIN
            $scores_table AS scores
        ON
            corpus.text == scores.text
        ORDER BY
            score
        ;
    END DO
ELSE
    DO BEGIN
        INSERT INTO
            {{output1}}
        WITH TRUNCATE
        SELECT
            *
        FROM (
            SELECT
                corpus.text AS text,
                corpus.intent AS intent,
                scores.score AS score,
                corpus.cnt AS cnt
            FROM
                $corpus AS corpus
            LEFT JOIN
                $scores_table AS scores
            ON
                corpus.text == scores.text
            ORDER BY score DESC
            LIMIT 10000000
        )
        ORDER BY
            score
        ;
    END DO;
"""

EVAL_CATBOOST_ON_DATASET_REQUEST = """
PRAGMA yt.DefaultMaxJobFails = '1';

$classifier = CatBoost::LoadModel(
    FilePath('model.cbm')
);

$corpus = {{input1->table_quote()}};

$dataset = (
    SELECT
        UNWRAP(CAST(SOME(sentence_embedding) AS List<Float>)) AS FloatFeatures,
        ListCreate(String) AS CatFeatures,
        text AS PassThrough
    FROM
        $corpus
    GROUP BY
        text
);

$scores_table = (
    SELECT
        UNWRAP(processed.Result[0]) AS score,
        processed.PassThrough AS text
    FROM (
        PROCESS $dataset
        USING CatBoost::EvaluateBatch(
            $classifier,
            TableRows()
        )
    ) AS processed
);

""" + EVAL_SAVE_RESULT


MERGE_RESULTS_SCRIPT = """
INSERT INTO {{output1}}
SELECT * FROM {{concat_input1}};
"""


ADD_EMBEDDINGS_SCRIPT = """
$embeddings = (
    SELECT
        text AS text,
        SOME(normalized_text) AS normalized_text,
        SOME(sentence_embedding) AS sentence_embedding
    FROM {{input2}}
    GROUP BY text
);

INSERT INTO {{output1}}
SELECT
    dataset.text AS text,
    dataset.target AS target,
    embeddings.sentence_embedding AS sentence_embedding,
    embeddings.normalized_text AS normalized_text
FROM {{input1}} AS dataset
JOIN $embeddings AS embeddings
USING(text);
"""


def get_separate_table_script(cache_table: str) -> str:
    return f"""
-- uncached part of queries
INSERT INTO {{{{output1}}}}
SELECT input.*
FROM {{{{input1}}}} AS input
     LEFT ONLY JOIN `{cache_table}` AS cache USING(text);

-- cached part of queries
$cache = (
    SELECT
        text AS text,
        SOME(normalized_text) AS normalized_text,
        SOME(sentence_embedding) AS sentence_embedding
    FROM `{cache_table}`
    GROUP BY text
);

INSERT INTO {{{{output2}}}}
SELECT
    input.*,
    cache.normalized_text AS normalized_text,
    cache.sentence_embedding AS sentence_embedding
FROM {{{{input1}}}} AS input
JOIN $cache AS cache
USING(text);
"""


GET_INTENT_STATS = """
INSERT INTO {{output1}}
SELECT
    intent AS intent,
    SUM(cnt) AS cnt,
    AGGREGATE_LIST(DISTINCT text, 10) AS examples
FROM {{input1}}
GROUP BY intent;
"""


GET_MATCH_STATS = """
INSERT INTO {{output1}}
SELECT
    'total' AS dataset,
    SUM(cnt) AS count_weighted,
    COUNT(*) AS count_unique
FROM {{input1}}
UNION ALL
SELECT
    'matched' AS dataset,
    SUM(cnt) AS count_weighted,
    COUNT(*) AS count_unique
FROM {{input2}};
"""
