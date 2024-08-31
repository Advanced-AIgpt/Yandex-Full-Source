#!/usr/bin/env python
# encoding: utf-8
# sessions intent to VINS intent


def to_vins_intent(intent):
    if '\t' not in intent:
        intent = intent.replace(".", '\t')
    path = intent.replace('external_skill_gc', 'external_skill') \
        .replace('scenarios\tmarket_beru', 'scenarios\tmarket').replace('scenarios\tmarket_beru', 'scenarios\tmarket')
    if path.startswith("personal_assistant\tscenarios\tskill_recommendation"):
        path = "personal_assistant\tscenarios\tskill_recommendation"
    if path in ("personal_assistant\tscenarios\tiot_do", "personal_assistant\tscenarios\tvinsless\tadd_point",
                "personal_assistant\tscenarios\tvinsless\tfind_poi",
                "personal_assistant\tscenarios\tvinsless\thandcrafted\thello"):
        path = path.replace('personal_assistant\tscenarios', 'alice')
    if path in ("personal_assistant\tscenarios\tfind_poi\torg_card",
                "personal_assistant\tscenarios\tfind_poi\torg_card\tevents"):
        path = "personal_assistant\tscenarios\tfind_poi"
    if path.startswith("personal_assistant\tscenarios\tsearch\t"):
        if path.endswith("_anaphoric"):
            path = "personal_assistant\tscenarios\tsearch_anaphoric"
        if path.split('\t')[-1] in ("factoid_call", "factoid_src", "serp", "show_on_map"):
            path = "personal_assistant\tscenarios\tsearch\t" + path.split('\t')[-1]
        else:
            path = "personal_assistant\tscenarios\tsearch"
    parts = path.split('\t')
    if parts[0] == "alice" or len(parts) >= 4 and \
            (parts[1] == "handcrafted" and parts[2] in ("autoapp", "drive", "quasar") or
             parts[1] == "scenarios" and parts[2] in ("hny", "quasar", "common")):
        vins_intent = '.'.join(parts[:4])
        if len(parts) > 4:
            vins_intent += '__' + '__'.join(parts[4:])
    else:
        vins_intent = '.'.join(parts[:3])
        if len(parts) > 3:
            vins_intent += '__' + '__'.join(parts[3:])
    return vins_intent
