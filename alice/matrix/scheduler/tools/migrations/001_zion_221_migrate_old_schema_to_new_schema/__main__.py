import argparse
import requests

from alice.protos.api.matrix.schedule_action_pb2 import TScheduleAction

import ydb

import library.python.init_log
import logging


requests.adapters.DEFAULT_RETRIES = 2


logger = logging.getLogger(__name__)
library.python.init_log.init_log(level='INFO')


def get_schedule_actions(ydb_endpoint, ydb_database):
    driver_config = ydb.DriverConfig(
        ydb_endpoint,
        ydb_database,
        credentials=ydb.construct_credentials_from_environ(),
        root_certificates=ydb.load_ydb_root_certificate(),
    )

    with ydb.Driver(driver_config) as driver:
        driver.wait(timeout=5)
        logger.info("YDB connection established")

        ydb_session = ydb.retry_operation_sync(lambda: driver.table_client.session().create())
        logger.info("YDB session created")

        query = """
            SELECT * FROM schedule_actions
        """

        result_sets = ydb_session.transaction(ydb.OnlineReadOnly()).execute(query, commit_tx=True)
        schedule_actions = []
        for row in result_sets[0].rows:
            schedule_actions.append((row.schedule_action_id.decode("utf-8"), row.schedule_action))

            parsed_schedule_action = TScheduleAction()
            parsed_schedule_action.ParseFromString(row.schedule_action)
            logger.info("Schedule action info %s\n%s\n", schedule_actions[-1][0], parsed_schedule_action)

        logger.info("Selected schedule actions: %s", len(schedule_actions))

        return schedule_actions


def do_migration(schedule_actions, scheduler_endpoint, migrate):
    if not migrate:
        logger.info("Migration is skipped")
        return

    for schedule_action in schedule_actions:
        try:
            rsp = requests.post(f"{scheduler_endpoint}/schedule", data=schedule_action[1])
            assert rsp.status_code == 200
            logger.info("Action %s scheduled", schedule_action[0])
        except:
            logger.exception("Failed to schedule action %s", schedule_action[0])


def parse_arguments():
    parser = argparse.ArgumentParser(description="Mirgrate database in old schema to new (ZION-221).")

    parser.add_argument(
        "-e", "--ydb-endpoint",
        dest="ydb_endpoint",
        type=str,
        default="ydb-ru-prestable.yandex.net:2135",
        help="YDB endpoint",
    )

    parser.add_argument(
        "-d", "--ydb-database",
        dest="ydb_database",
        type=str,
        default="/ru-prestable/speechkit_ops_alice_notificator/test/matrix-queue-common",
        help="Source YDB database",
    )

    parser.add_argument(
        "-t", "--scheduler-endpoint",
        dest="scheduler_endpoint",
        type=str,
        default="http://matrix-scheduler.alice.yandex.net:80",
        help="Target scheduler endpoint",
    )

    parser.add_argument(
        "--migrate",
        dest="migrate",
        action="store_true",
        default=False,
        help="Perform migration",
    )

    return parser.parse_args()


def main():
    arguments = parse_arguments()

    try:
        schedule_actions = get_schedule_actions(arguments.ydb_endpoint, arguments.ydb_database)
    except:
        logger.exception("Failed to dump old database")
        return

    try:
        do_migration(schedule_actions, arguments.scheduler_endpoint, arguments.migrate)
    except:
        logger.exception("Failed to do migration requests")
        return
