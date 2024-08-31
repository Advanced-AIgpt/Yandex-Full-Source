class BaseCallback(object):
    def on_batch_end(self, trainer):
        """
        Callback to call on batch end

        Parameters
        ----------
        trainer : vins_core.nlu.neural.metric_learning.trainer.Trainer
            a trainer instance to get training logs/set training parameters etc.
        """

        pass
