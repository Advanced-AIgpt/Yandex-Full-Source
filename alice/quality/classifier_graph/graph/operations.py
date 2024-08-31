import vh3
import typing
from .enums import ClientTypeEnum, FormulaResourceOptionsEnum


@vh3.decorator.external_operation("https://nirvana.yandex-team.ru/operation/2417849a-4303-11e7-89a6-0025909427cc")
@vh3.decorator.nirvana_output_names("output")
def single_option_to_text_output(*, input: vh3.String) -> vh3.Text:
    """
    Single Option To Text Output

    Передает указанный в опции текст на выход в формате Text. https://nda.ya.ru/t/spesTmxv3W4w6X

    """
    raise NotImplementedError("Write your local execution stub here")


@vh3.decorator.external_operation("https://nirvana.yandex-team.ru/operation/2fdd4bb4-4303-11e7-89a6-0025909427cc")
@vh3.decorator.nirvana_output_names("output")
def single_option_to_json_output(input: vh3.String) -> vh3.JSON:
    """
    Single Option To Json Output

    **Назначение операции**

    Передает заданный текст на выход в формате JSON. Содержимое не валидируется.

    **Описание выходов**

    Результат в формате JSON подается на выход "output".

    """
    raise NotImplementedError("Write your local execution stub here")


@vh3.decorator.external_operation("https://nirvana.yandex-team.ru/operation/6af8b5e1-1117-4eb3-ac1d-33e7481ddc53")
@vh3.decorator.nirvana_output_names("output")
def single_option_to_file_output(input: vh3.String) -> vh3.File:
    """
    Single Option To File Output

    **Назначение операции**

    Передает заданный текст на выход в формате JSON. Содержимое не валидируется.

    **Описание выходов**

    Результат в формате JSON подается на выход "output".

    """
    raise NotImplementedError("Write your local execution stub here")


@vh3.decorator.external_operation("https://nirvana.yandex-team.ru/operation/883b0d1a-7844-4adc-87a5-99a866d549ed")
@vh3.decorator.nirvana_output_names("output0")
@vh3.decorator.nirvana_names_transformer(vh3.name_transformers.snake_to_dash, options=True)
def options_to_json_7_key_values(
        *,
        key1: vh3.String,
        value1: vh3.String,
        ttl: vh3.Integer = 360,
        max_ram: vh3.Integer = 100,
        key2: vh3.String = None,
        value2: vh3.String = None,
        key3: vh3.String = None,
        value3: vh3.String = None,
        key4: vh3.String = None,
        value4: vh3.String = None,
        key5: vh3.String = None,
        value5: vh3.String = None,
        key6: vh3.String = None,
        value6: vh3.String = None,
        key7: vh3.String = None,
        value7: vh3.String = None
) -> vh3.JSON:
    """
    Options to JSON (7 key-values)
    """
    raise NotImplementedError("Write your local execution stub here")


@vh3.decorator.external_operation("https://nirvana.yandex-team.ru/operation/c880e02a-e937-46ae-8590-5191abefaa16")
@vh3.decorator.nirvana_names(max_ram="max-ram")
@vh3.decorator.nirvana_output_names("output")
def input_to_option(
        *,
        option_name: vh3.String,
        input: typing.Union[
            vh3.Binary,
            vh3.Executable,
            vh3.FMLDumpParse,
            vh3.FMLFormula,
            vh3.FMLFormulaSerpPrefs,
            vh3.FMLPool,
            vh3.FMLPrs,
            vh3.FMLSerpComparison,
            vh3.FMLWizards,
            vh3.File,
            vh3.HTML,
            vh3.Image,
            vh3.JSON,
            vh3.MRDirectory,
            vh3.MRFile,
            vh3.MRTable,
            vh3.TSV,
            vh3.Text,
            vh3.XML,
        ],
        ttl: vh3.Integer = 360,
        max_ram: vh3.Integer = 100
) -> vh3.JSON:
    """
    Input to Option

    Deserialize input and create JSON for dynamic options.
    The output will be {<option_name>: <input_value>}.

    :param input: Input to deserialize
    """
    raise NotImplementedError("Write your local execution stub here")


@vh3.decorator.external_operation("https://nirvana.yandex-team.ru/operation/1b10b97b-84fd-46ac-9db5-905da305cc81")
@vh3.decorator.nirvana_output_names("sorted")
@vh3.decorator.nirvana_names_transformer(vh3.name_transformers.snake_to_dash, options=True)
def mr_sort(
        *,
        sort_by: vh3.MultipleStrings,
        yt_token: vh3.Secret = vh3.Factory(lambda: vh3.context.yt_token),
        srcs: typing.Sequence[vh3.MRTable],
        dst_path: vh3.String = None,
        preserve_attributes: vh3.MultipleStrings = (),
        spec: vh3.String = None,
        mr_account: vh3.String = vh3.Factory(lambda: vh3.context.mr_account),
        yt_pool: vh3.String = None,
        timestamp: vh3.String = None
) -> vh3.MRTable:
    """
    MR Sort

    Sorts one or more tables on a single MR cluster.

    :param sort_by: Sort By:
      Sort columns
    :param yt_token: YT Token:
      Name of Nirvana Secret holding your YT Access Token
    :param srcs: Source tables to sort and merge. Must all be from the same MR cluster.
    :param dst_path: Sort Destination:
      Absolute path to sorted table (need not exist)
    :param preserve_attributes: Preserve Source Attributes:
      Copy the specified attributes from the source table to the sorted table, then perform the sort.
      Will FAIL if you specify > 1 source
    :param spec: Sort Specification (JSON):
      Format is JSON
    :param mr_account: MR Account:
      MR account name (e.g. `rank_machine'). Overrides MR_USER setting for the sort operation
    :param yt_pool: YT Pool:
      Custom operation pool this operation will run in. If not specified, sort will run in the default pool
    :param timestamp: Timestamp:
      Set a recent, not previously used timestamp to force "MR Sort" to run even if `srcs' were already sorted into `dst-path'
    """
    raise NotImplementedError("Write your local execution stub here")


@vh3.decorator.external_operation("https://nirvana.yandex-team.ru/operation/d23ec268-5a94-42f9-bd6b-5062dccbd3f5")
@vh3.decorator.nirvana_output_names("tsv")
@vh3.decorator.nirvana_names_transformer(vh3.name_transformers.snake_to_dash, options=True)
def mr_read_tsv(
        *,
        yt_token: vh3.Secret = vh3.Factory(lambda: vh3.context.yt_token),
        columns: vh3.MultipleStrings,
        table: vh3.MRTable,
        ttl: vh3.Integer = 360,
        max_ram: vh3.Integer = 1024,
        max_disk: vh3.Integer = 1024,
        start_row: vh3.Integer = None,
        end_row: vh3.Integer = None,
        missing_value_mode: vh3.Enum[typing.Literal["FAIL", "PRINT_SENTINEL", "SKIP_ROW"]] = "FAIL",
        missing_value_sentinel: vh3.String = None,
        output_escaping: vh3.Boolean = True,
        output_escaping_symbol: vh3.String = None,
        timestamp: vh3.String = None,
        sync: typing.Sequence[
            typing.Union[
                vh3.Binary,
                vh3.Executable,
                vh3.FMLDumpParse,
                vh3.FMLFormula,
                vh3.FMLFormulaSerpPrefs,
                vh3.FMLPool,
                vh3.FMLPrs,
                vh3.FMLSerpComparison,
                vh3.FMLWizards,
                vh3.File,
                vh3.HTML,
                vh3.HiveTable,
                vh3.Image,
                vh3.JSON,
                vh3.MRDirectory,
                vh3.MRFile,
                vh3.MRTable,
                vh3.TSV,
                vh3.Text,
                vh3.XML,
            ]
        ] = ()
) -> vh3.TSV:
    """
    MR Read TSV

    Reads an MR table (or a part of it) into TSV (tab-separated values) file

    :param yt_token: YT Token:
      ID of Nirvana Secret with YT access token (https://nda.ya.ru/3RSzVU).
      Guide to Nirvana Secrets: https://nda.ya.ru/3RSzWZ
    :param columns: Output Columns:
      Columns to output. At least one column MUST be specified
    :param table: MR Table to read data from
    :param start_row: Start Row Index:
      Inclusive, counting from 0
    :param end_row: End Row Index:
      Exclusive, counting from 0
    :param missing_value_mode: Missing Value Mode:
      Use "Fail" if all columns are required; and "Print Sentinel" if at least some columns are optional.
      Using "Skip Row" is not recommended.
    :param missing_value_sentinel: Missing Value Sentinel:
      Used only in "Print Sentinel" missing value mode. If not specified, assumed to be "\" (empty string)
    :param output_escaping: Escape '\n', '\t' and '\\'
      It is recommended to enable escaping unless you have <key[,subkey],value> tables with tab-separated data in the `value' column
    :param output_escaping_symbol: Escaping Symbol:
      '\\' will be used if no value is specified.
      This option is ignored if "Escape '
      ', '\t' and '\\'" is off
    :param timestamp: Timestamp:
      Set a recent, not previously used timestamp to force "MR Read" to run even if `table' was already read once
    :param sync: Synchronization input. Can be used *e.g.* to read the table only after specific data is ready
    """
    raise NotImplementedError("Write your local execution stub here")


@vh3.decorator.external_operation("https://nirvana.yandex-team.ru/operation/6202577c-d354-40b8-9131-d5af9e7f8635")
@vh3.decorator.nirvana_output_names("output_table")
@vh3.decorator.nirvana_names_transformer(vh3.name_transformers.snake_to_dash, options=True)
def tsv_to_mr_table(
        *,
        yt_token: vh3.Secret = vh3.Factory(lambda: vh3.context.yt_token),
        mr_account: vh3.String = vh3.Factory(lambda: vh3.context.mr_account),
        format: vh3.String,
        tsv_data: vh3.TSV,
        yt_pool: vh3.String = None,
        ttl: vh3.Integer = 360,
        max_ram: vh3.Integer = 100,
        mr_default_cluster: vh3.String = "hahn"
) -> vh3.MRTable:
    """
    TSV to MR Table

    **Назначение операции**

    Загружает данные в формате TSV на YT.

    **Описание входов**

    Исходные данные в формате TSV File подаются на вход "tsv_data".

    **Описание выходов**

    Путь к таблице/папкам на YT в формате MR Table возвращается на выход "output_table".

    **Ограничения**

    Для работы операции необходимо указание YT-токена и MR-аккаунта в опциях `YT Token` и `MR Account`.

    :param yt_token: YT Token:
      ID of Nirvana Secret with YT access token (https://nda.ya.ru/3RSzVU).
      Guide to Nirvana Secrets: https://nda.ya.ru/3RSzWZ
    :param mr_account: MR Account:
      MR Account Name.
      By default, output tables and directories will be created in some subdirectory of home/<MR Account>/<workflow owner>/nirvana
    :param format: Format
    :param yt_pool: YT Pool:
      Pool used by YT scheduler. Leave blank to use default pool.
      This option has no effect on YaMR.
    :param mr_default_cluster: MR Cluster
    """
    raise NotImplementedError("Write your local execution stub here")


