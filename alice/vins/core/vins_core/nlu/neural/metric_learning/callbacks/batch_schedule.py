from vins_core.nlu.neural.metric_learning.callbacks.base_callback import BaseCallback


class BatchSchedule(BaseCallback):
    def __init__(self, schedule=None):
        """
        Callback to change batch shape during training

        Parameters
        ----------
        schedule
            list of tuples (step : int, shape : dict) to switch batch shape to 'shape' on step 'step'
            'shape' is a dict with keys 'num_classes_in_batch' and 'batch_samples_per_class'
        """

        self._schedule = schedule or []

    def on_batch_end(self, trainer):
        for step, data in self._schedule:
            if step == trainer.step:
                trainer.preprocessor.set_batch_shape(data['num_classes_in_batch'],
                                                     data['batch_samples_per_class'])
