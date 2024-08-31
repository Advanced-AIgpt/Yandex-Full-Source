import pytest
from pytest import approx

from alice.paskills.penguinarium.dssm_applier import DssmApplier


REL = 0.001  # approx relative error


@pytest.fixture(scope='module')
def model():
    return DssmApplier(b'assessor.dssm')


def test_load(model):
    pass


@pytest.fixture
def record_0():
    return {
        b'query': 'Ботинки'.encode('utf-8'),
        b'region_id': '1'.encode('utf-8'),
        b'title': 'Магазин обуви'.encode('utf-8'),
        b'snippet': 'Мужская обувь'.encode('utf-8'),
        b'domain_id': '999'.encode('utf-8'),
        b'goodurl_uta': 'botinki sapogi'.encode('utf-8'),
        b'stripped_goodurl': 'botino4k.ru'.encode('utf-8'),
    }


def test_predict_single(model, record_0):
    assert approx(1.042049, rel=REL) == model.predict(record_0, output_variable=b'joint_output')


def test_predict2_single(model, record_0):
    assert {b'joint_output': approx(1.042049, rel=REL)} == model.predict(record_0, output_variable=[b'joint_output'])


def test_predict_single_output(model, record_0):
    assert approx(0.739245355, rel=REL) == model.predict(record_0, output_variable=b'output')


def test_predict2_single_output(model, record_0):
    assert {b'output': approx(0.739245355, rel=REL)} == model.predict(record_0, output_variable=[b'output'])


def test_predict_single_two_variables(model, record_0):
    assert {
        b'output': approx(0.739245355, rel=REL),
        b'joint_output': approx(1.042049, rel=REL),
    } == model.predict(record_0, output_variable=[b'output', b'joint_output'])


@pytest.fixture
def record_2():
    return {
        b'query': 'Ботинки'.encode('utf-8'),
        b'region_id': 1,
        b'title': 'Магазин обуви'.encode('utf-8'),
        b'snippet': 'Мужская обувь'.encode('utf-8'),
        b'domain_id': 999,
        b'goodurl_uta': 'botinki sapogi'.encode('utf-8'),
        b'stripped_goodurl': 'botino4k.ru'.encode('utf-8'),
    }


def test_predict_single_int_fields(model, record_2):
    assert approx(0.739245355, rel=REL) == model.predict(record_2, output_variable=b'output')


def test_predict2_single_int_fields(model, record_2):
    assert {b'output': approx(0.739245355, rel=REL)} == model.predict(record_2, output_variable=[b'output'])


@pytest.fixture
def record_3():
    return {
        b'query': 'Ботинки'.encode('utf-8'),
        b'region_id': ''.encode('utf-8'),
        b'title': None,
        b'snippet': 'Мужская обувь'.encode('utf-8'),
        b'domain_id': ''.encode('utf-8'),
        b'goodurl_uta': 'botinki sapogi'.encode('utf-8'),
        b'stripped_goodurl': 'botino4k.ru'.encode('utf-8')
    }


def test_predict_single_none_fields(model, record_3):
    with pytest.raises(TypeError):
        model.predict(record_3, output_variable=b'output')


def test_predict2_single_none_fields(model, record_3):
    with pytest.raises(TypeError):
        model.predict(record_3, output_variable=[b'output'])


@pytest.fixture
def record_4():
    return {
        b'query': 'Ботинки'.encode('utf-8'),
        b'title': 'Магазин обуви'.encode('utf-8'),
        b'snippet': 'Мужская обувь'.encode('utf-8'),
        b'domain_id': '999'.encode('utf-8'),
        b'goodurl_uta': 'botinki sapogi'.encode('utf-8'),
        b'stripped_goodurl': 'botino4k.ru'.encode('utf-8')
    }


def test_predict_single_no_field(model, record_4):
    with pytest.raises(KeyError):
        model.predict(record_4, output_variable=b'output')


def test_predict2_single_no_field(model, record_4):
    with pytest.raises(KeyError):
        model.predict(record_4, output_variable=[b'output'])


def test_predict_single_wrong_output(model, record_0):
    with pytest.raises(Exception):
        model.predict(record_0, output_variable=b'join_output')


def test_predict2_single_wrong_output(model, record_0):
    with pytest.raises(Exception):
        model.predict(record_0, output_variable=[b'join_output'])