@vh3.decorator.external_operation("https://nirvana.yandex-team.ru/operation/fc0a47c0-6f3c-4b95-8ea3-bfcff47b7015")
@vh3.decorator.nirvana_output_names("output")
@vh3.decorator.nirvana_names_transformer(vh3.name_transformers.snake_to_dash, options=True)
def cut_tsv_header(*, input: vh3.TSV, ttl: vh3.Integer = 360, max_ram: vh3.Integer = 100) -> vh3.TSV:
    """
    Cut TSV header
    """
    raise NotImplementedError("Write your local execution stub here")


@vh3.decorator.external_operation("https://nirvana.yandex-team.ru/operation/351d58e9-06fc-4c92-816a-51d38af906e3")
@vh3.decorator.nirvana_output_names("dataset")
@vh3.decorator.nirvana_names_transformer(vh3.name_transformers.snake_to_dash, options=True)
def convert_to_fml(
        *,
        target: vh3.Integer,
        features: vh3.Integer,
        executable: vh3.Executable,
        dataset: vh3.TSV,
        ttl: vh3.Integer = 360,
        max_ram: vh3.Integer = 100,
        utterance: vh3.Integer = None,
        grouping: vh3.Integer = None
) -> vh3.TSV:
    """
    Convert to FML

    :param target:
      Target column
    :param features:
      Features column
    :param utterance:
      Utterance column
    :param grouping:
      Grouping column
    """
    raise NotImplementedError("Write your local execution stub here")


@vh3.decorator.external_operation("https://nirvana.yandex-team.ru/operation/dfd8837e-080e-11e7-a161-5b4753cafa85")
@vh3.decorator.nirvana_names(factor_slices="factor_slices", pool_files="poolFiles", parent_pool="parentPool")
@vh3.decorator.nirvana_output_names("fmlPool")
@vh3.decorator.nirvana_names_transformer(vh3.name_transformers.snake_to_dash, options=True)
def upload_pool_to_fml(
        *,
        title: vh3.String,
        comment: vh3.String = None,
        tags: vh3.MultipleStrings = (),
        category: vh3.String = None,
        search_type: vh3.Enum[
            typing.Literal[
                "WEB",
                "IMG",
                "VIDEO",
                "MUSIC",
                "NEWS",
                "PEOPLESEARCH",
                "OTHER",
                "SELECTION_RANK",
                "PERSONALIZATION",
                "SNIPPETS",
                "EXPORT_RANK",
                "MASCOT",
                "BLENDER",
                "RECOMMENDATIONS",
                "CLICK_ADDITION_WEB",
                "PPO",
                "MOBILE_FEATURES",
                "SEARCH_ADVERTISING",
                "MARKET",
                "GEO",
                "COLLECTIONS_BOARDS",
            ]
        ] = "WEB",
        split_mode: vh3.Enum[
            typing.Literal["NONE", "KOSHER_IDS", "MD5_HASH", "RANDOM_PART", "TEST_POOL", "RANDOM_QUERIES"]
        ] = "NONE",
        kosher_pool_group_id: vh3.String = None,
        test_percentage: vh3.Number = None,
        seed_source: vh3.Enum[typing.Literal["MD5_OF_POOL_FILE", "CUSTOM"]] = "MD5_OF_POOL_FILE",
        seed: vh3.Integer = None,
        channels: vh3.MultipleEnums[typing.Literal["EMAIL", "SMS"]] = (),
        events: vh3.MultipleEnums[typing.Literal["COMPLETED", "FAILED"]] = (),
        cc_users: vh3.MultipleStrings = (),
        data_ttl_days: vh3.Integer = 90,
        timestamp: vh3.String = None,
        features: vh3.TSV = None,
        queries: vh3.TSV = None,
        factor_slices: vh3.TSV = None,
        pool_files: typing.Sequence[vh3.TSV] = (),
        parent_pool: vh3.FMLPool = None
) -> vh3.FMLPool:
    """
    Upload Pool to FML

    Uploads pool containing of specified files to FML.

    :param title: Title:
    :param comment: Comment:
    :param tags: Tags:
    :param category: Category:
      Comma-separated list of categories, e.g. ru, ru.latin. Choose categories from the Formulator page or enter new ones
    :param search_type: Search Type:
      Factor indices are mapped to names using the chosen Search Type.
      For a custom index, upload factor_mapping.tsv (pool index\t factor index) file together with pool files to get correct factor index-name mapping.
    :param split_mode: Split Learn/Test:
      Formulas trained on split pool can be tested consistently with production formulas
    :param kosher_pool_group_id: Kosher PoolGroup for Test IDs:
      Only used in "Yes (Kosher IDs)" split mode
    :param test_percentage: Percentage of Test:
      Only used in "Yes (Random Part)" and "Yes (Random Queries)" split modes
    :param seed_source: Seed Source:
      Only used in "Yes (Random Part)" and "Yes (Random Queries)" split modes
    :param seed: Custom Seed:
      Only used in "Yes (Random Part)" and "Yes (Random Queries)" split modes if "Custom" seed source is specified.
    :param channels: Notify by:
    :param events: When:
    :param cc_users: CC Users:
    :param data_ttl_days: Data TTL days:
      Set -1 for unlimited TTL
    :param timestamp: Timestamp:
      Use for cache invalidation
    :param pool_files: Other pool files
    """
    raise NotImplementedError("Write your local execution stub here")


@vh3.decorator.external_operation("https://nirvana.yandex-team.ru/operation/45712e31-b334-4857-97b2-da0ff7f4b0e6")
@vh3.decorator.nirvana_names(
    description_title="description.title",
    description_comment="description.comment",
    description_tags="description.tags",
    matrixnet_options="matrixnet-options",
    trained_on_complete_pool="trained-on-complete-pool",
    notifications_channels="notifications.channels",
    notifications_events="notifications.events",
    notifications_cc_users="notifications.cc-users",
)
@vh3.decorator.nirvana_output_names("fmlFormula")
@vh3.decorator.nirvana_names_transformer(vh3.name_transformers.snake_to_camel)
def upload_formula_to_fml(
        *,
        description_title: vh3.String,
        matrixnet_info: vh3.Binary,
        description_comment: vh3.String = None,
        description_tags: vh3.MultipleStrings = (),
        matrixnet_options: vh3.String = None,
        trained_on_complete_pool: vh3.Boolean = False,
        notifications_channels: vh3.MultipleEnums[typing.Literal["EMAIL", "SMS"]] = (),
        notifications_events: vh3.MultipleEnums[typing.Literal["COMPLETED", "FAILED"]] = (),
        notifications_cc_users: vh3.MultipleStrings = (),
        learn_pool: vh3.FMLPool = None
) -> vh3.FMLFormula:
    """
    Upload Formula to FML

    Uploads matrixnet formula created in Nirvana to FML.

    :param description_title: Title:
    :param matrixnet_info: matrixnet.info
    :param description_comment: Comment:
    :param description_tags: Tags:
    :param matrixnet_options: Matrixnet args:
    :param trained_on_complete_pool: Trained on complete pool:
      Formulas trained on LEARN can be tested consistently with production formulas
    :param notifications_channels: Notify by:
    :param notifications_events: When:
    :param notifications_cc_users: CC Users:
    """
    raise NotImplementedError("Write your local execution stub here")


@vh3.decorator.external_operation("https://nirvana.yandex-team.ru/operation/e1c11fb7-e0b6-4441-9088-b748c28aeac6")
@vh3.decorator.nirvana_output_names("output_json")
@vh3.decorator.nirvana_names_transformer(vh3.name_transformers.snake_to_dash, options=True)
def fml_formula_to_json(*, formula: vh3.FMLFormula, ttl: vh3.Integer = 360, max_ram: vh3.Integer = 100) -> vh3.JSON:
    """
    Fml formula to json
    """
    raise NotImplementedError("Write your local execution stub here")


class GetMrTableByFmlPoolOutput(typing.NamedTuple):
    table: vh3.MRTable
    directory: vh3.MRDirectory


@vh3.decorator.external_operation("https://nirvana.yandex-team.ru/operation/5dad5d8e-2682-11e6-a28b-5268111f66a3")
@vh3.decorator.nirvana_names_transformer(vh3.name_transformers.snake_to_dash, options=True)
def get_mr_table_by_fml_pool(
        *,
        pool: vh3.FMLPool,
        table: vh3.Enum[
            typing.Literal[
                "directory",
                "features",
                "learn",
                "test",
                "queries",
                "ratings",
                "query_timestamps",
                "factor_slices",
                "features.pairs",
                "factor_names",
                "requests.WebTier0",
                "requests.PlatinumTier0",
            ]
        ] = "features",
        preferred_mr_runtime_type: vh3.Enum[typing.Literal["YAMR", "YT"]] = "YAMR",
        fail_on_runtime_mismatch: vh3.Boolean = False
) -> GetMrTableByFmlPoolOutput:
    """
    Get MR Table by FML Pool

    Get FML Pool table for specified file on specified mapreduce cluster (YAMR or YT). If table does not exist, operation **will be failed**.

    :param table: Table:
    :param preferred_mr_runtime_type: Preferred MR runtime:
    :param fail_on_runtime_mismatch: Fail on MR runtime mismatch:
    """
    raise NotImplementedError("Write your local execution stub here")


class ResolveFactorsOutput(typing.NamedTuple):
    tested_factors: vh3.Text
    ignored_factors: vh3.Text
    factors_config: vh3.Text
    comments_file: vh3.Text
    resolved_factors_info: vh3.JSON


@vh3.decorator.external_operation("https://nirvana.yandex-team.ru/operation/22425816-3afe-420d-9fd2-18daf9238bc3")
@vh3.decorator.nirvana_names(pool_path="PoolPath", config_path="ConfigPath")
@vh3.decorator.nirvana_output_names(
    tested_factors="TestedFactors",
    ignored_factors="IgnoredFactors",
    factors_config="FactorsConfig",
    comments_file="CommentsFile",
    resolved_factors_info="ResolvedFactorsInfo",
)
@vh3.decorator.nirvana_names_transformer(vh3.name_transformers.snake_to_dash, options=True)
def resolve_factors(
        *,
        yt_token: vh3.Secret = vh3.Factory(lambda: vh3.context.yt_token),
        for_catboost: vh3.Boolean,
        ttl: vh3.Integer = 360,
        max_ram: vh3.Integer = 512,
        yt_pool: vh3.String = "",
        mr_transaction_policy: vh3.Enum[typing.Literal["MANUAL", "AUTO"]] = "MANUAL",
        mr_default_cluster: vh3.String = "hahn",
        expression: vh3.String = None,
        optional_expression: vh3.String = None,
        timestamp: vh3.String = None,
        list_of_expressions: vh3.MultipleStrings = (),
        intersected_factors_policy: vh3.Enum[typing.Literal["DEFAULT", "TEST", "IGNORE"]] = "DEFAULT",
        fail_on_empty_feature_set: vh3.Boolean = False,
        pool_path: vh3.MRDirectory = None,
        config_path: vh3.Text = None
) -> ResolveFactorsOutput:
    """
    Resolve factors

    Resolve factors like fml.
    Syntax description https://a.yandex-team.ru/arc/trunk/arcadia/quality/relev_tools/resolve_factors/Readme.md.

    :param yt_token: YT Token:
      ID of Nirvana Secret with YT access token (https://nda.ya.ru/3RSzVU).
      Guide to Nirvana Secrets: https://nda.ya.ru/3RSzWZ
    :param yt_pool: YT Pool:
      Pool used by YT scheduler. Leave blank to use default pool.
      This option has no effect on YaMR.
    :param mr_transaction_policy: MR transaction policy:
    """
    raise NotImplementedError("Write your local execution stub here")


