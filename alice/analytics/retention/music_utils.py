import math
from collections import defaultdict

def add_features(feature_dict):
    music_scenarios = ['music', 'meditation', 'music_ambient_sound', 'music_fairy_tale', 'music_podcast']
    prefixes = ['total', 'recommendation']
    for p in prefixes:
        p_name = "tlt_{}".format(p)
        feature_dict[p_name] = sum([feature_dict.get('music.{}.{}.total.time$'.format(s, p), 0) for s in music_scenarios])
        feature_dict["sqrt_" + p_name] = math.sqrt(feature_dict[p_name])
        feature_dict["log_" + p_name] = math.log(feature_dict[p_name] + 1)
        p_name_music = 'music.music.{}.total.time$'.format(p)
        feature_dict["sqrt_" + p_name_music] = math.sqrt(feature_dict.get(p_name_music, 0))
        feature_dict["log_" + p_name_music] = math.log(feature_dict.get(p_name_music, 0) + 1)
    actions = ['event.music.total.count$', 'event.player_commands.total.count$']
    denom = 'event.total.count$'
    for action in actions:
        feature_dict['share_' + action] = 1.0*feature_dict.get(action, 0.0)/(feature_dict.get(denom) or 1.0)
    actions = [
        'music.music.total.music_what_is_playing.count$',
        'music.music.total.player_dislike.count$',
        'music.music.total.player_like.count$',
        'music.music.total.player_next_track.count$',
        'music.music.total.player_replay.count$',
        'event.player_dislike.total.count$',
        'event.player_next_track.total.count$',
        'uuid.music_answer_types.total.count$',
        'uuid.music_genres.total.count$'
    ]
    denom = 'event.music.total.count$'
    for action in actions:
        feature_dict['share_' + action] = 1.0*feature_dict.get(action, 0.0)/(feature_dict.get(denom) or 1.0)
    wrongs = [
        'broken_reply',
        'cant_reply',
        'dura_query',
        'not_found_reply'
    ]
    for wrong in wrongs:
        nom = 'event.{}.music.count$'.format(wrong)
        denom = 'event.{}.total.count$'.format(wrong)
        feature_dict['share_' + nom] = 1.0*feature_dict.get(nom, 0.0)/(feature_dict.get(denom) or 1.0)
    feature_dict['is_active'] = int(feature_dict["tlt_total"] > 90 and feature_dict.get("event.music.total.count$", 0.0) > 2)
    feature_dict['activity'] = feature_dict["tlt_total"] if feature_dict["tlt_total"] > 90 else 0.0
    return feature_dict

def process(feature_dict):
    feature_dict = dict(feature_dict.iteritems())
    feature_dict = add_features(feature_dict)
    return feature_dict

def create_zero_dict(features):
    return {i: 0.0 for i in features}

def sum_dicts(feature_dict_list):
    res = defaultdict(float)
    for d in feature_dict_list:
        for k, v in d.items():
            res[k] += v or 0.0
    return res

def diff_dicts(feature_dict_now, feature_dict_past):
    if feature_dict_past is None:
        res = {i: None for i in feature_dict_now.keys()}
    else:
        res = defaultdict(float)
        for k, v in feature_dict_now.items():
            res[k] = (v or 0.0) - feature_dict_past.get(k, 0.0)
    return res

def dicts_to_string(feature_dict_list, features):
    res = []
    for feature_dict in feature_dict_list:
        for feature in features:
            res.append(str(feature_dict[feature]) if feature_dict.get(feature) else '')
    return '\t'.join(res)

def create_feature_names(feature_groups, features):
    res = []
    for feature_group in feature_groups:
        for feature in features:
            res.append('{}_{}'.format(feature_group, feature.strip()))
    return res
