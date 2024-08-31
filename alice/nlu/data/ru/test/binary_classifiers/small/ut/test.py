import pytest
from yatest import common

import alice.nlu.data.ru.test.binary_classifiers.common.conftest as testing
from yql.client.parameter_value_builder import YqlParameterValueBuilder as ValueBuilder


QUERY_TOP = 'DECLARE $threshold AS Float;\n'

QUERY_BOTTOM = """

$classifier = CatBoost::LoadModel(
    FilePath('model.cbm')
);

$corpus = $pool;

""" + testing.YQL_BEGGINS_CATBOOST_QUERY


def make_query(queries, begemot_client):
    assert len(queries) <= 100, 'test case limit of entries for small test is 100'
    values = []
    for text in queries:
        embedding = testing.parse_beggins_embedding(begemot_client.query(text))
        values.append(f"('{text}', [{', '.join(map(str, embedding))}])")
    values = ',\n'.join(values)
    return QUERY_TOP + f"""
$pool = SELECT
    *
FROM (
    VALUES
{values}
) AS t(text, beggins_models__sentence_embedding);
""" + QUERY_BOTTOM


def read_target(params, type_):
    path = common.source_path(f'alice/nlu/data/ru/test/binary_classifiers/small/target/{params["name"]}.{type_}.txt')
    with open(path, 'r') as stream:
        return {line.strip() for line in stream}


def read_positives(params):
    return read_target(params, 'positives')


def read_negatives(params):
    return read_target(params, 'negatives')


def get_error_message(false_positives, false_negatives):
    msg_lines = ['\nSmallTestError:']
    for name, elements in {'false positives': false_positives, 'false negatives': false_negatives}.items():
        if len(elements) == 0:
            continue
        msg_lines.append(f'  {name}:')
        for element in elements:
            msg_lines.append(f'    {element}')
    msg_lines.append('\n')
    return '\n'.join(msg_lines)


@pytest.mark.parametrize('params', [
    {
        "name": "qr_code",
        "model": "AliceBegginsFixlistQRCode",
        "threshold": 2.5590160000206197,
    },
], ids=lambda x: x['name'])
def test_beggins_models(params, yql_client, models, begemot_client):
    positives = read_positives(params)
    negatives = read_negatives(params)
    request = yql_client.query(make_query(positives | negatives, begemot_client), syntax_version=1)
    request.attach_url(testing.get_model_url(params['model'], models), 'model.cbm')
    request.run(ValueBuilder.build_json_map({
        '$threshold': ValueBuilder.make_float(params['threshold']),
    }))
    matches = testing.get_matches(request)

    false_positives = {match for match in matches if match in negatives}
    false_negatives = {positive for positive in positives if positive not in matches}

    assert not (false_positives | false_negatives), get_error_message(false_positives, false_negatives)