class Python3TextProcessorTextInputJsonTextOutputsOutput(typing.NamedTuple):
    output1: vh3.JSON
    output2: vh3.Text


@vh3.decorator.external_operation("https://nirvana.yandex-team.ru/operation/718dcf83-9d34-49c3-91e8-df04208a493c")
@vh3.decorator.nirvana_names_transformer(vh3.name_transformers.snake_to_dash, options=True)
def python3_text_processor_text_input_json_text_outputs(
        *,
        max_ram: vh3.Integer = 512,
        cpu_cores: vh3.Integer = 1,
        code: vh3.String = 'input1 == "text in input text file"\n\noutput1 = {1: "a", 2: "b"}  # deserialized output json object\noutput2 = "smth"',
        input1: typing.Union[vh3.File, vh3.HTML, vh3.JSON, vh3.TSV, vh3.Text, vh3.XML] = None
) -> Python3TextProcessorTextInputJsonTextOutputsOutput:
    """
    Python3 Text Processor (text input, json + text outputs)

    :param cpu_cores:
      Number of CPU cores used by the job
    """
    raise NotImplementedError("Write your local execution stub here")


@vh3.decorator.external_operation("https://nirvana.yandex-team.ru/operation/513f1bdd-066e-4238-bf28-4c5828899fae")
@vh3.decorator.nirvana_names(import_="import", global_="global", input_type="input_type", output_type="output_type")
@vh3.decorator.nirvana_output_names("output")
@vh3.decorator.nirvana_names_transformer(vh3.name_transformers.snake_to_dash, options=True)
def python3_any_to_json(
        *,
        input_type: vh3.Enum[
            typing.Literal["file", "none", "txt-gen", "txt-mem", "tsv-gen", "tsv-mem", "json-gen", "json-mem", "xml"]
        ],
        ttl: vh3.Integer = 360,
        max_ram: vh3.Integer = 1024,
        max_disk: vh3.Integer = 16384,
        import_: vh3.String = None,
        global_: vh3.String = None,
        body: vh3.String = "return v",
        output_type: vh3.Enum[typing.Literal["json", "json-utf8", "json-pretty", "txt", "file", "none"]] = "json",
        job_environments: vh3.MultipleStrings = (),
        yt_token: vh3.Secret = vh3.Factory(lambda: vh3.context.yt_token),
        mr_account: vh3.String = vh3.Factory(lambda: vh3.context.mr_account),
        yt_pool: vh3.String = None,
        mr_default_cluster: vh3.Enum[typing.Literal["hahn", "freud", "marx", "hume", "arnold"]] = "hahn",
        input: typing.Union[
            vh3.Binary,
            vh3.Executable,
            vh3.HTML,
            vh3.Image,
            vh3.JSON,
            vh3.MRDirectory,
            vh3.MRFile,
            vh3.MRTable,
            vh3.TSV,
            vh3.Text,
            vh3.XML,
        ] = None
) -> vh3.JSON:
    """
    Python3 any to json

    Transform input data using Python3

    Documentation and examples: https://wiki.yandex-team.ru/mlmarines/operations/pythontransform

    =======

    The operation was created by dj/nirvana/nirvana_make tool.

    See https://nda.ya.ru/3UUaov for updating the operation.

    Please, do not update the operation manually!


    Update command:

    dj/nirvana/nirvana_make/operation_make --update data/python_transform/py3_any_to_json --root yt://hahn/home/mlmarines/common/nirvana_make/operations --import dj/nirvana/operations


    Svn info:

        URL: svn://arcadia.yandex.ru/arc/trunk/arcadia

        Last Changed Rev: 7299768

        Last Changed Author: arumyan

        Last Changed Date: 2020-09-04T06:33:11.142785Z


    Other info:

        Build by: sandbox

        Top src dir: /place/sandbox-data/tasks/9/8/765502089/__FUSE/mount_point_fa14edbc-5606-4ccd-bb38-2ccf1ca086c6

        Top build dir: /place/sandbox-data/tasks/9/8/765502089/__FUSE/mount_point_fa14edbc-5606-4ccd-bb38-2ccf1ca086c6

        Hostname: linux-ubuntu-12-04-precise

        Host information:

            Linux linux-ubuntu-12-04-precise 4.9.151-35 #1 SMP Thu Jan 17 16:21:25 UTC 2019 x86_64 x86_64 x86_64 GNU/Linux



    :param input_type: Input type
      Type of input
    :param import_: Import modules
      Modules to import, comma separated
    :param global_: Global definitions
      Global definitions, multiline allowed
    :param body: Function(v) body
      Body for function
    :param output_type: Output type
      Type of output
    :param yt_token: YT Token:
      ID of Nirvana Secret with YT access token (https://nda.ya.ru/3RSzVU).
      Guide to Nirvana Secrets: https://nda.ya.ru/3RSzWZ
    :param mr_account: MR Account:
      MR Account Name.
      By default, output tables and directories will be created in some subdirectory of home/<MR Account>/<workflow owner>/nirvana
    :param yt_pool: YT Pool:
      Pool used by YT scheduler. Leave blank to use default pool.
      This option has no effect on YaMR.
    :param mr_default_cluster: Default YT cluster:
      Default YT cluster
    :param input: Input
    """
    raise NotImplementedError("Write your local execution stub here")


@vh3.decorator.external_operation("https://nirvana.yandex-team.ru/operation/3cc08c91-a171-441a-893d-1fa8c3d8acf2")
@vh3.decorator.nirvana_names(import_="import", global_="global", input_type="input_type", output_type="output_type")
@vh3.decorator.nirvana_output_names("output")
@vh3.decorator.nirvana_names_transformer(vh3.name_transformers.snake_to_dash, options=True)
def python3_any_to_tsv(
        *,
        input_type: vh3.Enum[
            typing.Literal["file", "none", "txt-gen", "txt-mem", "tsv-gen", "tsv-mem", "json-gen", "json-mem", "xml"]
        ],
        ttl: vh3.Integer = 360,
        max_ram: vh3.Integer = 1024,
        max_disk: vh3.Integer = 16384,
        import_: vh3.String = None,
        global_: vh3.String = None,
        body: vh3.String = "return v",
        output_type: vh3.Enum[typing.Literal["tsv", "txt", "file", "none"]] = "tsv",
        job_environments: vh3.MultipleStrings = (),
        yt_token: vh3.Secret = vh3.Factory(lambda: vh3.context.yt_token),
        mr_account: vh3.String = vh3.Factory(lambda: vh3.context.mr_account),
        yt_pool: vh3.String = None,
        mr_default_cluster: vh3.Enum[typing.Literal["hahn", "freud", "marx", "hume", "arnold"]] = "hahn",
        input: typing.Union[
            vh3.Binary,
            vh3.Executable,
            vh3.HTML,
            vh3.Image,
            vh3.JSON,
            vh3.MRDirectory,
            vh3.MRFile,
            vh3.MRTable,
            vh3.TSV,
            vh3.Text,
            vh3.XML,
        ] = None
) -> vh3.TSV:
    """
    Python3 any to tsv

    Transform input data using Python3

    Documentation and examples: https://wiki.yandex-team.ru/mlmarines/operations/pythontransform

    =======

    The operation was created by dj/nirvana/nirvana_make tool.

    See https://nda.ya.ru/3UUaov for updating the operation.

    Please, do not update the operation manually!


    Update command:

    dj/nirvana/nirvana_make/operation_make --update data/python_transform/py3_any_to_tsv --root yt://hahn/home/mlmarines/common/nirvana_make/operations --import dj/nirvana/operations


    Svn info:

        URL: svn://arcadia.yandex.ru/arc/trunk/arcadia

        Last Changed Rev: 7299768

        Last Changed Author: arumyan

        Last Changed Date: 2020-09-04T06:33:11.142785Z


    Other info:

        Build by: sandbox

        Top src dir: /place/sandbox-data/tasks/9/8/765502089/__FUSE/mount_point_fa14edbc-5606-4ccd-bb38-2ccf1ca086c6

        Top build dir: /place/sandbox-data/tasks/9/8/765502089/__FUSE/mount_point_fa14edbc-5606-4ccd-bb38-2ccf1ca086c6

        Hostname: linux-ubuntu-12-04-precise

        Host information:

            Linux linux-ubuntu-12-04-precise 4.9.151-35 #1 SMP Thu Jan 17 16:21:25 UTC 2019 x86_64 x86_64 x86_64 GNU/Linux



    :param input_type: Input type
      Type of input
    :param import_: Import modules
      Modules to import, comma separated
    :param global_: Global definitions
      Global definitions, multiline allowed
    :param body: Function(v) body
      Body for function
    :param output_type: Output type
      Type of output
    :param yt_token: YT Token:
      ID of Nirvana Secret with YT access token (https://nda.ya.ru/3RSzVU).
      Guide to Nirvana Secrets: https://nda.ya.ru/3RSzWZ
    :param mr_account: MR Account:
      MR Account Name.
      By default, output tables and directories will be created in some subdirectory of home/<MR Account>/<workflow owner>/nirvana
    :param yt_pool: YT Pool:
      Pool used by YT scheduler. Leave blank to use default pool.
      This option has no effect on YaMR.
    :param mr_default_cluster: Default YT cluster:
      Default YT cluster
    :param input: Input
    """
    raise NotImplementedError("Write your local execution stub here")


class PythonExecutorOutput(typing.NamedTuple):
    out_json: vh3.JSON
    out_text: vh3.Text
    out_tsv: vh3.TSV
    out_bin: vh3.Binary


@vh3.decorator.external_operation("https://nirvana.yandex-team.ru/operation/328f78e9-059d-11e7-a873-0025909427cc")
@vh3.decorator.nirvana_names_transformer(vh3.name_transformers.snake_to_dash, options=True)
def python_executor(
        *,
        ttl: vh3.Integer = 360,
        max_ram: vh3.Integer = 100,
        script_url: vh3.String = None,
        input_params: vh3.String = None,
        infiles: typing.Sequence[typing.Union[vh3.Binary, vh3.Executable, vh3.File, vh3.JSON, vh3.TSV, vh3.Text]] = (),
        script: typing.Union[vh3.File, vh3.Text] = None
) -> PythonExecutorOutput:
    """
    Python executor

    Execute auxiliary python code.

    :param input_params:
      String parameter which will be passed as -params INPUT-PARAMS if specified.
    :param infiles: Input files, will be passed as -i FILENAME.
    :param script: File with python script to be executed. Has more priority than corresponding 'Option'.
    """
    raise NotImplementedError("Write your local execution stub here")


