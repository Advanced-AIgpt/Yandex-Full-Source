import attr


@attr.s
class FormCandidate(object):
    form = attr.ib()
    intent = attr.ib()
    index = attr.ib(default=0)
    frame = attr.ib(default=None)
    precomputed_data = attr.ib(default={})
