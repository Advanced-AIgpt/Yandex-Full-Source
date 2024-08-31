import vh3
import library.python.resource as resource
from vh3.yt import get_mr_table
from .data_preparation import DataPreparationGraph
from .operations import mr_sort, mr_read_tsv, convert_to_fml, upload_pool_to_fml, get_mr_table_by_fml_pool, \
    resolve_factors, python3_text_processor_text_input_json_text_outputs, python3_any_to_json, build_arcadia_project


class ResolveSlicesGraph:
    def __init__(self, data_preparation_graph: DataPreparationGraph):
        self.sort_val = mr_sort(
            sort_by=['text'],
            srcs=[data_preparation_graph.build_mm_targets[0]],
            timestamp=vh3.context.timestamp_for_data_preparation,
            **vh3.block_args(name='')
        )
        self.mr_read_tsv_1 = mr_read_tsv(
            columns=['music', 'text', 'features'],
            table=self.sort_val,
            output_escaping=False,
            timestamp=vh3.context.timestamp_for_data_preparation,
            **vh3.block_args(name='')
        )
        self.build_converter = build_arcadia_project(
            targets='junk/olegator/DIALOG-4759/convert_to_fml',
            arts='junk/olegator/DIALOG-4759/convert_to_fml/convert_to_fml',
            arcadia_revision=vh3.context.arcadia_revision,
            strip_binaries=True,
            **vh3.block_args(name='')
        )
        self.convert_to_fml_1 = convert_to_fml(
            target=0,
            features=2,
            executable=self.build_converter.arcadia_project,
            dataset=self.mr_read_tsv_1,
            max_ram=1000,
            utterance=1,
            **vh3.block_args(name='')
        )
        self.get_mr_table_slices = get_mr_table(
            cluster='hahn',
            creation_mode='CHECK_EXISTS',
            table=vh3.context.slices,
            **vh3.block_args(name='Get slices')
        )
        self.mr_read_tsv_2 = mr_read_tsv(
            columns=['slices'],
            table=self.get_mr_table_slices,
            output_escaping=False,
            timestamp=vh3.context.timestamp_for_data_preparation,
            **vh3.block_args(name='')
        )
        self.python3_any_to_json_1 = python3_any_to_json(
            input_type='file',
            body=resource.find('resolve_slices/model_metadata.py').decode(),
            input=self.mr_read_tsv_2,
            **vh3.block_args(name='')
        )
        self.upload_pool_to_fml_1 = upload_pool_to_fml(
            title='Postclassifier Megamind',
            timestamp=vh3.context.timestamp_for_data_preparation,
            features=self.convert_to_fml_1,
            factor_slices=self.mr_read_tsv_2,
            **vh3.block_args(name='')
        )
        self.get_mr_table_by_fml_pool_1 = get_mr_table_by_fml_pool(
            pool=self.upload_pool_to_fml_1,
            table='directory',
            preferred_mr_runtime_type='YT',
            **vh3.block_args(name='')
        )
        self.resolve_factors_1 = resolve_factors(
            for_catboost=True,
            expression=vh3.context.force_zero_factors_pre,
            timestamp=vh3.context.timestamp_for_data_preparation,
            pool_path=self.get_mr_table_by_fml_pool_1.directory,
            **vh3.block_args(name='')
        )
        self.resolve_factors_2 = resolve_factors(
            for_catboost=True,
            expression=vh3.context.force_zero_factors_post,
            timestamp=vh3.context.timestamp_for_data_preparation,
            pool_path=self.get_mr_table_by_fml_pool_1.directory,
            **vh3.block_args(name='')
        )
        self.process_python3_1 = python3_text_processor_text_input_json_text_outputs(
            max_ram=128,
            code=resource.find('resolve_slices/ignored_features.py').decode(),
            input1=self.resolve_factors_1.tested_factors,
            **vh3.block_args(name='')
        )
        self.process_python3_2 = python3_text_processor_text_input_json_text_outputs(
            max_ram=128,
            code=resource.find('resolve_slices/ignored_features.py').decode(),
            input1=self.resolve_factors_2.tested_factors,
            **vh3.block_args(name='')
        )