class PythonExecutorWithScriptOutput(typing.NamedTuple):
    out_json: vh3.JSON
    out_text: vh3.Text
    out_tsv: vh3.TSV
    out_bin: vh3.Binary


@vh3.decorator.external_operation("https://nirvana.yandex-team.ru/operation/61f09789-b3cf-49d0-8d19-eeb0314005d9")
@vh3.decorator.nirvana_names(script="Script")
def python_executor_with_script(
        *,
        script: vh3.String,
        infiles: typing.Sequence[typing.Union[vh3.Binary, vh3.Executable, vh3.File, vh3.JSON, vh3.TSV, vh3.Text]] = ()
) -> PythonExecutorWithScriptOutput:
    """
    Python executor with script

    :param infiles: Input files, will be passed as -i FILENAME.
    """
    raise NotImplementedError("Write your local execution stub here")


@vh3.decorator.external_operation("https://nirvana.yandex-team.ru/operation/cf92f7ad-a759-4087-8163-98793b70e0d5")
@vh3.decorator.nirvana_output_names("output")
@vh3.decorator.nirvana_names_transformer(vh3.name_transformers.snake_to_dash, options=True)
def skip_empty_json(*, input: vh3.JSON, ttl: vh3.Integer = 360, max_ram: vh3.Integer = 100) -> vh3.JSON:
    """
    Skip empty json

    Skip empty branch
    """
    raise NotImplementedError("Write your local execution stub here")


@vh3.decorator.external_operation("https://nirvana.yandex-team.ru/operation/dfd39fac-ef76-4b25-b1fe-53ea30e9b8f6")
@vh3.decorator.nirvana_output_names("output")
def json_unwrap_single_object(
        *,
        on_empty_input: vh3.Enum[typing.Literal["FAIL", "WRITE_EMPTY_OBJECT"]],
        on_multi_input: vh3.Enum[typing.Literal["FAIL", "WRITE_FIRST_OBJECT"]],
        input: vh3.JSON
) -> vh3.JSON:
    """
    Json Unwrap Single Object

    **Назначение операции**

    Извлекает из JSON-массива один JSON-объект.

    **Описание входов**

    JSON-массив подается на вход "input".

    **Описание выходов**

    Результат в формате JSON подается на выход "output".

    **Ограничения**

    Не предусмотрены.

    :param on_empty_input: on empty input
    :param on_multi_input: on multi input
    """
    raise NotImplementedError("Write your local execution stub here")


@vh3.decorator.external_operation("https://nirvana.yandex-team.ru/operation/cda33ba8-3580-11e7-89a6-0025909427cc")
@vh3.decorator.nirvana_output_names("output")
@vh3.decorator.nirvana_names_transformer(vh3.name_transformers.snake_to_dash, options=True)
def merge_json_files(
        *,
        input: typing.Sequence[vh3.JSON],
        ttl: vh3.Integer = 360,
        max_ram: vh3.Integer = 100
) -> vh3.JSON:
    """
    Merge json files
    """
    raise NotImplementedError("Write your local execution stub here")


class BuildArcadiaProjectOutput(typing.NamedTuple):
    arcadia_project: vh3.Executable
    sandbox_task_id: vh3.Text


@vh3.decorator.external_operation("https://nirvana.yandex-team.ru/operation/dd4b5735-1ee7-497d-91fc-b81ba8b510fc")
@vh3.decorator.nirvana_output_names(arcadia_project="ARCADIA_PROJECT", sandbox_task_id="SANDBOX_TASK_ID")
def build_arcadia_project(
        *,
        targets: vh3.String,
        arts: vh3.String,
        arcadia_url: vh3.String = "arcadia:/arc/trunk/arcadia",
        arcadia_revision: vh3.Integer = None,
        checkout_arcadia_from_url: vh3.String = None,
        build_type: vh3.Enum[
            typing.Literal["release", "debug", "profile", "coverage", "relwithdebinfo", "valgrind", "valgrind-release"]
        ] = "release",
        arts_source: vh3.String = None,
        result_single_file: vh3.Boolean = False,
        definition_flags: vh3.String = None,
        sandbox_oauth_token: vh3.Secret = None,
        arcadia_patch: vh3.String = None,
        owner: vh3.String = None,
        use_aapi_fuse: vh3.Boolean = True,
        use_arc_instead_of_aapi: vh3.Boolean = False,
        aapi_fallback: vh3.Boolean = False,
        kill_timeout: vh3.Integer = None,
        sandbox_requirements_disk: vh3.Integer = None,
        sandbox_requirements_ram: vh3.Integer = None,
        sandbox_requirements_platform: vh3.Enum[
            typing.Literal[
                "Any",
                "darwin-20.4.0-x86_64-i386-64bit",
                "linux",
                "linux_ubuntu_10.04_lucid",
                "linux_ubuntu_12.04_precise",
                "linux_ubuntu_14.04_trusty",
                "linux_ubuntu_16.04_xenial",
                "linux_ubuntu_18.04_bionic",
                "osx",
                "osx_10.12_sierra",
                "osx_10.13_high_sierra",
                "osx_10.14_mojave",
                "osx_10.15_catalina",
                "osx_10.16_big_sur",
            ]
        ] = None,
        checkout: vh3.Boolean = False,
        clear_build: vh3.Boolean = True,
        strip_binaries: vh3.Boolean = False,
        lto: vh3.Boolean = False,
        thinlto: vh3.Boolean = False,
        musl: vh3.Boolean = False,
        use_system_python: vh3.Boolean = False,
        target_platform_flags: vh3.String = None,
        javac_options: vh3.String = None,
        ya_yt_proxy: vh3.String = None,
        ya_yt_dir: vh3.String = None,
        ya_yt_token_vault_owner: vh3.String = None,
        ya_yt_token_vault_name: vh3.String = None,
        result_rt: vh3.String = None,
        timestamp: vh3.String = None
) -> BuildArcadiaProjectOutput:
    """
    Build Arcadia Project

    Launches YA_MAKE task in Sandbox for provided target and downloads requested artifact.

    :param targets: Target
      Multiple targets with ";" are not allowed
    :param arts: Build artifact
      Multiple artifacts with ";" and custom destination directory with "=" are not allowed
    :param arcadia_url: Svn url for arcadia
      Should not contain revision
    :param arcadia_revision: Arcadia Revision
    :param checkout_arcadia_from_url: Full SVN url for arcadia (Overwrites base URL and revision, use @revision to fix revision)
    :param build_type: Build type
    :param arts_source: Source artifacts (semicolon separated pairs path[=destdir])
      Какие файлы из Аркадии поместить в отдельный ресурс (формат тот же, что и у build artifacts)
    :param result_single_file: Result is a single file
    :param definition_flags: Definition flags
      For example "-Dkey1=val1 ... -DkeyN=valN"
    :param sandbox_oauth_token: Sandbox OAuth token
      To run task on behalf of specific user
    :param arcadia_patch: Apply patch
      Diff file rbtorrent, paste.y-t.ru link or plain text. Doc: https://nda.ya.ru/3QTTV4
    :param owner: Custom sandbox task owner (should be used only with OAuth token)
      OAuth token owner should be a member of sandbox group
    :param use_aapi_fuse: Use arcadia-api fuse
    :param use_arc_instead_of_aapi: Use arc fuse instead of aapi
    :param aapi_fallback: Fallback to svn/hg if AAPI services are temporary unavailable
    :param kill_timeout: Kill Timeout (seconds)
    :param sandbox_requirements_disk: Disk requirements in Mb
    :param sandbox_requirements_ram: RAM requirements in Mb
    :param sandbox_requirements_platform: Platform
    :param checkout: Run ya make with --checkout
    :param clear_build: Clear build
    :param strip_binaries: Strip result binaries
    :param lto: Build with LTO
    :param thinlto: Build with ThinLTO
    :param musl: Build with musl-libc
    :param use_system_python: Use system Python to build python libraries
    :param target_platform_flags: Target platform flags (only for cross-compilation)
    :param javac_options: Javac options (semicolon separated)
    :param ya_yt_proxy: YT store proxy
    :param ya_yt_dir: YT store cypress path
    :param ya_yt_token_vault_owner: YT token vault owner
    :param ya_yt_token_vault_name: YT token vault name
    :param result_rt: Result resource type
    :param timestamp: Timestamp
    """
    raise NotImplementedError("Write your local execution stub here")


@vh3.decorator.external_operation("https://nirvana.yandex-team.ru/operation/2bf2d9c9-bc2d-4945-a10f-fe1e47b08dd0")
@vh3.decorator.nirvana_names(cache_sync="cache_sync")
@vh3.decorator.nirvana_output_names("output_table")
@vh3.decorator.nirvana_names_transformer(vh3.name_transformers.snake_to_dash, options=True)
def build_mm_target(
        *,
        yt_token: vh3.Secret = vh3.Factory(lambda: vh3.context.yt_token),
        mr_account: vh3.String = vh3.Factory(lambda: vh3.context.mr_account),
        executable: vh3.Executable,
        input_table: vh3.MRTable,
        yt_pool: vh3.String = None,
        ttl: vh3.Integer = 360,
        max_ram: vh3.Integer = 100,
        classification_bias: vh3.Boolean = False,
        no_search_bias: vh3.Boolean = False,
        debug_target: vh3.Boolean = False,
        search_from_vins: vh3.Boolean = False,
        search_from_toloka: vh3.Boolean = False,
        important_parts: vh3.Boolean = False,
        ignore_gc: vh3.Boolean = False,
        raw_scores: vh3.Boolean = False,
        cache_sync: vh3.String = None,
        disable_vins_bias: vh3.Boolean = False,
        add_gc: vh3.Boolean = False,
        scenarios_list: vh3.String,
        client: ClientTypeEnum,
        additional_flags: vh3.String = None,
) -> vh3.MRTable:
    """
    Build MM target

    Added search_from_vins and important_parts

    :param yt_token: YT Token:
      ID of Nirvana Secret with YT access token (https://nda.ya.ru/3RSzVU).
      Guide to Nirvana Secrets: https://nda.ya.ru/3RSzWZ
    :param mr_account: MR Account:
      MR Account Name.
      By default, output tables and directories will be created in some subdirectory of home/<MR Account>/<workflow owner>/nirvana
    :param yt_pool: YT Pool:
      Pool used by YT scheduler. Leave blank to use default pool.
      This option has no effect on YaMR.
    :param classification_bias:
      Try boost target with classification markup
    """
    raise NotImplementedError("Write your local execution stub here")


class CatBoostTrainOutput(typing.NamedTuple):
    eval_result: vh3.TSV
    model_bin: vh3.Binary
    fstr: vh3.TSV
    learn_error_log: vh3.TSV
    test_error_log: vh3.TSV
    training_log_json: vh3.JSON
    plots_html: vh3.HTML
    snapshot_file: vh3.Binary
    tensorboard_log: vh3.Binary
    tensorboard_url: vh3.Text
    training_options_json: vh3.JSON
    borders: vh3.TSV
    plots_png: vh3.Image


