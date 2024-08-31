# coding: utf-8
from __future__ import absolute_import


class ToDictMixin(object):
    def to_dict(self):
        rv = dict()
        for k, v in vars(self).iteritems():
            if isinstance(v, ToDictMixin):
                rv[k] = v.to_dict()
            elif isinstance(v, list):
                rv[k] = [
                    vv.to_dict() if isinstance(vv, ToDictMixin) else vv
                    for vv in v
                ]
            elif isinstance(v, dict):
                rv[k] = dict([
                    (kk, vv.to_dict() if isinstance(vv, ToDictMixin) else vv)
                    for kk, vv in v.iteritems()
                ])
            else:
                rv[k] = v
        return rv

    @classmethod
    def create_dict(cls, *args, **kwargs):
        return cls(*args, **kwargs).to_dict()
