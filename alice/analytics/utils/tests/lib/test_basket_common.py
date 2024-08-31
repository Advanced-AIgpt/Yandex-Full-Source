from copy import deepcopy
from utils.yt.basket_common import filter_bad_flags, BAD_FLAGS


def test_filter_bad_flags():
    base_experiments = {'a': 1, 'b': '2', 'c': True}

    experiments = deepcopy(base_experiments)
    assert filter_bad_flags(experiments, BAD_FLAGS) == base_experiments

    experiments = deepcopy(base_experiments)
    experiments['context_load_apply'] = '1'
    assert filter_bad_flags(experiments, BAD_FLAGS) == base_experiments

    experiments = deepcopy(base_experiments)
    experiments['websearch_bass_music_cgi_rearr'] = '1'
    experiments['websearch_cgi_rearr=fon%3DV_ALICE_WEB_MUSIC_L3-629680'] = '1'
    assert filter_bad_flags(experiments, BAD_FLAGS) == base_experiments
