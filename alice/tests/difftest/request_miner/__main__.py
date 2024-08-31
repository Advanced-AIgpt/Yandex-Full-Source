from yql.api.v1.client import YqlClient
import argparse
import datetime
import json
import logging


QUERY_TEMPLATE = '''
$intent_limit = {intent_limit};

-- Захват интента из ответов Megamind
-- Так как handcrafted очень много, то считаем все handcrafted одним интентом
$intent_capture_callable = Re2::Capture(
    '"intent":"(?P<intent>([^A-Z"]+?))(\.handcrafted\..*)?"'
);
$intent_capture = ($message) -> {{
    RETURN $intent_capture_callable($message);
}};

-- Захват request_id из ответов и запросов Megamind
$reqid_capture_callable = Re2::Capture(
    '"request_id":"(?P<reqid>.*?)"'
);
$reqid_capture = ($message) -> {{
    RETURN $reqid_capture_callable($message);
}};

-- Берем request_id и intent от ОТВЕТОВ
$reqids_and_intents_responses = SELECT $reqid_capture(Message).reqid as reqid, $intent_capture(Message).intent AS intent
                                FROM hahn.`home/logfeller/logs/megamind-log/1d/{day}`
                                WHERE Message LIKE 'Megamind response%'
                                    AND $intent_capture(Message).intent NOT LIKE '%handcrafted%'
                                    AND $reqid_capture(Message).reqid NOT LIKE 'd4fa807b-b5cc-49d7-8a82-8b037dfedff8'
                                    AND $reqid_capture(Message).reqid NOT LIKE 'ffffffff-ffff-ffff%'
                                    AND $reqid_capture(Message).reqid NOT LIKE 'dddddddd-dddd-dddd%';

-- Берем request_id и Message от ЗАПРОСОВ
$reqids_and_messages_requests = SELECT $reqid_capture(Message).reqid as reqid, Message
                                FROM hahn.`home/logfeller/logs/megamind-log/1d/{day}`
                                WHERE Message LIKE '{{"%'
                                    AND $reqid_capture(Message).reqid NOT LIKE 'd4fa807b-b5cc-49d7-8a82-8b037dfedff8'
                                    AND $reqid_capture(Message).reqid NOT LIKE 'ffffffff-ffff-ffff%'
                                    AND $reqid_capture(Message).reqid NOT LIKE 'dddddddd-dddd-dddd%';

-- Джойним и берем только те запросы, где есть связь между запросом и ответом
$reqids_and_intents = SELECT L.reqid as reqid, L.intent as intent
                      FROM $reqids_and_intents_responses as L
                      LEFT JOIN $reqids_and_messages_requests as R
                      ON L.reqid == R.reqid
                      WHERE R.Message IS NOT NULL
                      AND intent IS NOT NULL;

-- Нумерация позиции внутри интента
$reqids_and_intents_numerated = SELECT reqid, intent,
                                (ROW_NUMBER() OVER w) AS pos
                                FROM $reqids_and_intents
                                WINDOW w AS (
                                    PARTITION BY intent
                                );

-- request_id сбалансированно по интентам
$reqids_limited = SELECT reqid
                    FROM $reqids_and_intents_numerated
                    WHERE pos <= $intent_limit;

-- Склейка и взятие нужных Message
SELECT Message
    FROM $reqids_limited as L
    LEFT JOIN $reqids_and_messages_requests as R
    ON L.reqid == R.reqid;
'''


def setup_logger():
    root_logger = logging.getLogger('')
    file_logger = logging.StreamHandler()
    file_logger.setFormatter(logging.Formatter('%(asctime)s %(levelname)-8s %(filename)s:%(lineno)d %(message)s'))
    root_logger.addHandler(file_logger)
    root_logger.setLevel(logging.DEBUG)


if __name__ == '__main__':
    setup_logger()

    # parse args
    parser = argparse.ArgumentParser(description='I will make table for requests!')
    parser.add_argument('--intent-limit', dest='intent_limit', required=True)
    parser.add_argument('--output', dest='output', required=True)
    args = parser.parse_args()

    # setup query
    now = datetime.datetime.now() - datetime.timedelta(days=2)
    query = QUERY_TEMPLATE.format(
        intent_limit=args.intent_limit,
        day='{:02d}-{:02d}-{:02d}'.format(now.year, now.month, now.day)
    )

    # setup request
    client = YqlClient()
    request = client.query(query.decode('utf-8'), syntax_version=1)

    logging.info('run YQL query {}'.format(query))
    request.run()  # start the execution

    logging.info('wait query result')
    results = request.get_results()  # wait execution finish

    if not results.is_success:
        logging.error('request failed: status={}'.format(results.status))
        if results.errors:
            for error in results.errors:
                logging.error('returned error: {}'.format(error))
        exit(1)
    logging.info('query is success')

    table_path = {}
    for cluster, path in results.table_paths():
        logging.info('Cluster {}, path {}'.format(cluster, path))
        table_path = {'cluster': cluster, 'path': path}
    with open(args.output, 'w') as f:
        json.dump(table_path, f)
