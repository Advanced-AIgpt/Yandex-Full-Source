from alice.uniproxy.library.experiments import Experiment, Experiments
from alice.uniproxy.library.events import Event


def impl_experiment(cfg):
    base_exp = {
        'id': 'exp1313',
        'share': 0.01,
        'flags': []
    }
    base_exp.update(cfg)
    return Experiment(base_exp)


class FakeUniSystem:
    def __init__(self):
        self.patcher = None
        self.log_events = []
        self.session_data = {'uuid': '123'}
        self.exps_check = False

    def set_event_patcher(self, patcher):
        self.patcher = patcher

    def log_experiment(self, log_event):
        self.log_events.append(log_event)


def test_use_experiment():
    u = FakeUniSystem()
    es = Experiments(None)
    es.exps.append(impl_experiment({
        'share': 1.,
        'flags': ['somedata'],
    }))
    assert(es.try_use_experiment(u))
    assert(u.patcher)
    assert(u.log_events)
    assert(not u.log_events[0]['control'])


def test_use_mutabe_shares():
    u = FakeUniSystem()
    es = Experiments([
        {
            'id': 'exp1',
            'share': 0.01,
            'flags': [
                [
                    'if_event_type', 'Vins.VoiceInput',
                    'set', '.test_id1', True
                ]
            ],
        },
    ], mutable_shares=True)
    event = Event({
        'header': {
            'namespace': 'Vins',
            'name': 'VoiceInput',
            'messageId': '123',
        },
        'payload': {
            'a': 'b',
        }
    })
    session_data = {}

    share = es.get_share('exp1')
    assert share < 0.01 + 0.0001 and share > 0.01 - 0.0001

    share = es.set_share('exp1', 0.5)
    share = es.get_share('exp1')
    assert share < 0.5 + 0.0001 and share > 0.5 - 0.0001

    share = es.set_share('exp1', 1.0)
    assert es.try_use_experiment(u)
    u.patcher.patch(event, session_data)

    # check aсtivation experiment (after update share to 1.0)
    assert event.payload.get('test_id1', False)


def test_experiment_distribution():
    es = Experiments(None)
    share = 0.1
    es.exps.append(impl_experiment({
        'share': share,
        'flags': ['somedata'],
    }))
    cnt_sessions = 50000
    accuracy = 0.1
    cnt_usage = 0
    for i in range(0, cnt_sessions):
        u = FakeUniSystem()
        u.session_data['uuid'] = '123-123-{}'.format(i)
        if es.try_use_experiment(u):
            cnt_usage += 1

    ideal_cnt_usage = share * cnt_sessions
    assert(cnt_usage < ideal_cnt_usage * (1 + accuracy))
    assert(cnt_usage > ideal_cnt_usage * (1 - accuracy))


def test_same_uuid_experiment_distribution():
    es = Experiments(None)
    share = 0.5
    es.exps.append(impl_experiment({
        'share': share,
        'flags': ['somedata'],
    }))
    cnt_sessions = 1000
    cnt_usage = 0
    for i in range(0, cnt_sessions):
        u = FakeUniSystem()
        u.session_data['uuid'] = '1234-5678-910A'
        if es.try_use_experiment(u):
            cnt_usage += 1

    # check all sessions entirely was used for experiment or not
    assert cnt_usage == 0 or cnt_usage == cnt_sessions
