import os
import pytest

from yatest import common

from alice.nlu.data.ru.test.binary_classifiers.common.begemot import BegemotClient
from yql.api.v1.client import YqlClient


YQL_BEGGINS_CATBOOST_QUERY = """
$dataset = (
    SELECT
        UNWRAP(CAST(SOME(beggins_models__sentence_embedding) AS List<Float>)) AS FloatFeatures,
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

SELECT text FROM $scores_table WHERE score >= $threshold;
"""


@pytest.fixture
def begemot_client():
    return BegemotClient()


@pytest.fixture
def models():
    path = common.source_path('alice/nlu/data/ru/models/alice_binary_intent_classifier/data.inc')
    with open(path, 'r') as stream:
        rows = stream.readlines()
    models = {}
    row_id = 0
    while row_id < len(rows):
        while row_id < len(rows) and not rows[row_id].startswith('FROM_SANDBOX'):
            row_id += 1
        row_id += 1
        if row_id >= len(rows):
            break
        resource_id = rows[row_id].strip()
        while row_id < len(rows) and not rows[row_id].startswith(')'):
            row_id +=1
        if row_id >= len(rows):
            break
        path = rows[row_id - 1].strip()
        if 'beggins_models' in path and path.endswith('model.cbm'):
            models[path.split('/')[1]] = {
                'resource_id': resource_id,
                'model': rows[row_id - 3].strip(),
            }
    return models


@pytest.fixture
def yql_client():
    return YqlClient(token=os.environ['YQL_TOKEN'], db='hahn')


def get_model_url(name, models):
    meta = models[name]
    return f'https://proxy.sandbox.yandex-team.ru/{meta["resource_id"]}/{meta["model"]}'


def get_matches(request):
    matches = set()
    for table in request.get_results():
        table.fetch_full_data()
        cell_id = None
        for i, column_name in enumerate(table.column_names):
            if column_name == 'text':
                cell_id = i
            break
        if cell_id is not None:
            for row in table.rows:
                matches.add(str(row[cell_id]))
            break
    return matches


def parse_beggins_embedding(begemot_response):
    return list(map(float, begemot_response['rules']['AliceBegginsEmbedder']['SentenceEmbedding']))
