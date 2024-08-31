# Based on arc:/alice/analytics/operations/skill_discovery/extract_discovery.py

from nile.api.v1 import (
    clusters,
    Record,
    with_hints
)

from qb2.api.v1 import (
    typing as qt,
)
from datetime import datetime, timedelta
from os import path
from argparse import ArgumentParser

# schema of a resulting discovery tables
EXTRACT_DISCOVERY_SCHEMA = {'relevant_skills': qt.List[qt.String],
                            'clicks': qt.List[qt.SizedTuple[qt.String, qt.String]],
                            'request_id': qt.String,
                            'uuid': qt.String,
                            'utterance_text': qt.String,
                            'session_id': qt.String}


def get_skill_id_from_action_name(action_name):
    return action_name.split('__')[-1]


def get_relevant_skills(req):
    """ try to get list of shown skills. If req is not a discovery card, return None """
    if 'cards' in req and req['cards'] is not None:
        for card in req['cards']:
            if 'card_id' in card and card['card_id'] == 'relevant_skills' and card.get('actions'):
                return list(map(get_skill_id_from_action_name, card['actions']))


@with_hints(output_schema=EXTRACT_DISCOVERY_SCHEMA)
def extract_discovery(records):
    # as of now, there is no reliable way to see a click on a discovery card, so we just assume
    # that if there is a skill in session after a discovery (with skill_id that was in discovery)
    # then it was a click in discovery card
    # FIXME: cannot work properly until logging is fixed

    for record in records:

        relevant_skills = []
        clicks = []
        request_id = None
        utterance_text = None

        for req in record['session']:
            # try to get list of shown skills. If req is not a discovery card, return None
            maybe_relevant_skills = get_relevant_skills(req)

            if maybe_relevant_skills is not None:
                # if this req is a discovery
                if relevant_skills != [] and request_id is not None:
                    # if it is not a first discovery in session, yield all clicks seen so far
                    yield Record(relevant_skills=relevant_skills,
                                 clicks=clicks,
                                 request_id=request_id,
                                 uuid=record['uuid'],
                                 session_id=record['session_id'],
                                 utterance_text=utterance_text if utterance_text is not None else "")

                relevant_skills = maybe_relevant_skills
                request_id = req.get('req_id')
                utterance_text = req.get('_query')
                clicks = []
            elif (relevant_skills != [] and
                  'callback' in req and
                  req['callback'] is not None and
                  'card_id' in req['callback'] and
                  req['callback']['card_id'] == 'relevant_skills' and
                  'action_name' in req['callback'] and
                  get_skill_id_from_action_name(req['callback']['action_name']) in relevant_skills):
                # if we received a callback that indicates a click on discovery
                clicks.append((get_skill_id_from_action_name(req['callback']['action_name']), req['req_id']))
            elif (relevant_skills != [] and
                  'intent' in req and
                  req['intent'] == "personal_assistant\tscenarios\texternal_skill\tactivate_only" and
                  'skill_id' in req and
                  req['skill_id'] in relevant_skills):
                # if we simply saw a skill in session that was in a discovery
                clicks.append((req['skill_id'], req['req_id']))

        if relevant_skills != [] and request_id is not None:
            # after the session ends, don't forget to yield saved results
            yield Record(relevant_skills=relevant_skills,
                         clicks=clicks,
                         request_id=request_id,
                         uuid=record['uuid'],
                         session_id=record['session_id'],
                         utterance_text=utterance_text if utterance_text is not None else "")


def run(date, start_date, end_date, sessions_root, discovery_root, pool):
    assert (date or (start_date and end_date)), "you should specify dates: Either date or start_date AND end_date"

    cluster = clusters.Hahn()
    job = cluster.job('[Discovery] clicks')

    sessions = []

    if start_date and end_date:
        # for case then it is necessary to process many tables at once

        # convert string dates into datetime objects (can't iterate dates in string format)
        end = datetime(*map(int, end_date.split('-')))
        start = datetime(*map(int, start_date.split('-')))

        assert start <= end, 'end_date is smaller then start_date'

        for i in range((end - start).days + 1):
            # convert datetime back into string
            date_str = (start + timedelta(i)).isoformat()[:10]

            sessions.append(job.table(path.join(sessions_root, date_str)))
            sessions[-1].map(extract_discovery) \
                .put(path.join(discovery_root, date_str),
                     schema=EXTRACT_DISCOVERY_SCHEMA,
                     ensure_optional=False)
    else:
        # regular case, one session table to one discovery table
        sessions = job.table(path.join(sessions_root, date))
        discovery = sessions.map(extract_discovery)
        discovery.put(path.join(discovery_root, date),
                      schema=EXTRACT_DISCOVERY_SCHEMA,
                      ensure_optional=False)

    job.run()


def main():
    arg_parser = ArgumentParser()
    arg_parser.add_argument('--date')
    arg_parser.add_argument('--start-date')
    arg_parser.add_argument('--end-date')
    arg_parser.add_argument('--pool', default='voice')
    arg_parser.add_argument('--sessions-root', default='//home/voice/dialog/sessions')
    arg_parser.add_argument('--out-root', default='//home/paskills/discovery/datasets/Clicks/raw')

    args = arg_parser.parse_args()

    run(args.date, args.start_date, args.end_date, args.sessions_root, args.out_root, args.pool)


if __name__ == '__main__':
    main()