@vh3.decorator.external_operation("https://nirvana.yandex-team.ru/operation/b375f325-08b1-42c2-be68-82b126ac7001")
@vh3.decorator.nirvana_names(
    bootstrap_type="bootstrap_type",
    od_type="od_type",
    yt_pool="yt_pool",
    model_metadata="model_metadata",
    test_pairs="test_pairs",
    learn_group_weights="learn_group_weights",
    test_group_weights="test_group_weights",
    catboost_binary="catboost_binary",
    params_file="params_file",
    snapshot_file="snapshot_file",
    baseline_model="baseline_model",
    learn_baseline="learn_baseline",
    test_baseline="test_baseline",
    pool_metainfo="pool_metainfo",
)
@vh3.decorator.nirvana_output_names(
    eval_result="eval_result",
    model_bin="model.bin",
    learn_error_log="learn_error.log",
    test_error_log="test_error.log",
    training_log_json="training_log.json",
    plots_html="plots.html",
    snapshot_file="snapshot_file",
    tensorboard_log="tensorboard.log",
    tensorboard_url="tensorboard_url",
    training_options_json="training_options.json",
    plots_png="plots.png",
)
@vh3.decorator.nirvana_names_transformer(vh3.name_transformers.snake_to_dash, options=True)
def cat_boost_train(
        *,
        learn: typing.Union[vh3.Binary, vh3.File, vh3.MRTable, vh3.TSV, vh3.Text],
        ttl: vh3.Integer = 3600,
        gpu_type: vh3.Enum[
            typing.Literal["NONE", "ANY", "CUDA_3_5", "CUDA_5_2", "CUDA_6_1", "CUDA_7_0", "CUDA_8_0"]
        ] = "NONE",
        restrict_gpu_type: vh3.Boolean = False,
        slaves: vh3.Integer = None,
        cpu_guarantee: vh3.Integer = 1600,
        use_catboost_builtin_quantizer: vh3.Boolean = False,
        loss_function: vh3.Enum[
            typing.Literal[
                "RMSE",
                "Logloss",
                "MAE",
                "CrossEntropy",
                "Quantile",
                "LogLinQuantile",
                "Poisson",
                "MAPE",
                "MultiClass",
                "MultiClassOneVsAll",
                "PairLogit",
                "YetiRank",
                "QueryRMSE",
                "QuerySoftMax",
                "YetiRankPairwise",
                "PairLogitPairwise",
                "QueryCrossEntropy",
                "LambdaMart",
                "StochasticFilter",
                "StochasticRank",
                "MultiRMSE",
                "Huber",
                "Lq",
                "Expectile",
            ]
        ] = "Logloss",
        loss_function_param: vh3.String = None,
        iterations: vh3.Integer = 1000,
        learning_rate: vh3.Number = None,
        ignored_features: vh3.String = None,
        l2_leaf_reg: vh3.Number = None,
        random_strength: vh3.Number = None,
        bootstrap_type: vh3.Enum[typing.Literal["Poisson", "Bayesian", "Bernoulli", "MVS", "No"]] = None,
        bagging_temperature: vh3.Number = None,
        subsample: vh3.Number = None,
        sampling_unit: vh3.Enum[typing.Literal["Object", "Group"]] = None,
        rsm: vh3.Number = None,
        leaf_estimation_method: vh3.Enum[typing.Literal["Newton", "Gradient", "Exact"]] = None,
        leaf_estimation_iterations: vh3.Integer = None,
        depth: vh3.Integer = None,
        seed: vh3.Integer = 0,
        create_tensorboard: vh3.Boolean = False,
        use_best_model: vh3.Boolean = True,
        od_type: vh3.Enum[typing.Literal["IncToDec", "Iter"]] = None,
        od_pval: vh3.Number = None,
        eval_metric: vh3.String = None,
        custom_metric: vh3.String = None,
        one_hot_max_size: vh3.Number = None,
        feature_border_type: vh3.Enum[
            typing.Literal["Median", "GreedyLogSum", "UniformAndQuantiles", "MinEntropy", "MaxLogSum"]
        ] = None,
        per_float_feature_quantization: vh3.String = None,
        border_count: vh3.Integer = None,
        target_border: vh3.Number = None,
        has_header: vh3.Boolean = False,
        delimiter: vh3.String = None,
        ignore_csv_quoting: vh3.Boolean = False,
        cv_type: vh3.Enum[typing.Literal["Classical", "Inverted"]] = None,
        cv_fold_index: vh3.Integer = None,
        cv_fold_count: vh3.Integer = None,
        fstr_type: vh3.Enum[typing.Literal["PredictionValuesChange", "LossFunctionChange"]] = None,
        prediction_type: vh3.Enum[typing.Literal["RawFormulaVal", "Probability", "Class"]] = None,
        output_columns: vh3.MultipleStrings = (),
        max_ctr_complexity: vh3.Integer = None,
        ctr_leaf_count_limit: vh3.Integer = None,
        simple_ctr: vh3.String = None,
        combinations_ctr: vh3.String = None,
        text_processing: vh3.String = None,
        args: vh3.String = None,
        yt_token: vh3.Secret = vh3.Factory(lambda: vh3.context.yt_token),
        yt_pool: vh3.String = None,
        model_metadata: vh3.MultipleStrings = (),
        inner_options_override: vh3.String = None,
        boost_from_average: vh3.String = "False",
        job_core_yt_token: vh3.Secret = "",
        test: typing.Union[vh3.Binary, vh3.File, vh3.MRTable, vh3.TSV, vh3.Text] = None,
        cd: typing.Union[vh3.Binary, vh3.File, vh3.MRTable, vh3.TSV, vh3.Text] = None,
        pairs: typing.Union[vh3.Binary, vh3.File, vh3.MRTable, vh3.TSV, vh3.Text] = None,
        test_pairs: typing.Union[vh3.Binary, vh3.File, vh3.MRTable, vh3.TSV, vh3.Text] = None,
        learn_group_weights: typing.Union[vh3.Binary, vh3.File, vh3.MRTable, vh3.TSV, vh3.Text] = None,
        test_group_weights: typing.Union[vh3.Binary, vh3.File, vh3.MRTable, vh3.TSV, vh3.Text] = None,
        catboost_binary: typing.Union[vh3.Binary, vh3.Executable] = None,
        params_file: typing.Union[vh3.JSON, vh3.Text] = None,
        snapshot_file: vh3.Binary = None,
        borders: typing.Union[vh3.File, vh3.TSV, vh3.Text] = None,
        baseline_model: vh3.Binary = None,
        learn_baseline: typing.Union[vh3.Binary, vh3.File, vh3.MRTable, vh3.TSV, vh3.Text] = None,
        test_baseline: typing.Union[vh3.Binary, vh3.File, vh3.MRTable, vh3.TSV, vh3.Text] = None,
        pool_metainfo: vh3.JSON = None
) -> CatBoostTrainOutput:
    """
    CatBoost: Train

    CatBoost is a machine learning algorithm that uses gradient boosting on decision trees

    [Documentation](https://doc.yandex-team.ru/english/ml/catboost/doc/nirvana-operations/catboost__nirvana__train-catboost.html)

    [Change log](https://a.yandex-team.ru/arc/trunk/arcadia/ml/nirvana/catboost/train/change_log.md)

    :param learn: Training dataset. [Documentaion](https://tech.yandex.com/catboost/doc/dg/concepts/input-data_values-file-docpage/).
    :param restrict_gpu_type:
      Request gpu type exactly as specified
    :param slaves: Number of slaves
      or null for auto selection
    :param use_catboost_builtin_quantizer: Use CatBoost builtin quantizer
    :param loss_function: Loss function
    :param loss_function_param: Loss function param
      e.g. `border=0.7` for Logloss
    :param iterations: Iterations
    :param learning_rate: Learning rate
      0.0314 is a good choice
    :param ignored_features: Ignored features
      List feature indices to disregard, e.g. `0:5-13:2`
    :param l2_leaf_reg: L2 leaf regularization coeff
      Any positive value
    :param random_strength: Random strength
      Coeff at std deviation of score randomization.
      NOT supported for: QueryCrossEntropy, YetiRankPairwise, PairLogitPairwise
    :param bootstrap_type: Bootstrap type
      Method for sampling the weights of objects.
    :param bagging_temperature: Bagging temperature
      For Bayesian bootstrap
    :param subsample: Sample rate for bagging
    :param sampling_unit: Sampling unit
    :param rsm: Rsm
      CPU-only. A value in range (0; 1]
    :param leaf_estimation_method: Leaf estimation method
    :param leaf_estimation_iterations: Leaf estimation iterations
    :param depth: Max tree depth
      Suggested < 10 on GPU version
    :param seed: Random seed
    :param create_tensorboard: Create tensorboard
    :param use_best_model: Use best model
    :param od_type: Overfitting detector type
    :param od_pval: Auto stop PValue
      For IncToDec: a small value, [1e-12...1e-2], is a good choice.
      Do not use this this with Iter type overfitting detector.
    :param eval_metric: Eval metric (for OD and best model)
      Metric name with params, e.g. `Quantile:alpha=0.3`
    :param custom_metric: Custom metric
      Written during training. E.g. `Quantile:alpha=0.1`
    :param one_hot_max_size: One-hot max size
      Use one-hot encoding for cat features with number of values <= this size.
      Ctrs are not calculated for such features.
    :param feature_border_type: Border type for num-features
    :param border_count: Border count for num-features
      Must be < 256 for GPU. May be large on CPU.
    :param target_border: Border for target binarization
    :param has_header: Has header
    :param delimiter: Delimiter
    :param cv_type: CV type
      Classical: train on many, test on 1, Inverted: train on 1, test on many
    :param cv_fold_index: CV fold index
    :param cv_fold_count: CV fold count
    :param fstr_type: Feature importance calculation type
    :param prediction_type: Prediction type
      only CPU version
    :param output_columns: Output columns
      Columns for eval file, e.g. SampleId RawFormulaVal #5:FeatureN
      > https://nda.ya.ru/3VodMf
    :param max_ctr_complexity: Max ctr complexity
      Maximum number of cat-features in ctr combinations.
    :param ctr_leaf_count_limit: ctr leaf count limit (CPU only)
    :param text_processing: text-processing
    :param args: additional arguments
    :param model_metadata: Model metadata key-value pairs
      Key-value pairs, quote key and value for reasonable results
    :param inner_options_override: Override options for inner train cube
      JSON string, e.g. {"master-max-ram":250000}
    :param boost_from_average:
      For RMSE: start training with approxes set to mean target
    :param test: Testing dataset. [Documentaion](https://tech.yandex.com/catboost/doc/dg/concepts/input-data_values-file-docpage/).
    :param cd: Column description. [Documentation](https://tech.yandex.com/catboost/doc/dg/concepts/input-data_column-descfile-docpage/).
    :param pairs: Training pairs. [Documentation](https://tech.yandex.com/catboost/doc/dg/concepts/input-data_pairs-description-docpage/).
    :param test_pairs: Testing pairs. [Documentation](https://tech.yandex.com/catboost/doc/dg/concepts/input-data_pairs-description-docpage/).
    :param learn_group_weights: Training query weights.

      Format:
      ```
      <group_id>\t<weight>\n
      ```

      Example:
      ```
      id1\t0.5
      id2\t0.7
      ```
    :param test_group_weights: Testing query weights.

      Format:
      ```
      <group_id>\t<weight>\n
      ```

      Example:
      ```
      id1\t0.5
      id2\t0.7
      ```
    :param catboost_binary: CatBoost binary.
      Build command:
      ```
      ya make trunk/arcadia/catboost/app/ -r --checkout -DHAVE_CUDA=yes -DCUDA_VERSION=9.0
      ```
    :param params_file: JSON file that contains the training parameters, for example:
      ```
      {
          "loss_function": "Logloss",
          "iterations": 400
      }
      ```
    :param snapshot_file: Snapshot file. Used for recovering training after an interruption.
    :param borders: File with borders description.

      Format:
      ```
      <feature_index(zero-based)>\t<border_value>\n
      ```

      Example:
      ```
      0\t0.5
      0\t1.0
      1\t0.7
      ```

      Note: File should be sorted by first column.
    :param learn_baseline: Baseline for learn
    :param test_baseline: Baseline for test (the first one, if many)
    :param pool_metainfo: Metainfo file with feature tags.
    """
    raise NotImplementedError("Write your local execution stub here")


