from alice.megamind.protos.common.frame_pb2 import TTypedSemanticFrame
from alice.megamind.protos.common.atm_pb2 import TAnalyticsTrackingModule
from alice.megamind.protos.scenarios.notification_state_pb2 import TNotification
from alice.megamind.protos.scenarios.frame_pb2 import TParsedUtterance
from alice.uniproxy.library.protos.uniproxy_pb2 import TPushMessage

from flask import current_app

import datetime
import random
import json
import time


def series_notification(row):
    res = {}

    res['content_name'] = row['content_name']
    res['content_uuid'] = row['content_uuid']
    res['season_num'] = row['season_num']
    res['episode_num'] = row['episode_num']
    res['watch_uuid'] = row['watch_uuid']
    res['push_id'] = row['push_id']
    res['puid'] = row['puid']

    res['subscription_id'] = "2"
    res['ring'] = "1"
    res['device_id'] = None

    notification = TNotification()
    notification.Id = row['push_id']

    response_text = "Вышла новая серия сериала «{}». Включить?".format(row['content_name'])
    response_voice = "Вышла новая серия сериала {}. Включить?".format(row['content_name'])
    notification.Text = response_text
    notification.Voice = response_voice

    # construct parsed utterance
    parsed_utterance = TParsedUtterance()

    parsed_utterance.Utterance = "включи {} {} сезон {} серия".format(row['content_name'], row['season_num'], row['episode_num'])

    parsed_utterance.TypedSemanticFrame.VideoPlaySemanticFrame.ContentType.StringValue = 'tv_show'
    parsed_utterance.TypedSemanticFrame.VideoPlaySemanticFrame.Action.StringValue = 'play'
    parsed_utterance.TypedSemanticFrame.VideoPlaySemanticFrame.SearchText.StringValue = row['content_name']
    parsed_utterance.TypedSemanticFrame.VideoPlaySemanticFrame.Season.NumValue = row['season_num']
    parsed_utterance.TypedSemanticFrame.VideoPlaySemanticFrame.Episode.NumValue = row['episode_num']

    parsed_utterance.Analytics.ProductScenario = 'series_push'
    parsed_utterance.Analytics.Origin = TAnalyticsTrackingModule.EOrigin.SmartSpeaker
    parsed_utterance.Analytics.Purpose = 'New series episode notification'

    notification.ParsedUtterance.CopyFrom(parsed_utterance)

    data = notification.SerializeToString()
    res['data'] = data.decode(errors='ignore')

    msg = TPushMessage()
    msg.Notification.ParseFromString(data)
    msg.SubscriptionId = int(res['subscription_id'])
    msg.Ring = int(res['ring'])
    msg.Uid = res['puid']
    if res['device_id']:
        msg.DeviceId = res['device_id']

    res['ts'] = int(time.time())

    return res, msg


def podcasts_notification(row):
    res = {}

    res['album_name'] = row['album_name']
    res['album_id'] = row['album_id']
    res['track_name'] = row['track_name']
    res['track_id'] = row['track_id']
    res['push_id'] = row['push_id']
    res['puid'] = row['puid']

    res['subscription_id'] = "6"
    res['ring'] = "1"
    res['device_id'] = None

    notification = TNotification()
    notification.Id = row['push_id']

    response_text = "Вышел новый выпуск подкаста «{}». Включить?".format(row['album_name'])
    response_voice = "Вышел новый выпуск подкаста {}. Включить?".format(row['album_name'])
    notification.Text = response_text
    notification.Voice = response_voice

    # construct parsed utterance
    parsed_utterance = TParsedUtterance()

    parsed_utterance.Utterance = "включи подкаст {}, выпуск {}".format(row['album_name'], row['track_name'])
    parsed_utterance.TypedSemanticFrame.MusicPlaySemanticFrame.SearchText.StringValue = 'подкаст ' + row['album_name']
    parsed_utterance.Analytics.ProductScenario = 'podcasts_push'
    parsed_utterance.Analytics.Origin = TAnalyticsTrackingModule.EOrigin.SmartSpeaker
    parsed_utterance.Analytics.Purpose = 'New podcast episode notification'

    notification.ParsedUtterance.CopyFrom(parsed_utterance)

    data = notification.SerializeToString()
    res['data'] = data.decode(errors='ignore')

    msg = TPushMessage()
    msg.Notification.ParseFromString(data)
    msg.SubscriptionId = int(res['subscription_id'])
    msg.Ring = int(res['ring'])
    msg.Uid = res['puid']
    if res['device_id']:
        msg.DeviceId = res['device_id']

    res['ts'] = int(time.time())

    return res, msg


