import nn_applier


class DssmApplier:
    """
    Class to restore the DSSM model from the path and to use it for text embedding.
    """

    def __init__(self, dssm_model_path, input_name, output_name, text_preprocessing_fn):
        self.model = nn_applier.Model(dssm_model_path)

        self.input_name = input_name
        self.output_name = output_name
        self.text_preprocessing_fn = text_preprocessing_fn

    def predict(self, text):
        if self.text_preprocessing_fn is not None:
            text = self.text_preprocessing_fn(text)
        return self.model.predict({self.input_name: text.encode('utf-8')}, [self.output_name])