class CatBoostApplyOutput(typing.NamedTuple):
    result: vh3.TSV
    output_yt_table: vh3.MRTable


@vh3.decorator.external_operation("https://nirvana.yandex-team.ru/operation/77c52797-7f8b-4037-8d7e-9b5d15dd881d")
@vh3.decorator.nirvana_names(
    yt_pool="yt_pool",
    model_bin="model.bin",
    catboost_external_binary="catboost_external_binary",
    output_yt_table="output_yt_table",
)
@vh3.decorator.nirvana_output_names(output_yt_table="output_yt_table")
@vh3.decorator.nirvana_names_transformer(vh3.name_transformers.snake_to_dash, options=True)
def cat_boost_apply(
        *,
        pool: typing.Union[vh3.Binary, vh3.File, vh3.MRTable, vh3.TSV, vh3.Text],
        model_bin: vh3.Binary,
        tree_count_limit: vh3.Integer = None,
        prediction_type: vh3.Enum[typing.Literal["RawFormulaVal", "Class", "Probability"]] = "RawFormulaVal",
        output_columns: vh3.MultipleStrings = (),
        has_header: vh3.Boolean = False,
        delimiter: vh3.String = None,
        ignore_csv_quoting: vh3.Boolean = False,
        args: vh3.String = None,
        yt_token: vh3.Secret = vh3.Factory(lambda: vh3.context.yt_token),
        ttl: vh3.Integer = 360,
        yt_pool: vh3.String = None,
        cpu_guarantee: vh3.Integer = 800,
        cd: typing.Union[vh3.Binary, vh3.File, vh3.MRTable, vh3.TSV, vh3.Text] = None,
        catboost_external_binary: typing.Union[vh3.Binary, vh3.Executable] = None,
        output_yt_table: vh3.MRTable = None
) -> CatBoostApplyOutput:
    """
    CatBoost: Apply

    CatBoost is a machine learning algorithm that uses gradient boosting on decision trees.
    [Doc](https://doc.yandex-team.ru/english/ml/catboost/doc/nirvana-operations/catboost__nirvana__apply-catboost-model.html).
    [Change log](https://a.yandex-team.ru/arc/trunk/arcadia/ml/nirvana/catboost/apply_model/change_log.md).

    :param pool: Dataset. [Documentaion](https://tech.yandex.com/catboost/doc/dg/concepts/input-data_values-file-docpage/).
    :param model_bin: Trained model file.
    :param tree_count_limit: Tree count limit
      The number of trees from the model to use when applying. If specified, the first <value> trees are used.
    :param prediction_type: Prediction type
      Prediction type. Supported prediction types: `Probability`, `Class`, `RawFormulaVal`.
    :param output_columns:
      A comma-separated list of prediction types.
      Supported prediction types: `Probability`, `Class`, `RawFormulaVal`, `Label`, `DocId`, `GroupId`, `Weight` and other column types from [here]
      (https://tech.yandex.com/catboost/doc/dg/concepts/input-data_column-descfile-docpage/#input-data_column-descfile).
    :param has_header:
      Read the column names from the first line if this parameter is set to True.
    :param delimiter:
      The delimiter character used to separate the data in the train/test input file.
    :param args: Additional args
    :param cd: Column description. [Documentation](https://tech.yandex.com/catboost/doc/dg/concepts/input-data_column-descfile-docpage/).
    :param catboost_external_binary: CatBoost binary.
      Build command:
      ```
      ya make trunk/arcadia/catboost/app/ -r --checkout -DHAVE_CUDA=yes -DCUDA_VERSION=9.0
      ```
    """
    raise NotImplementedError("Write your local execution stub here")


class CatBoostSelectFeaturesOutput(typing.NamedTuple):
    summary_json: vh3.JSON
    plots_html: vh3.HTML
    model_bin: vh3.Binary
    snapshot_file: vh3.Binary


@vh3.decorator.external_operation("https://nirvana.yandex-team.ru/operation/53e381ff-7ad3-42cc-a553-d7439c249850")
@vh3.decorator.nirvana_output_names(summary_json="summary.json", plots_html="plots.html", model_bin="model.bin")
@vh3.decorator.nirvana_names_transformer(vh3.name_transformers.snake_to_dash, options=True)
def cat_boost_select_features(
        *,
        features_for_select: vh3.String,
        num_features_to_select: vh3.Integer,
        learn: typing.Union[vh3.Binary, vh3.File, vh3.MRTable, vh3.TSV, vh3.Text],
        features_selection_steps: vh3.String = "1",
        features_selection_algorithm: vh3.Enum[
            typing.Literal[
                "RecursiveByPredictionValuesChange",
                "RecursiveByLossFunctionChange",
                "RecursiveByShapValues"
            ]
        ] = "RecursiveByLossFunctionChange",
        shap_calc_type: vh3.Enum[typing.Literal["Approximate", "Regular", "Exact"]] = "Regular",
        train_final_model: vh3.Boolean = False,
        iterations: vh3.Integer = 500,
        learning_rate: vh3.Number = None,
        loss_function: vh3.Enum[
            typing.Literal[
                "RMSE",
                "Logloss",
                "MAE",
                "CrossEntropy",
                "Quantile",
                "LogLinQuantile",
                "Poisson",
                "MAPE",
                "MultiClass",
                "MultiClassOneVsAll",
                "PairLogit",
                "YetiRank",
                "QueryRMSE",
                "QuerySoftMax",
                "YetiRankPairwise",
                "PairLogitPairwise",
                "QueryCrossEntropy",
                "LambdaMart",
                "StochasticFilter",
                "StochasticRank",
                "MultiRMSE",
                "Huber",
                "Lq",
                "Expectile",
            ]
        ] = "RMSE",
        loss_function_param: vh3.String = None,
        eval_metric: vh3.String = None,
        rsm: vh3.Number = None,
        seed: vh3.Integer = 0,
        ignored_features: vh3.String = None,
        l2_leaf_reg: vh3.Number = None,
        use_best_model: vh3.Boolean = True,
        border_count: vh3.Integer = None,
        depth: vh3.Integer = None,
        args: vh3.String = None,
        bagging_temperature: vh3.Number = None,
        random_strength: vh3.Number = None,
        ctr_leaf_count_limit: vh3.Integer = None,
        feature_border_type: vh3.Enum[
            typing.Literal["Median", "GreedyLogSum", "UniformAndQuantiles", "MinEntropy", "MaxLogSum"]
        ] = None,
        od_type: vh3.Enum[typing.Literal["IncToDec", "Iter"]] = None,
        od_pval: vh3.Number = None,
        bootstrap_type: vh3.Enum[typing.Literal["Poisson", "Bayesian", "Bernoulli", "MVS", "No"]] = None,
        subsample: vh3.Number = None,
        per_float_feature_quantization: vh3.String = None,
        max_ctr_complexity: vh3.Integer = None,
        simple_ctr: vh3.String = None,
        combinations_ctr: vh3.String = None,
        text_processing: vh3.String = None,
        has_header: vh3.Boolean = False,
        delimiter: vh3.String = None,
        ignore_csv_quoting: vh3.Boolean = False,
        target_border: vh3.Number = None,
        one_hot_max_size: vh3.Number = None,
        leaf_estimation_method: vh3.Enum[typing.Literal["Newton", "Gradient", "Exact"]] = None,
        leaf_estimation_iterations: vh3.Integer = None,
        sampling_unit: vh3.Enum[typing.Literal["Object", "Group"]] = None,
        model_metadata: vh3.MultipleStrings = (),
        yt_token: vh3.Secret = vh3.Factory(lambda: vh3.context.yt_token),
        yt_pool: vh3.String = None,
        slaves: vh3.Integer = None,
        ttl: vh3.Integer = 3600,
        cpu_guarantee: vh3.Integer = 1600,
        cb_gpu_type: vh3.Enum[typing.Literal[
            "NONE", "ANY", "CUDA_3_5", "CUDA_5_2", "CUDA_6_1", "CUDA_7_0", "CUDA_8_0"]
        ] = "NONE",
        restrict_gpu_type: vh3.Boolean = False,
        inner_options_override: vh3.String = None,
        debug_timeout: vh3.Integer = None,
        boost_from_average: vh3.Boolean = False,
        test: typing.Union[vh3.Binary, vh3.File, vh3.MRTable, vh3.TSV, vh3.Text] = None,
        cd: typing.Union[vh3.Binary, vh3.File, vh3.MRTable, vh3.TSV, vh3.Text] = None,
        pairs: typing.Union[vh3.Binary, vh3.File, vh3.MRTable, vh3.TSV, vh3.Text] = None,
        test_pairs: typing.Union[vh3.Binary, vh3.File, vh3.MRTable, vh3.TSV, vh3.Text] = None,
        learn_group_weights: typing.Union[vh3.Binary, vh3.File, vh3.MRTable, vh3.TSV, vh3.Text] = None,
        test_group_weights: typing.Union[vh3.Binary, vh3.File, vh3.MRTable, vh3.TSV, vh3.Text] = None,
        catboost_external_binary: typing.Union[vh3.Binary, vh3.Executable] = None,
        params_file: typing.Union[vh3.JSON, vh3.Text] = None,
        snapshot_file: vh3.Binary = None,
        borders: typing.Union[vh3.File, vh3.TSV, vh3.Text] = None,
        baseline_model: vh3.Binary = None,
        learn_baseline: typing.Union[vh3.Binary, vh3.File, vh3.MRTable, vh3.TSV, vh3.Text] = None,
        test_baseline: typing.Union[vh3.Binary, vh3.File, vh3.MRTable, vh3.TSV, vh3.Text] = None,
        pool_metainfo: vh3.JSON = None
) -> CatBoostSelectFeaturesOutput:
    """
    CatBoost: Select Features

    Features selection for CatBoost.
    [Changelog](https://a.yandex-team.ru/arc/trunk/arcadia/ml/nirvana/catboost/select_features/change_log.md)

    :param features_for_select: Features for select
      Specify scored features (indices/names) delimited by comma (for example: 1,3,5,10). Sequential features can be specified as range (for example: 1-4,5-8,12,16).
    :param num_features_to_select: Number of features to select
      How many features to select from `features-for-select`.
    :param features_selection_steps: Number of selection steps
      How many steps of elimination to perform. More steps - more accurate results.
    :param features_selection_algorithm: Selection algorithm
      Algorithms are ordered based on accuracy and complexity.
    :param shap_calc_type: Shap calculation type
      Shap types are ordered based on accuracy/complexity.
    :param train_final_model: Train final model
      Should the final model with selected features train?
    :param learning_rate: Learning rate
    :param loss_function: Learning method
    :param loss_function_param:
      loss param in format paramName=value
    :param rsm:
      only CPU version
    :param ignored_features: Ignore features
    :param l2_leaf_reg: l2 leaf reg
    :param use_best_model: Use best model
    :param border_count: float feature border count
    :param depth: Max tree depth
      suggested < 10 on GPU version
    :param args: additional arguments
    :param bagging_temperature: bagging temperature
    :param random_strength: random strength
    :param ctr_leaf_count_limit: ctr leaf count limit
      only CPU version
    :param feature_border_type: feature border type
    :param od_type: Overfitting detector type
    :param od_pval: auto stop pval
    :param bootstrap_type: Bootstrap type
    :param subsample: Sample rate for bagging
    :param max_ctr_complexity: max ctr complexity
    :param simple_ctr:
      should be written in format CtrType[:TargetBorderCount=BorderCount][:TargetBorderType=BorderType][:CtrBorderCount=Count][:CtrBorderType=Type][:Prior=num/denum]
    :param combinations_ctr:
      should be written in format CtrType[:TargetBorderCount=BorderCount][:TargetBorderType=BorderType][:CtrBorderCount=Count][:CtrBorderType=Type][:Prior=num/denum]
    :param text_processing: text-processing
    :param has_header: has header
    :param delimiter: Learning and training sets delimiter
    :param model_metadata: Model metadata key-value pairs
      Key-value pairs, quote key and value for reasonable results
    :param slaves: Number of Slaves:
      Number of slaves. Set 0 to run in singlehost mode
    :param cb_gpu_type:
      Type of GPU required for the job to run. Defaults to `No GPU` (`NONE`).
    :param inner_options_override: Override computed settings
      JSON string, e.g. {"master-max-ram":300000}
    :param boost_from_average:
      For RMSE: start training with approxes set to mean target
    :param learn_baseline: Baseline for learn
    :param test_baseline: Baseline for test (the first one, if many)
    :param pool_metainfo: Metainfo file with feature tags.
    """
    raise NotImplementedError("Write your local execution stub here")