def _get_time_to_notify():
    today = datetime.datetime.utcnow().replace(tzinfo=datetime.timezone.utc)
    if (today.weekday() >= 1 and today.hour >= 19) or (today.weekday() <= 4 and today.hour < 16):
        next_friday = today + datetime.timedelta(4 - today.weekday() % 7)
        next_friday.replace(hour=16, minute=0)
        return next_friday
    else:
        next_tuesday = today + datetime.timedelta(1 - today.weekday() % 7)
        next_tuesday.replace(hour=19, minute=0)
        return next_tuesday


def music_notification(row):
    rec = row
    res = {}

    res['subscription_id'] = "3"
    res['ring'] = "1"
    res['puid'] = rec['puid']
    res['device_id'] = None
    res['time'] = _get_time_to_notify().isoformat()

    notification = TNotification()
    notification.Id = 'music_notification'

    # get all entities, take first 10, select 1 randomly
    entities = rec['entities']
    filtered = []
    n_filtered = 0
    for e in entities:
        if (e['type'] == 'single' or e['type'] == 'album') and n_filtered < 10:
            filtered.append(e)
            n_filtered += 1
    current_app.logger.info(f'filtered size: {len(filtered)}')
    if len(filtered) == 0:
        current_app.logger.info(f'nothing was found for {row["puid"]}')
        return None, None

    music_res = filtered[random.randint(0, n_filtered-1)]

    # construct notification text/voice
    data = music_res.get('data', '')
    title = data.get('title', '')
    artists = data.get('artists', [])
    if len(artists) == 0:
        current_app.logger.info(f'empty artist for {row["puid"]}')
        current_app.logger.info(data)
        return None, None
    artist = artists[0].get('name', '')
    response = ""
    if data.get('type', '') == 'single':
        response = "У {} вышел сингл {}. Послушаем вместе?".format(artist, title)
    else:
        response = "Это должно вас порадовать. У {} вышел альбом {}. Послушаем вместе?".format(artist, title)
    notification.Text = response
    notification.Voice = response

    # construct parsed utterance
    utterance = ""
    if music_res.get('type', '') == 'single':
        utterance = "включи песню {}".format(title)
    else:
        utterance = "включи альбом {}".format(title)

    parsed_utterance = TParsedUtterance()
    parsed_utterance.Utterance = utterance

    # construct typed semantic frame
    typed_frame = TTypedSemanticFrame()

    value = {}
    value['answer_type'] = music_res.get('type', '')
    value['id'] = data.get('id', '')
    value['title'] = title
    value['genre'] = data.get('genre', '')
    value['artist'] = {}
    value['artist']['id'] = artists[0].get('id', '')
    value['artist']['name'] = artist

    typed_frame.MusicPlaySemanticFrame.SpecialAnswerInfo.SpecialAnswerInfoValue = json.dumps(value)
    typed_frame.MusicPlaySemanticFrame.ActionRequest.ActionRequestValue = "autoplay"

    parsed_utterance.TypedSemanticFrame.CopyFrom(typed_frame)

    # fill analytics info
    atm = TAnalyticsTrackingModule()
    atm.ProductScenario = 'notifications'
    atm.Origin = TAnalyticsTrackingModule.EOrigin.Scenario
    atm.Purpose = 'music'

    parsed_utterance.Analytics.CopyFrom(atm)

    notification.ParsedUtterance.CopyFrom(parsed_utterance)

    res['notification'] = notification.SerializeToString()

    msg = TPushMessage()
    msg.Notification.ParseFromString(res['notification'])
    msg.SubscriptionId = int(res['subscription_id'])
    msg.Ring = int(res['ring'])
    msg.Uid = res['puid']
    if res['device_id']:
        msg.DeviceId = res['device_id']

    return res, msg


def handcrafted_kinopoisk_notification(row):
    res = {}

    res['push_text'] = row['push_text']
    res['push_id'] = row['push_id']
    res['puid'] = row['puid']

    res['subscription_id'] = "5"
    res['ring'] = "1"
    res['device_id'] = None

    notification = TNotification()
    notification.Id = row['push_id']

    response_text = row['push_text']
    response_voice = row['push_text']
    notification.Text = response_text
    notification.Voice = response_voice

    current_app.logger.info(notification)
    data = notification.SerializeToString()
    res['data'] = data.decode(errors='ignore')

    msg = TPushMessage()
    msg.Notification.ParseFromString(data)
    msg.SubscriptionId = int(res['subscription_id'])
    msg.Ring = int(res['ring'])
    msg.Uid = res['puid']
    if res['device_id']:
        msg.DeviceId = res['device_id']

    res['ts'] = int(time.time())

    return res, msg
