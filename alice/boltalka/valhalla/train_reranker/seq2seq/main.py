import vh
import yt.wrapper as yt
import numpy as np
from itertools import count


class MockSelectiveColumnsMapper(object):
    def __init__(self, num_static_factors, context_length=3):
        self.context_length = context_length
        self.num_static_factors = num_static_factors
    def __call__(self, row):
        for i in range(self.context_length):
            row['context_' + str(i)] = ''
        for i in range(self.num_static_factors):
            row['static_factor_' + str(i)] = 0
        row['dssm_id'] = 0
        row['engagement_score'] = None
        row['is_assessor_index'] = None
        yield row

@vh.lazy.hardware_params(vh.HardwareParams(max_ram=1000))
@vh.lazy(input_table=vh.YTTable, output_table=vh.mkoutput(vh.YTTable), num_static_factors=vh.mkoption(int))
def mock_selective_columns(input_table, num_static_factors, output_table):
    yt.run_map(MockSelectiveColumnsMapper(num_static_factors), input_table, output_table)
    return output_table

@vh.lazy.hardware_params(vh.HardwareParams(max_ram=1000))
@vh.lazy(input_table=vh.YTTable, output_table=vh.mkoutput(vh.YTTable))
def zero_context_embeddings(input_table, output_table):
    def mapper(row):
        zero = np.zeros(300, dtype=np.float32).tobytes()
        row['context_embedding'] = zero
        for i in count():
            k = 'factor_dssm_{}:context_embedding'.format(i)
            if k not in row:
                break
            row[k] = zero
        yield row
    yt.run_map(mapper, input_table, output_table)
    return output_table