class MeasureMmClassifierQualityOutput(typing.NamedTuple):
    result: vh3.JSON
    requests_winners: vh3.TSV
    scenario_list: vh3.JSON


@vh3.decorator.external_operation("https://nirvana.yandex-team.ru/operation/1f510285-9cbe-475c-8778-4f99d8ea89a3")
@vh3.decorator.nirvana_output_names(requests_winners="requests_winners", scenario_list="scenario_list")
@vh3.decorator.nirvana_names_transformer(vh3.name_transformers.snake_to_dash, options=True)
def measure_mm_classifier_quality(
        *,
        executable: vh3.Executable,
        other: vh3.TSV,
        ttl: vh3.Integer = 360,
        max_ram: vh3.Integer = 256,
        catboost: vh3.Boolean = False,
        music: vh3.TSV = None,
        video: vh3.TSV = None,
        search: vh3.TSV = None,
        gc: vh3.TSV = None
) -> MeasureMmClassifierQualityOutput:
    """
    Measure MM classifier quality

    """
    raise NotImplementedError("Write your local execution stub here")


class MmPreclassifierEmulatorCatboostOutput(typing.NamedTuple):
    music_output: vh3.TSV
    video_output: vh3.TSV
    vins_output: vh3.TSV
    search_output: vh3.TSV
    gc_output: vh3.TSV


@vh3.decorator.external_operation("https://nirvana.yandex-team.ru/operation/bd3eab0e-2494-483f-b438-425a7cd6b35a")
@vh3.decorator.nirvana_names(
    sandbox_oauth_token="sandbox_oauth_token", arcadia_revision="arcadia_revision", yt_pool="yt_pool"
)
@vh3.decorator.nirvana_names_transformer(vh3.name_transformers.snake_to_dash, options=True)
def mm_preclassifier_emulator_catboost(
        *,
        yt_token: vh3.Secret = vh3.Factory(lambda: vh3.context.yt_token),
        mr_account: vh3.String = vh3.Factory(lambda: vh3.context.mr_account),
        yql_token: vh3.Secret = vh3.Factory(lambda: vh3.context.yql_token),
        data: vh3.MRTable,
        slices: vh3.TSV,
        thresholds: vh3.JSON,
        timestamp: vh3.String = None,
        sandbox_oauth_token: vh3.Secret = vh3.Factory(lambda: vh3.context.sandbox_token),
        arcadia_revision: vh3.Integer = 6256503,
        max_ram: vh3.Integer = 100,
        yt_pool: vh3.String = None,
        max_disk: vh3.Integer = 1024,
        music_pre_formula: vh3.Binary = None,
        video_pre_formula: vh3.Binary = None,
        vins_pre_formula: vh3.Binary = None,
        search_pre_formula: vh3.Binary = None,
        gc_pre_formula: vh3.Binary = None
) -> MmPreclassifierEmulatorCatboostOutput:
    """
    MM Preclassifier emulator catboost

    :param yt_token: YT Token:
      ID of Nirvana Secret with YT access token (https://nda.ya.ru/3RSzVU).
      Guide to Nirvana Secrets: https://nda.ya.ru/3RSzWZ
    :param mr_account: MR Account:
      MR Account Name.
      By default, output tables and directories will be created in some subdirectory of home/<MR Account>/<workflow owner>/nirvana
    :param yql_token: YQL Token:
      YQL OAuth Token, see https://wiki.yandex-team.ru/kikimr/yql/userguide/cli/#autentifikacija
    :param data: Data for formula evaluation
    :param timestamp: Timestamp:
      Set a recent, not previously used timestamp to force "MR Read" to run even if `table' was already read once
    :param sandbox_oauth_token: Sandbox OAuth token
      To run task on behalf of specific user
    :param arcadia_revision: Arcadia Revision
    """
    raise NotImplementedError("Write your local execution stub here")


@vh3.decorator.external_operation("https://nirvana.yandex-team.ru/operation/b82faf5c-948b-4208-b70f-2520eb0e3337")
@vh3.decorator.nirvana_names(
    request_data_folder="request_data_folder", request_data_table_name="request_data_table_name", yql_token="yql_token"
)
@vh3.decorator.nirvana_output_names("classification_results")
@vh3.decorator.nirvana_names_transformer(vh3.name_transformers.snake_to_dash, options=True)
def aggregate_classification_results(
        *,
        yt_token: vh3.Secret = vh3.Factory(lambda: vh3.context.yt_token),
        mr_account: vh3.String = vh3.Factory(lambda: vh3.context.mr_account),
        yql_token: vh3.Secret = vh3.Factory(lambda: vh3.context.yql_token),
        executable: vh3.Executable,
        scenario_list: vh3.JSON,
        request_winner_scenarios: vh3.MRTable,
        request_ids: vh3.MRTable,
        mr_output_path: vh3.String = "",
        yt_pool: vh3.String = None,
        ttl: vh3.Integer = 360,
        max_ram: vh3.Integer = 100,
        mr_transaction_policy: vh3.Enum[typing.Literal["MANUAL", "AUTO"]] = "MANUAL",
        cluster: vh3.String = "hahn",
        request_data_folder: vh3.String = "//home/alice-dev/tolyandex/test-learn/eval",
        request_data_table_name: vh3.String = "basket",
        mr_output_ttl: vh3.Integer = vh3.Factory(lambda: vh3.context.mr_output_ttl)
) -> vh3.MRTable:
    """
    Aggregate classification results

    This operation generates MR table with requests and winner scenario and answer for each request.

    Input: executable file; JSON with list of scenarios used in classification; MR table with winner scenario for each request; MR table with request ids (in the same order as winner scenarios).

    Required parameters: path to the folder with MR tables with request info for each scenario; name of the MR table with info for each scenario.

    Output: MR table with aggregated info; link on YQL with merge query.

    :param yt_token: YT Token:
      ID of Nirvana Secret with YT access token (https://nda.ya.ru/3RSzVU).
      Guide to Nirvana Secrets: https://nda.ya.ru/3RSzWZ
    :param mr_account: MR Account:
      MR Account Name.
      By default, output tables and directories will be created in some subdirectory of home/<MR Account>/<workflow owner>/nirvana
    :param yql_token: YQL token
    :param yt_pool: YT Pool:
      Pool used by YT scheduler. Leave blank to use default pool.
      This option has no effect on YaMR.
    :param mr_transaction_policy: MR transaction policy:
    :param cluster: YT cluster
    :param request_data_folder: Path to request data folder
    :param request_data_table_name: Request data table name
    """
    raise NotImplementedError("Write your local execution stub here")


@vh3.decorator.external_operation("https://nirvana.yandex-team.ru/operation/54acdb65-5810-408e-ba21-9b712f521fdc")
@vh3.decorator.nirvana_names(
    formula_resource_options="formula-resource-options",
    experiment_name="experiment-name",
    client_type="client-type",
    scenario_names="scenario-names",
)
@vh3.decorator.nirvana_output_names("resource_id")
def build_formula_resource(
    *,
    client_type: ClientTypeEnum,
    formula_resource_options: FormulaResourceOptionsEnum = "skip",
    experiment_name: vh3.String = None,
    scenario_names: vh3.String = '{"music": "HollywoodMusic", "video": "Video", "vins": "Vins", "search": "Search", "gc": "GeneralConversation"}',
    patch_type: vh3.Enum[typing.Literal["skip", "commit", "review"]] = "skip",
    sandbox_oauth_token: vh3.Secret = vh3.Factory(lambda: vh3.context.sandbox_token),
    formulas_revision: vh3.Integer = None,
    source_formula_extension: vh3.String = "info",
    target_formula_extension: vh3.String = "info",
    pre_formulas: typing.Sequence[vh3.JSON] = (),
    post_formulas: typing.Sequence[vh3.JSON] = (),
    thresholds: vh3.JSON = None,
    commit_configs: vh3.Boolean = False,
    commit_formulas: vh3.Boolean = False,
    commit_message: vh3.String = '',
    language: vh3.Enum[typing.Literal["L_RUS", "L_ARA"]] = "L_RUS",
) -> vh3.OptionalOutput[vh3.JSON]:
    """
    Build formula resource

    Creates formula resource via sandbox task RELEASE_MEGAMIND_FORMULAS.

    :param client_type: Client type
    :param formula_resource_options: Formula resource options
    :param experiment_name: Experiment name
    :param scenario_names: Short scenario names to full
    :param pre_formulas:
      Gets formula id as "id" field of JSON. The link name should be the short name of the scenario, e.g. "music".
    :param post_formulas:
      Gets formula id as "id" field of JSON. The link name should be the short name of the scenario, e.g. "music".
    :param thresholds:
      Gets thresholds for each pre_formula.
    """
    raise NotImplementedError("Write your local execution stub here")


