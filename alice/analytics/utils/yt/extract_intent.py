# -*-coding: utf8 -*-
import json
from alice.analytics.utils.json_utils import get_path


def get_vins_response(VinsResponse, context):
    vins_response = json.loads(VinsResponse)
    if not context:
        vins_response = vins_response["directive"]["payload"]
    return vins_response


def get_mm_scenario(VinsResponse, context):
    if VinsResponse:
        vins_response = get_vins_response(VinsResponse, context)
        analytics_info = vins_response.get("megamind_analytics_info")
        if analytics_info and analytics_info.get("analytics_info"):
            # g-kostin@ говорит, что не бывает 2 сценариев и в этом поле всегда лежит название сценария,
            # а вот scenario_analytics_info может и не быть, т.к. поле не обязательное
            info = analytics_info.get("analytics_info")
            if info:
                return list(info.keys())[0]
    return None


def get_product_scenario_name(VinsResponse, context):
    if VinsResponse:
        vins_response = get_vins_response(VinsResponse, context)
        analytics_info = vins_response.get("megamind_analytics_info")
        if analytics_info and analytics_info.get("analytics_info"):
            info = analytics_info.get("analytics_info")
            for key, val in info.items():
                return val.get("scenario_analytics_info", {}).get("product_scenario_name")
    return None


def get_skill_id(analytics_info):
    objects = get_path(analytics_info, ['analytics_info', 'Dialogovo', 'scenario_analytics_info', 'objects'])
    skill_id = None
    if objects:
        skill_id = get_path(objects[0], ['skill', 'id'])
    return skill_id


def get_music_genre(analytics_info):
    objects = get_path(analytics_info, ['analytics_info', 'HollywoodMusic', 'scenario_analytics_info', 'objects'])
    genre = None
    if objects:
        genre = get_path(objects[0], ['first_track', 'genre'])
    return genre


def get_intent_from_analytics_info(analytics_info):
    if analytics_info and analytics_info.get("analytics_info"):
        info = analytics_info.get("analytics_info")
        for key, val in info.items():
            return val.get("scenario_analytics_info", {}).get("intent")
    return None


def get_intent_from_semantic_frame(analytics_info):
    if analytics_info and analytics_info.get("analytics_info"):
        info = analytics_info.get("analytics_info")
        for key, val in info.items():
            return val.get("semantic_frame", {}).get("name")
    return None


def get_intent_from_meta(vins_response):
    if vins_response.get("response", {}) and vins_response["response"].get("meta"):
        for meta in vins_response["response"]["meta"]:
            if meta["type"] == "analytics_info":
                if "intent" in meta:
                    return meta["intent"]
                elif "intent" in meta.get("payload", {}):
                    return meta["payload"]["intent"]
    return None


def get_intent(VinsResponse, context=False):
    intent = None
    if VinsResponse:
        vins_response = get_vins_response(VinsResponse, context)
        analytics_info = vins_response.get("megamind_analytics_info")
        intent = get_intent_from_analytics_info(analytics_info)
        if not intent:
            intent = get_intent_from_semantic_frame(analytics_info)
            if not intent:
                intent = get_intent_from_meta(vins_response)
    if intent:
        intent = intent.replace(".", "\t").replace("__", "\t")
    return intent