@pytest.fixture
def records_0(record_0):
    return [
        record_0,
        {
            b'query': 'ботинки'.encode('utf-8'),
            b'region_id': '23'.encode('utf-8'),
            b'title': 'стиральная машина'.encode('utf-8'),
            b'snippet': 'продаем все'.encode('utf-8'),
            b'domain_id': '45'.encode('utf-8'),
            b'goodurl_uta': 'купить все'.encode('utf-8'),
            b'stripped_goodurl': 'все в наличии'.encode('utf-8')
        },
    ]


def test_predict_multiple(model, records_0):
    assert approx([0.739245355, 0.000952581933], rel=REL) == model.predict(records_0, output_variable=b'output')


def test_predict2_multiple(model, records_0):
    assert {b'output': approx([0.739245355, 0.000952581933], rel=REL)} == model.predict(records_0, output_variable=[b'output'])


def test_predict2_multiple2(model, records_0):
    assert {b'joint_output': approx([1.042047, -6.95538], rel=REL)} == model.predict(records_0, output_variable=[b'joint_output'])


def test_predict_multiple_two_variables(model, records_0):
    assert {
        b'output': approx([0.739245355, 0.000952581933], rel=REL),
        b'joint_output': approx([1.042047, -6.95538], rel=REL),
    } == model.predict(records_0, output_variable=[b'output', b'joint_output'])


@pytest.fixture
def record_embed_0():
    return {
        b'query': 'Ботинки'.encode('utf-8'),
        b'region_id': '1'.encode('utf-8'),
    }


EMBED_0 = [
    -0.0440574251, 0.26330635, 0.161014915, -0.0944126174, -0.0497841388, -0.0560266674,
    0.0607352145, -0.351548523, -0.0428635553, -0.227219999, -0.16842109, 0.122503534,
    -0.0338583589, 0.243168041, 0.106314473, -0.258476347, -0.13620694, -0.124583349, -0.284339666,
    0.0342236236, -0.164328128, -0.0530459546, 0.191022426, 0.122385174, -0.37266916, -0.154133767,
    -0.0578309521, -0.172005236, -0.0430171713, -0.312578797, 0.00324716303, -0.182334855
]


def test_query_embedding(model, record_embed_0):
    assert approx(EMBED_0, rel=REL) == model.predict(record_embed_0, output_variable=b'query_embedding')


def test_query_embedding2(model, record_embed_0):
    assert {b'query_embedding': approx(EMBED_0, rel=REL)} == model.predict(record_embed_0, output_variable=[b'query_embedding'])


@pytest.fixture
def record_embed_1():
    return {
        b'title': 'Магазин обуви'.encode('utf-8'),
        b'snippet': 'Мужская обувь'.encode('utf-8'),
        b'domain_id': '999'.encode('utf-8'),
        b'goodurl_uta': 'botinki sapogi'.encode('utf-8'),
        b'stripped_goodurl': 'botino4k.ru'.encode('utf-8'),
    }


EMBED_1 = [
    0.200000003, -0.333333343, 0.333333343, -0.466666669, -0.200000003, 0.333333343, 0.200000003,
    -0.733333349, -0.200000003, -0.600000024, -0.600000024, 0.333333343, -0.333333343, 0.600000024,
    -0.333333343, -0.733333349, -0.200000003, -0.466666669, -0.733333349, 0.200000003, -0.466666669,
    0.333333343, 0.733333349, 0.733333349, -0.600000024, -1.0, 0.466666669, -0.0666666701,
    -0.600000024, -0.733333349, 0.466666669, -0.600000024
]


def test_doc_embedding(model, record_embed_1):
    assert approx(EMBED_1, rel=REL) == model.predict(record_embed_1, output_variable=b'doc_embedding')


def test_doc_embedding2(model, record_embed_1):
    assert {b'doc_embedding': approx(EMBED_1, rel=REL)} == model.predict(record_embed_1, output_variable=[b'doc_embedding'])


def test_wrong_path_type():
    with pytest.raises(ValueError):
        DssmApplier(None)


def test_wrong_records_type(model):
    with pytest.raises(TypeError):
        model.predict(None, output_variable=b'')

    with pytest.raises(TypeError):
        model.predict(None, output_variable=[b''])


def test_wrong_output_type(model):
    with pytest.raises(TypeError):
        model.predict(record_embed_1, output_variable=None)