class CatBoostEvalFeatureOutput(typing.NamedTuple):
    summary: vh3.TSV
    colored_pvalues_and_deltas: vh3.HTML
    scores_archive: vh3.Binary
    html_plot: vh3.HTML
    png_plot: vh3.Image
    fstr_deltas: vh3.HTML
    json_fstr_deltas: vh3.JSON
    json_total_gpu_stats: vh3.JSON


@vh3.decorator.external_operation("https://nirvana.yandex-team.ru/operation/9ab24f82-9558-4989-bc84-5f6a8ec71837")
@vh3.decorator.nirvana_names(
    time_split_quantile="time_split_quantile",
    bootstrap_type="bootstrap_type",
    od_type="od_type",
    model_metadata="model_metadata",
    yt_pool="yt_pool",
    query_timestamps="queryTimestamps",
)
@vh3.decorator.nirvana_names_transformer(vh3.name_transformers.snake_to_dash, options=True)
def cat_boost_eval_feature(
    *,
    features: typing.Union[vh3.Binary, vh3.File, vh3.MRTable, vh3.TSV, vh3.Text],
    ttl: vh3.Integer = 10080,
    gpu_type: vh3.Enum[typing.Literal["ANY", "CUDA_3_5", "CUDA_5_2", "CUDA_6_1", "CUDA_7_0", "CUDA_8_0"]] = "ANY",
    cpu_guarantee: vh3.Integer = 1600,
    iterations: vh3.Integer = 10000,
    learning_rate: vh3.Number = None,
    seed: vh3.Integer = 0,
    ignored_features: vh3.String = None,
    features_to_evaluate: vh3.String = None,
    evaluation_mode: vh3.Enum[typing.Literal["OneVsNone", "OneVsOthers", "OneVsAll", "OthersVsAll"]] = "OneVsNone",
    fold_count: vh3.Integer = None,
    offset: vh3.Integer = 0,
    fold_size_unit: vh3.Enum[typing.Literal["queries", "documents"]] = "queries",
    fold_size: vh3.Integer = None,
    relative_fold_size: vh3.Number = None,
    time_split_quantile: vh3.Number = None,
    loss_function: vh3.Enum[
        typing.Literal[
            "RMSE",
            "Logloss",
            "MAE",
            "CrossEntropy",
            "Quantile",
            "LogLinQuantile",
            "Poisson",
            "MAPE",
            "MultiClass",
            "MultiClassOneVsAll",
            "PairLogit",
            "YetiRank",
            "QueryRMSE",
            "QuerySoftMax",
            "YetiRankPairwise",
            "PairLogitPairwise",
            "QueryCrossEntropy",
        ]
    ] = "CrossEntropy",
    loss_function_param: vh3.String = None,
    target_border: vh3.Number = None,
    boost_from_average: vh3.Boolean = False,
    custom_metric: vh3.String = None,
    l2_leaf_reg: vh3.Number = None,
    leaf_estimation_method: vh3.Enum[typing.Literal["Newton", "Gradient", "Exact"]] = None,
    leaf_estimation_iterations: vh3.Integer = None,
    feature_border_type: vh3.Enum[
        typing.Literal["Median", "GreedyLogSum", "UniformAndQuantiles", "MinEntropy", "MaxLogSum"]
    ] = None,
    border_count: vh3.Integer = None,
    fstr_type: vh3.Enum[typing.Literal["PredictionValuesChange", "LossFunctionChange"]] = "PredictionValuesChange",
    depth: vh3.Integer = None,
    prediction_type: vh3.Enum[typing.Literal["RawFormulaVal", "Probability", "Class"]] = None,
    args: vh3.String = None,
    bootstrap_type: vh3.Enum[typing.Literal["Poisson", "Bayesian", "Bernoulli", "No"]] = None,
    bagging_temperature: vh3.Number = None,
    subsample: vh3.Number = None,
    rsm: vh3.Number = None,
    random_strength: vh3.Number = None,
    eval_metric: vh3.String = None,
    od_type: vh3.Enum[typing.Literal["IncToDec", "Iter"]] = None,
    od_pval: vh3.Number = 0.001,
    max_ctr_complexity: vh3.Integer = None,
    simple_ctr: vh3.String = None,
    combinations_ctr: vh3.String = None,
    one_hot_max_size: vh3.Number = None,
    ctr_leaf_count_limit: vh3.Integer = None,
    has_header: vh3.Boolean = False,
    delimiter: vh3.String = None,
    ignore_csv_quoting: vh3.Boolean = False,
    model_metadata: vh3.MultipleStrings = (),
    inner_options_override: vh3.String = None,
    yt_token: vh3.Secret = "fml_public_yt_token",
    yt_pool: vh3.String = None,
    max_ram: vh3.Integer = 5000,
    per_float_feature_quantization: vh3.String = None,
    catboost_binary: vh3.Executable = None,
    models_archive: vh3.Binary = None,
    grid: typing.Union[vh3.File, vh3.TSV, vh3.Text] = None,
    pairs: typing.Union[vh3.Binary, vh3.File, vh3.MRTable, vh3.TSV, vh3.Text] = None,
    query_timestamps: typing.Union[vh3.Binary, vh3.File, vh3.MRTable, vh3.TSV, vh3.Text] = None,
    learn_group_weights: typing.Union[vh3.Binary, vh3.File, vh3.MRTable, vh3.TSV, vh3.Text] = None,
    cd: typing.Union[vh3.Binary, vh3.File, vh3.MRTable, vh3.TSV, vh3.Text] = None,
    params_file: typing.Union[vh3.JSON, vh3.Text] = None,
    pool_metainfo: vh3.JSON = None
) -> CatBoostEvalFeatureOutput:
    """
    CatBoost: Eval Feature

    Eval feature using CatBoost

    [Change log](https://a.yandex-team.ru/arc/trunk/arcadia/ml/nirvana/catboost/eval_feature/changelog.md)

    [Documentation](https://doc.yandex-team.ru/english/ml/feature-selection/concepts/eval-feature-intro.html)

    :param features:
      Training dataset. [Documentaion](https://tech.yandex.com/catboost/doc/dg/concepts/input-data_values-file-docpage/).
    :param learning_rate: Learning rate
    :param ignored_features: Ignore features
      [[only CPU version]]
    :param features_to_evaluate:
      [[List of sets of tested features
    Sets are delimited by semicolon
    Features are delimited by comma]]
      List of sets of tested features

      Sets are delimited by semicolon

      Features are delimited by comma
    :param evaluation_mode:
      [[Specify which features to use in baseline and test runs for each F = set of features from features-to-evaluate]]
      Specify which features to use in baseline and test runs for each F = set of features from features-to-evaluate
    :param fold_count:
      [[Folds to run for fixed-fold-size splitting]]
      Folds to run for fixed-fold-size splitting
    :param offset:
      [[First fold for fixed-fold-size splitting]]
      First fold for fixed-fold-size splitting
    :param fold_size_unit:
      [[Fold size unit type for fixed-fold-size splitting]]
      Fold size unit type for fixed-fold-size splitting
    :param fold_size:
      [[Absolute fold size in fold-size-unit's; mutually exclusive with relative-fold-size]]
      Absolute fold size in fold-size-unit's; mutually exclusive with relative-fold-size
    :param relative_fold_size:
      [[Fold size as fraction of dataset; mutually exclusive with fold-size]]
      Fold size as fraction of dataset; mutually exclusive with fold-size
    :param time_split_quantile: Time split quantile for time-split eval mode
    :param loss_function: Loss function
      [[for GPU version available only RMSE, Logloss, CrossEntropy
    YetiRank is available only on GPU]]
    :param custom_metric:
      [[List of metrics to compute average change for, when moving from the baseline formula to the tested formula
    May be empty]]
      List of metrics to compute average change for, when moving from the baseline formula to the tested formula
      May be empty
    :param l2_leaf_reg: l2 leaf reg
    :param feature_border_type: feature border type
      "feature-border-type", "Should be one of: Median, GreedyLogSum, UniformAndQuantiles, MinEntropy, MaxLogSum"
    :param border_count: float feature border count
    :param depth: Max tree depth
      [[suggested < 10 on GPU version]]
    :param prediction_type: Prediction type
      [[only CPU version]]
    :param args: Аdditional arguments
    :param bootstrap_type: Bootstrap type
    :param bagging_temperature: bagging temperature
      [[only CPU version]]
    :param subsample: Sample rate for bagging
    :param random_strength: random strength
      [[only CPU version]]
    :param od_type: Overfitting detector type
    :param od_pval: auto stop pval
      [[only CPU version]]
    :param max_ctr_complexity: max ctr complexity
      [[only CPU version]]
    :param one_hot_max_size:
      If parameter is specified than features with no more than specified value different values will be converted to float features using one-hot encoding.
    :param ctr_leaf_count_limit: ctr leaf count limit
      [[only CPU version]]
    :param has_header: has header
      [[only CPU version]]
    :param delimiter: Learning and training sets delimiter
      [[only CPU version]]
    :param model_metadata: Model metadata key-value pairs
      [[Key-value pairs, quote key and value for reasonable results]]
    :param inner_options_override: Override options for inner train cube
      [[JSON string, e.g. {"master-max-ram":250000}]]
      Normally this option shall be null. Used to debug/override resource requirements
    :param catboost_binary:
      Optional CatBoost binary. Build command:
      ```
      ya make trunk/arcadia/catboost/app/ -r --checkout -DHAVE_CUDA=yes -DCUDA_VERSION=9.0
      ```
    :param models_archive:
      Snapshot file. Used for recovering training after an interruption.
    :param grid:
      File with borders description.

      Format:
      ```
      <feature_index(zero-based)>\t<border_value>\n
      ```

      Example:
      ```
      0\t0.5
      0\t1.0
      1\t0.7
      ```

      Note: File should be sorted by first column.
    :param pairs:
      Training pairs. [Documentation](https://tech.yandex.com/catboost/doc/dg/concepts/input-data_pairs-description-docpage/).
    :param query_timestamps:
      Query timestamps for time-split eval mode
    :param learn_group_weights:
      Training query weights.

      Format:
      ```
      <group_id>\t<weight>\n
      ```

      Example:
      ```
      id1\t0.5
      id2\t0.7
      ```
    :param cd:
      Column description. [Documentation](https://tech.yandex.com/catboost/doc/dg/concepts/input-data_column-descfile-docpage/).
    :param params_file:
      JSON file that contains the training parameters, for example:
      ```
      {
          "loss_function": "Logloss",
          "iterations": 400
      }
      ```
    :param pool_metainfo:
      Metainfo file with feature tags.
    """
    raise NotImplementedError("Write your local execution stub here")
