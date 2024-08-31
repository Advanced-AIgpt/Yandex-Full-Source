from alice.paskills.penguinarium.storages.ydb_utils import YdbTable


def test_base():
    assert YdbTable.columns.fget(None) is None
    assert YdbTable.primary_keys_num.fget(None) is None
