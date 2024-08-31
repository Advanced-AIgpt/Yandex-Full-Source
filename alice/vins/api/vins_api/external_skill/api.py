# coding: utf-8

from vins_api.common.api import make_app as make_base_app
from vins_api.external_skill.resources import ExternalSkillResource
from vins_api.external_skill import settings


def make_app():
    app = make_base_app()
    skill_resource = ExternalSkillResource(settings)
    app.add_route('/external_skill/app/{app_id}', skill_resource)
    return app, skill_resource


app, skill_resource = make_app()
