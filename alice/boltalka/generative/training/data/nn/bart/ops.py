import os

from ml_data_reader.text_processing.voc import Voc
from ml_data_reader.task.seq2seq.augment import bart_pretraining_augmentation

import vh
import yt.wrapper as yt


class BartPreprocessingMapper:
    def __init__(self, voc_path, columns, processed_prefix=''):
        self.voc_path = voc_path
        self.columns = columns
        self.preprocessing_fn = None
        self.processed_prefix = processed_prefix

    def start(self):
        self.preprocessing_fn = bart_pretraining_augmentation(Voc.load(self.voc_path))

    def __call__(self, row):
        for column in self.columns:
            src, dst = row[column], None
            src, dst = self.preprocessing_fn(src, dst)
            row[self.processed_prefix + column] = src
        yield row


@vh.lazy.hardware_params(vh.HardwareParams(max_ram=1000))
@vh.lazy.from_annotations
def bart_preprocessing(
        input_table: vh.YTTable, voc: vh.File,
        columns: vh.mkinput(str, nargs='*'), processed_prefix: str = ''
) -> vh.YTTable:
    out_table = yt.TablePath(yt.create_temp_table())
    yt.run_map(
        BartPreprocessingMapper(voc_path=os.path.basename(voc), columns=columns, processed_prefix=processed_prefix),
        input_table, out_table, local_files=[voc]
    )
    return out_table
