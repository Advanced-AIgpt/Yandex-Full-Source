# coding: utf-8
from __future__ import unicode_literals

from vins_api.speechkit.resources.speechkit import SKResource
from vins_api.speechkit.connectors.navi import NaviSKConnector


class NaviSKResource(SKResource):
    use_dummy_storages = True

    @classmethod
    def connector_cls(cls, *args, **kwargs):
        return NaviSKConnector(listen_by_default=False, *args, **kwargs)

    def on_post(self, req, resp):
        return super(NaviSKResource, self).on_post(req, resp, 'navi')
