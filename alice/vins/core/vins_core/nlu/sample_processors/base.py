# -*- coding: utf-8 -*-
from __future__ import unicode_literals


class BaseSampleProcessor(object):
    """Abstract class for sample processors.
    Childs serve to DialogManager for processing sample inside DialogManager.handle(utterance, session)
    method.

    All childs reserve key names (attribute ``NAME``) when are registered in ``registry.py``.

    They can use session object for implementing complex logic.
    """

    NAME = None

    def __init__(self, **kwargs):
        self.label = self.__class__.__name__

    @property
    def is_normalizing(self):
        """Should be True if processor changes text property of the Sample"""
        raise NotImplementedError

    def _process(self, sample, session, is_inference, *args, **kwargs):
        """Only one method which must be implemented in each child class. Process sample and return a modified one.

        Parameters
        ----------
        sample : vins_core.common.sample.Sample
            Sample object.
        session : vins_core.dm.session.Session
            Session object.
        is_inference : bool
            Flag indicating whether it's an inference time or training/initialization phase.

        Returns
        -------
        new_sample : vins_core.common.sample.Sample
            Modified sample.
        """
        raise NotImplementedError

    def __call__(self, sample, session=None, is_inference=True, **kwargs):
        """Process sample and return a modified one.

        Parameters
        ----------
        sample : vins_core.common.sample.Sample
            Sample object.
        session : vins_core.dm.session.Session
            Session object.
        is_inference : bool, optional
            Flag indicating whether it's an inference time or training/initialization phase. Default is True.

        Returns
        -------
        new_sample : vins_core.common.sample.Sample
            Modified sample.
        """
        if sample:
            return self._process(sample, session, is_inference, **kwargs)
        return sample
