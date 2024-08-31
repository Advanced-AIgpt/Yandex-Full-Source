# coding: utf-8

from vins_api.common.api import make_app as make_base_app
from vins_api.common.resources import SolomonMetricsResorce
from vins_api.speechkit import settings
from vins_api.speechkit.resources import protocol as protocol_resources
from vins_api.speechkit.resources import qa as qa_resourses
from vins_api.speechkit.resources import speechkit as speechkit_resources
from vins_api.speechkit.resources import navi as navi_resources


def make_app():
    app = make_base_app()

    app_resources = {
        'nlu': qa_resourses.NLUResource(settings),
        'features': qa_resourses.FeaturesResource(settings),
        'sk': speechkit_resources.SKResource(settings),
        'navi': navi_resources.NaviSKResource(settings),
        'protocol': protocol_resources.ProtocolResource(settings)
    }

    app.add_route('/qa/{app_id}/nlu', app_resources['nlu'])
    app.add_route('/qa/{app_id}/features', app_resources['features'])
    app.add_route('/qa/{app_id}/features_light', app_resources['features'])

    # Order in find matches in falcon's URI template:
    #  1) without variable template
    #  2) variable template
    # Example:
    # add_route('/speechkit/app/navi')
    # add_route('/speechkit/app/{app_id}')
    # Order to find matches will be:
    # 1 - /speechkit/app/navi
    # 2 - /speechkit/app/{app_id}
    # Another example:
    # add_route('/speechkit/app/{app_id}')
    # add_route('/speechkit/app/navi')
    # Order to find matches will be:
    # 1 - /speechkit/app/navi
    # 2 - /speechkit/app/{app_id}
    app.add_route('/speechkit/app/navi', app_resources['navi'])
    app.add_route('/speechkit/app/{app_id}', app_resources['sk'])

    app.add_route('/proto/app/{app_id}/{method}', app_resources['protocol'])
    app.add_route('/proto/app/{app_id}/scenario/{scenario_id}/{method}', app_resources['protocol'])

    app.add_route('/solomon', SolomonMetricsResorce(settings))

    return app, app_resources


app, app_resources = make_app()
