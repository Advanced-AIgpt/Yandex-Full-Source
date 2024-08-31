import pytest
from yatest import common

import alice.nlu.data.ru.test.binary_classifiers.common.conftest as testing
from yql.client.parameter_value_builder import YqlParameterValueBuilder as ValueBuilder


QUERY = """
DECLARE $threshold AS Float;
DECLARE $pool AS String;

$classifier = CatBoost::LoadModel(
    FilePath('model.cbm')
);

$corpus = $pool;

""" + testing.YQL_BEGGINS_CATBOOST_QUERY

ALICE6V3 = '//home/alice/beggins/medium/alice6v3'


def get_error_message(matches, target_matches):
    added, deleted = matches - target_matches, target_matches - matches
    msg_lines = ['\nMediumTestError:']
    for element in added:
        msg_lines.append(f'  + {element}')
    for element in deleted:
        msg_lines.append(f'  - {element}')
    msg_lines.append('')
    return '\n'.join(msg_lines)


@pytest.mark.parametrize('params', [
    {
        "name": "qr_code",
        "model": "AliceBegginsFixlistQRCode",
        "threshold": 2.5590160000206197,
        "pool": ALICE6V3,
    },
    {
        "name": "fines",
        "model": "AliceBegginsFixlistFines",
        "threshold": 2.2400485991743153,
        "pool": ALICE6V3,
    },
    {
        "name": "read_page",
        "model": "AliceBegginsFixlistReadPage",
        "threshold": -0.7819710969924927,
        "pool": ALICE6V3,
    },
    {
        "name": "receipts",
        "model": "AliceBegginsFixlistReceipts",
        "threshold": 0.06723214533738542,
        "pool": ALICE6V3,
    },
    {
        "name": "keyboard",
        "model": "AliceBegginsFixlistKeyboard",
        "threshold": 3.7161213016099737,
        "pool": ALICE6V3,
    },
    {
        "name": "phone_assistant",
        "model": "AliceBegginsPhoneAssistant",
        "threshold": -1.4400193934416166,
        "pool": ALICE6V3,
    },
    {
        "name": "goods_best_prices_reask",
        "model": "AliceBegginsGoodsBestPricesReask",
        "threshold": -0.73,
        "pool": ALICE6V3,
    },
], ids=lambda x: x['name'])
def test_beggins_models(params, yql_client, models):
    request = yql_client.query(QUERY, syntax_version=1)
    request.attach_url(testing.get_model_url(params['model'], models), 'model.cbm')
    request.run(ValueBuilder.build_json_map({
        '$threshold': ValueBuilder.make_float(params['threshold']),
        '$pool': ValueBuilder.make_string(params['pool'])
    }))
    target_filepath = common.source_path(f'alice/nlu/data/ru/test/binary_classifiers/medium/target/{params["name"]}.txt')
    matches = testing.get_matches(request)
    if common.get_param('canonize') == 'true':
        with open(target_filepath, 'w') as stream:
            for match in matches:
                stream.write(match)
                stream.write('\n')
    else:
        target_matches = set()
        with open(target_filepath, 'r') as stream:
            target_matches = {line.strip() for line in stream}
        assert matches == target_matches, get_error_message(matches, target_matches)
