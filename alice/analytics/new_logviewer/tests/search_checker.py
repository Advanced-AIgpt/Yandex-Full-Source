import pytest

from datetime import timedelta

from alice.analytics.new_logviewer.lib.search.logviewer_request import LogviewerRequest
from alice.analytics.new_logviewer.lib.search.logviewer_executor import (
    LogviewerExecutor,
    LogviewerExecutorConfig
)


def create_executor(**kwargs):
    default = dict(
        uuid="",
        query="",
        reply="",
        intent="",
        app="",
        skill_id="",
        generic_scenario="",
        expboxes="",
        search="",
        mm_scenario="",
    )

    default.update(kwargs)
    request = LogviewerRequest(**default)
    config = LogviewerExecutorConfig(
        cluster="hahn",
        clique="*public",
        storage_directory="//home/alice-dev/logviewer",
        date_to_table=lambda t: t.strftime("%Y-%m-%d"),
        table_granularity=timedelta(days=1),
        need_time_condition=True
    )
    yt_client = None
    return LogviewerExecutor(request, config, yt_client)


@pytest.fixture
def executor1():
    return create_executor(
        begin="2021-08-25 00:00",
        end="2021-08-27 23:59",
        query="hello",
        reply="goodbye",
        app="tv",
        search="Search",
        mm_scenario='Vins',
    )


@pytest.fixture()
def executor2():
    return create_executor(
        begin="2021-08-25 00:00",
        end="2021-08-27 23:59",
        search="Search",
        mm_scenario='Vins',
    )


@pytest.fixture()
def executor3():
    return create_executor(
        begin="2021-08-25 00:00",
        end="2021-08-27 23:59",
        search="Search",
        mm_scenario='Vins',
        is_newbie='y',
    )


def test_get_conditions(executor1):
    tested = executor1._LogviewerExecutor__get_conditions

    actual = tested(need_time_condition=False)
    expected = "`app` = 'tv' AND " \
               "match(`query`, 'hello') AND " \
               "match(`reply`, 'goodbye') AND " \
               "match(`mm_scenario`, 'Vins')"
    assert actual == expected

    actual = tested(need_time_condition=True)
    expected = "`app` = 'tv' AND " \
               "match(`query`, 'hello') AND " \
               "match(`reply`, 'goodbye') AND " \
               "match(`mm_scenario`, 'Vins') AND " \
               "(`ts` BETWEEN '2021-08-25 00:00' AND '2021-08-27 23:59')"
    assert actual == expected


def test_get_dialog_table_query(executor1):
    tested = executor1._LogviewerExecutor__get_dialog_table_query

    actual = tested(table="2021-08-26", need_time_condition=False)
    expected = "SELECT * FROM `//home/alice-dev/logviewer/2021-08-26`\n" \
               "WHERE `app` = 'tv' AND " \
               "match(`query`, 'hello') AND " \
               "match(`reply`, 'goodbye') AND " \
               "match(`mm_scenario`, 'Vins')\n" \
               "LIMIT 500"
    assert actual == expected


def test_get_number_table_query(executor1):
    tested = executor1._LogviewerExecutor__get_number_table_query

    actual = tested(table="2021-08-26", need_time_condition=False)
    expected = "SELECT '2021-08-26' AS `date`, COUNT(*) AS `number` FROM `//home/alice-dev/logviewer/2021-08-26`\n" \
               "WHERE `app` = 'tv' AND " \
               "match(`query`, 'hello') AND " \
               "match(`reply`, 'goodbye') AND " \
               "match(`mm_scenario`, 'Vins')"
    assert actual == expected


def test_get_dialog_query(executor2):
    tested = executor2._LogviewerExecutor__get_dialog_query

    actual = tested()
    expected = "SELECT * FROM (\n" \
               "SELECT * FROM `//home/alice-dev/logviewer/2021-08-25`\n" \
               "WHERE match(`mm_scenario`, 'Vins') AND " \
               "(`ts` BETWEEN '2021-08-25 00:00' AND '2021-08-27 23:59')\n" \
               "LIMIT 500\n" \
               "UNION ALL\n" \
               "SELECT * FROM `//home/alice-dev/logviewer/2021-08-26`\n" \
               "WHERE match(`mm_scenario`, 'Vins')\n" \
               "LIMIT 500\n" \
               "UNION ALL\n" \
               "SELECT * FROM `//home/alice-dev/logviewer/2021-08-27`\n" \
               "WHERE match(`mm_scenario`, 'Vins') AND " \
               "(`ts` BETWEEN '2021-08-25 00:00' AND '2021-08-27 23:59')\n" \
               "LIMIT 500\n" \
               ") ORDER BY `ts`\n" \
               "LIMIT 500"
    assert actual == expected


def test_get_number_query(executor2):
    tested = executor2._LogviewerExecutor__get_number_query

    actual = tested()
    expected = "SELECT '2021-08-25' AS `date`, COUNT(*) AS `number` FROM `//home/alice-dev/logviewer/2021-08-25`\n" \
               "WHERE match(`mm_scenario`, 'Vins') AND (`ts` BETWEEN '2021-08-25 00:00' AND '2021-08-27 23:59')\n" \
               "UNION ALL\n" \
               "SELECT '2021-08-26' AS `date`, COUNT(*) AS `number` FROM `//home/alice-dev/logviewer/2021-08-26`\n" \
               "WHERE match(`mm_scenario`, 'Vins')\n" \
               "UNION ALL\n" \
               "SELECT '2021-08-27' AS `date`, COUNT(*) AS `number` FROM `//home/alice-dev/logviewer/2021-08-27`\n" \
               "WHERE match(`mm_scenario`, 'Vins') AND (`ts` BETWEEN '2021-08-25 00:00' AND '2021-08-27 23:59')"
    assert actual == expected


def test_get_dialog_query_for_newbie(executor3):
    tested = executor3._LogviewerExecutor__get_dialog_table_query

    actual = tested(table="2021-08-26", need_time_condition=False)
    expected = (
        "SELECT * FROM `//home/alice-dev/logviewer/2021-08-26`\n"
        "WHERE match(`mm_scenario`, 'Vins') AND `is_new` = '1 week'\n"
        "LIMIT 500"
    )
    assert actual == expected
