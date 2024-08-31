# coding: utf-8
from __future__ import unicode_literals

from ..compile_app_model import create_app


def test_custom_intents(test_app):
    app = create_app(
        app_name_or_path=test_app,
        custom_intents=['vins_core/test/test_data/test_app/general/custom_intents.json']
    )
    assert set(app.nlu.intent_infos.keys()) == {
        'test_bot.general.the_funniest_intent_ever',
        'test_bot.general.the_most_boring_intent_ever',
        'test_bot.general.the_ugliest_intent_ever',
        'test_bot.general.micro1'
    }
