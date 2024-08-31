# -*- coding: utf-8 -*-
from __future__ import unicode_literals


class FrameScoring(object):

    def __call__(self, frames, session, **kwargs):
        self._validate_input(frames)
        scores = self._call(frames, session, **kwargs)
        self._validate_output(scores)
        return scores

    def _call(self, frames, session, **kwargs):
        raise NotImplementedError()

    @classmethod
    def _validate_input(cls, frames):
        pass

    @classmethod
    def _validate_output(cls, frames):
        pass

    @classmethod
    def _skip(cls, frames):
        return [None] * len(frames)
