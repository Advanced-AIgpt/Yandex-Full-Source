from typing import NamedTuple, Union, Sequence, Literal

import vh3


class ArcExportFileOutput(NamedTuple):
    binary: vh3.OptionalOutput[vh3.Binary]
    json: vh3.OptionalOutput[vh3.JSON]
    text: vh3.OptionalOutput[vh3.Text]


@vh3.decorator.external_operation("https://nirvana.yandex-team.ru/operation/6a6437c9-2125-4258-bf91-fe9749ac9221")
@vh3.decorator.nirvana_names_transformer(vh3.name_transformers.snake_to_dash, options=True)
def arc_export_file(
    *,
    arc_token: vh3.Secret = vh3.Factory(lambda: vh3.context.arc_token),
    path: vh3.String,
    ttl: vh3.Integer = 360,
    max_ram: vh3.Integer = 1024,
    max_disk: vh3.Integer = 2048,
    retries_on_job_failure: vh3.Integer = 3,
    debug_timeout: vh3.Integer = 0,
    reference: vh3.String = vh3.Factory(lambda: vh3.context.arc_reference),
) -> ArcExportFileOutput:
    """
    Arc export file

    Checkouts an arc branch. Puts one file to output

    :param reference: Reference
      [[Arc commit or branch name or revision]]
    """
    raise NotImplementedError("Write your local execution stub here")


@vh3.decorator.external_operation("https://nirvana.yandex-team.ru/operation/b81e64be-6f42-4495-9254-183a2df9804c")
@vh3.decorator.nirvana_names(file="File")
@vh3.decorator.nirvana_output_names("tsv")
def convert_any_to_tsv(
    file: Union[
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
) -> vh3.TSV:
    """
    Convert any to TSV
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
    max_ram: vh3.Integer = 4096,
    mr_default_cluster: vh3.String = vh3.Factory(lambda: vh3.context.mr_default_cluster_string),
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
      [[ID of Nirvana Secret with YT access token (https://nda.ya.ru/3RSzVU).
    Guide to Nirvana Secrets: https://nda.ya.ru/3RSzWZ]]
      YT OAuth Token.

        [Obtain access token](https://nda.ya.ru/3RSzVU), then [create a Nirvana secret](https://nda.ya.ru/3RSzWZ) and [use it here](https://nda.ya.ru/3RSzWb).
        You can [share the secret](https://nda.ya.ru/3RSzWd) with user(s) and/or a staff group.
    :param mr_account: MR Account:
      [[MR Account Name.
    By default, output tables and directories will be created in some subdirectory of home/<MR Account>/<workflow owner>/nirvana]]
      MR account name (e.g. `rank_machine`) used to build MR output path for this operation.

      See the `mr-output-path` option for more information
    :param format: Format
    :param yt_pool: YT Pool:
      [[Pool used by YT scheduler. Leave blank to use default pool.
    This option has no effect on YaMR.]]
      Pool used by [YT operation scheduler](https://nda.ya.ru/3Rk4af). Leave this blank to use default pool.
    :param mr_default_cluster: MR Cluster
    """
    raise NotImplementedError("Write your local execution stub here")


class Yql2Output(NamedTuple):
    output1: vh3.OptionalOutput[vh3.MRTable]
    output2: vh3.OptionalOutput[vh3.MRTable]
    directory: vh3.OptionalOutput[vh3.MRDirectory]


@vh3.decorator.external_operation("https://nirvana.yandex-team.ru/operation/8d93630f-4010-480d-9343-5241d0631f4f")
@vh3.decorator.nirvana_names(
    py_code="py_code",
    py_export="py_export",
    py_version="py_version",
    use_account_tmp="use_account_tmp",
    code_revision="code_revision",
    code_work_dir="code_work_dir",
    arcanum_token="arcanum_token",
    svn_user_name="svn_user_name",
    svn_user_id_rsa="svn_user_id_rsa",
    svn_operation_source="svn_operation_source",
)
@vh3.decorator.nirvana_names_transformer(vh3.name_transformers.snake_to_dash, options=True)
def yql_2(
    *,
    mr_account: vh3.String = vh3.Factory(lambda: vh3.context.mr_account),
    yt_token: vh3.Secret = vh3.Factory(lambda: vh3.context.yt_token),
    yql_token: vh3.Secret = vh3.Factory(lambda: vh3.context.yql_token),
    request: vh3.String,
    py_code: vh3.String = None,
    py_export: vh3.MultipleStrings = (),
    py_version: vh3.Enum[Literal["Python2", "ArcPython2", "Python3"]] = "Python3",
    mr_default_cluster: vh3.Enum[
        Literal[
            "hahn",
            "freud",
            "marx",
            "hume",
            "arnold",
            "markov",
            "bohr",
            "landau",
            "seneca-vla",
            "seneca-sas",
            "seneca-man",
        ]
    ] = vh3.Factory(lambda: vh3.context.mr_default_cluster),
    yt_pool: vh3.String = None,
    ttl: vh3.Integer = 7200,
    max_ram: vh3.Integer = 256,
    max_disk: vh3.Integer = 1024,
    timestamp: vh3.String = vh3.Factory(lambda: vh3.context.timestamp_string),
    param: vh3.MultipleStrings = (),
    mr_output_path: vh3.String = None,
    yt_owners: vh3.String = None,
    use_account_tmp: vh3.Boolean = False,
    code_revision: vh3.String = None,
    code_work_dir: vh3.String = None,
    arcanum_token: vh3.Secret = vh3.Factory(lambda: vh3.context.arc_token),
    svn_user_name: vh3.String = None,
    svn_user_id_rsa: vh3.Secret = None,
    svn_operation_source: vh3.MultipleStrings = (),
    yql_operation_title: vh3.String = "YQL Nirvana Operation: {{nirvana_operation_url}}",
    mr_output_ttl: vh3.Integer = None,
    retries_on_job_failure: vh3.Integer = 0,
    retries_on_system_failure: vh3.Integer = 10,
    job_metric_tag: vh3.String = None,
    mr_transaction_policy: vh3.Enum[Literal["MANUAL", "AUTO"]] = "AUTO",
    input1: Sequence[vh3.MRTable] = (),
    input2: Sequence[vh3.MRTable] = (),
    files: Sequence[
        Union[
            vh3.Binary,
            vh3.Executable,
            vh3.HTML,
            vh3.Image,
            vh3.JSON,
            vh3.MRDirectory,
            vh3.MRTable,
            vh3.TSV,
            vh3.Text,
            vh3.XML,
        ]
    ] = ()
) -> Yql2Output:
    """
    YQL 2

    Apply YQL script on MapReduce

    Code: https://a.yandex-team.ru/arc/trunk/arcadia/dj/nirvana/operations/yql/yql

    User guide: https://wiki.yandex-team.ru/nirvana-ml/ml-marines/#yql

    :param mr_account: MR Account:
      [[MR Account Name.
    By default, output tables and directories will be created in some subdirectory of home/<MR Account>/<workflow owner>/nirvana]]
      MR account name (e.g. `rank_machine`) used to build MR output path for this operation.

      See the `mr-output-path` option for more information
    :param yt_token: YT Token:
      [[ID of Nirvana Secret with YT access token (https://nda.ya.ru/3RSzVU).
    Guide to Nirvana Secrets: https://nda.ya.ru/3RSzWZ]]
      YT OAuth Token.

        [Obtain access token](https://nda.ya.ru/3RSzVU), then [create a Nirvana secret](https://nda.ya.ru/3RSzWZ) and [use it here](https://nda.ya.ru/3RSzWb).
        You can [share the secret](https://nda.ya.ru/3RSzWd) with user(s) and/or a staff group.
    :param yql_token: YQL Token:
      [[YQL OAuth Token, see https://wiki.yandex-team.ru/kikimr/yql/userguide/cli/#autentifikacija]]
      YQL OAuth Token, see https://wiki.yandex-team.ru/kikimr/yql/userguide/cli/#autentifikacija
    :param request: Request
      [[YQL request]]
      YQL request
    :param py_code: Python Code
      [[Python user defined functions definition]]
      Python user defined functions definition
    :param py_export: Python Export
      [[Python user defined functions declaration]]
      Python user defined functions declaration
    :param py_version: Python Version
      [[Python user defined functions version, https://clubs.at.yandex-team.ru/yql/2400]]
      Python user defined functions version, https://clubs.at.yandex-team.ru/yql/2400
    :param mr_default_cluster: Default YT cluster:
      [[Default YT cluster]]
      Default YT cluster
    :param yt_pool: YT Pool:
      [[Pool used by YT scheduler. Leave blank to use default pool.
    This option has no effect on YaMR.]]
      Pool used by [YT operation scheduler](https://nda.ya.ru/3Rk4af). Leave this blank to use default pool.
    :param timestamp: Timestamp for caching
      [[Any string used for Nirvana caching only]]
      Any string used for Nirvana caching only
    :param param: Parameters
      [[List of 'name=value' items which could be accessed as {{param[name]}}]]
      List of 'name=value' items which could be accessed as {{param[name]}}
    :param mr_output_path: MR Output Path:
      [[Directory for output MR tables and directories.
    Limited templating is supported: `${param["..."]}`, `${meta["..."]}`, `${mr_input["..."]}` (path to input MR *directory*) and `${uniq}` (= unique path-friendly string).]]
      Directory for output MR tables and directories.

      Limited templating is supported: `${param["..."]}`, `${meta["..."]}`, `${mr_input["..."]}` (path to input MR *directory*) and `${uniq}` (= unique path-friendly string).

      The default template for `mr-output-path` is

              home[#if param["mr-account"] != meta.owner]/${param["mr-account"]}[/#if]/${meta.owner}/nirvana/${meta.operation_uid}

      If output path does not exist, it will be created.

      Temporary directory, `${mr_tmp}`, is derived from output path in an unspecified way. It is ensured that:
        * It will exist before `job-command` is started
        * It need not be removed manually after execution ends. However, you **should** remove all temporary data created in `${mr_tmp}`, even if your command fails
    :param yt_owners: YT Owners
      [[Additional YT users allowed to read and manage operations]]
      Additional YT users allowed to read and manage operations
    :param use_account_tmp: Use tmp in account
      [[Use tmp folder in account but not in //tmp for avoid fails due to tmp overquota, recommended for production processes]]
      Use tmp folder in account but not in //tmp for avoid fails due to tmp overquota, recommended for production processes
    :param code_revision: Code default revision
      [[Default code revision for {{arcadia:/...}}]]
      Default code revision for {{arcadia:/...}}
    :param code_work_dir: Code default directory
      [[Default code working directory for {{./...}}]]
      Default code working directory for {{./...}}
    :param arcanum_token: Arcanum Token
      [[Arcanum Token, see https://wiki.yandex-team.ru/arcanum/api/]]
      Arcanum Token, see https://wiki.yandex-team.ru/arcanum/api/
    :param svn_user_name: SVN User name
      [[SVN user name for operation source and {{arcadia:/...}}]]
      SVN user name for operation source and {{arcadia:/...}}
    :param svn_user_id_rsa: SVN User private key
      [[SVN user private key for operation source and {{arcadia:/...}}]]
      SVN user private key for operation source and {{arcadia:/...}}
    :param svn_operation_source: SVN Operation source
      [[The YQL operation source path on SVN, should start with arcadia:/ or svn+ssh://, may contain @revision]]
      The YQL operation source path on SVN, should start with arcadia:/ or svn+ssh://, may contain @revision
    :param yql_operation_title: YQL Operation title
      [[YQL operation title for monitoring]]
      YQL operation title for monitoring
    :param mr_output_ttl: MR Output TTL, days:
      [[TTL in days for mr-output-path directory and outputs which are inside the directory]]
      TTL in days for mr-output-path directory and outputs which are inside the directory
    :param job_metric_tag: Job metric tag
      [[Tag for monitoring of resource usage]]
      Tag for monitoring of resource usage
    :param mr_transaction_policy: MR Transaction policy
      [[Transaction policy, in auto policy yql operations are canceled when nirvana workflow in canceled]]
      Transaction policy, in auto policy yql operations are canceled when nirvana workflow in canceled
    :param input1:
      Input 1
    :param input2:
      Input 2
    :param files:
      Attached files: if link_name is specified it is interpreted as file name, otherwise the input is unpacked as tar archive
    """
    raise NotImplementedError("Write your local execution stub here")


@vh3.decorator.external_operation("https://nirvana.yandex-team.ru/operation/86670fb6-6c9a-4cd3-8802-6fcd745b818e")
@vh3.decorator.nirvana_output_names("code")
@vh3.decorator.nirvana_names_transformer(vh3.name_transformers.snake_to_dash, options=True)
def arc_export_not_deterministic(
    *,
    arc_token: vh3.Secret = vh3.Factory(lambda: vh3.context.arc_token),
    path: vh3.String,
    ttl: vh3.Integer = 360,
    max_ram: vh3.Integer = 1024 * 2,
    max_disk: vh3.Integer = 1024 * 2,
    retries_on_job_failure: vh3.Integer = 3,
    debug_timeout: vh3.Integer = 0,
    reference: vh3.String = vh3.Factory(lambda: vh3.context.arc_reference)
) -> vh3.Binary:
    """
    Arc export (not deterministic)

    Checkouts an arc branch to an archive

    :param reference: Reference
      [[Arc commit or branch name or revision]]
    """
    raise NotImplementedError("Write your local execution stub here")


class Python3DeepLearningOutput(NamedTuple):
    data: vh3.Binary
    state: vh3.Binary
    logs: vh3.Binary
    json_output: vh3.JSON


@vh3.decorator.external_operation("https://nirvana.yandex-team.ru/operation/8ab677a2-7ba9-4f7c-9492-bbf8fcba7b2e")
@vh3.decorator.nirvana_names(
    base_layer="base_layer",
    environment="Environment",
    install_pydl_package="install_pydl_package",
    run_command="run_command",
    openmpi_runner="openmpi_runner",
    openmpi_params="openmpi_params",
    mpi_processes_count="mpi_processes_count",
    before_run_command="before_run_command",
    after_run_command="after_run_command",
    ssh_key="ssh_key",
    auto_snapshot="auto_snapshot",
    nodes_count="nodes_count",
    additional_layers="additional_layers",
    user_requested_secret="user_requested_secret",
)
@vh3.decorator.nirvana_names_transformer(vh3.name_transformers.snake_to_dash, options=True)
def python_3_deep_learning(
    *,
    base_layer: vh3.Enum[
        Literal["PYDL_V3", "PYDL_V4", "PYDL_V5", "PYDL_V5_GPU", "PYDL_V5_GPU_NVIDIA", "NONE"]
    ] = "PYDL_V5_GPU_NVIDIA",
    environment: vh3.MultipleStrings = (),
    pip: vh3.MultipleStrings = (),
    install_pydl_package: vh3.Boolean = True,
    run_command: vh3.String = "python3 $SOURCE_CODE_PATH/__main__.py",
    cpu_cores_usage: vh3.Integer = 1600,
    gpu_count: vh3.Integer = 0,
    openmpi_runner: vh3.Boolean = False,
    openmpi_params: vh3.String = None,
    mpi_processes_count: vh3.Integer = -1,
    before_run_command: vh3.String = None,
    after_run_command: vh3.String = None,
    ttl: vh3.Integer = 1440,
    max_ram: vh3.Integer = 10000,
    max_disk: vh3.Integer = 10000,
    force_tmpfs_disk: vh3.Boolean = False,
    gpu_max_ram: vh3.Integer = 1024,
    gpu_type: vh3.Enum[
        Literal["NONE", "ANY", "CUDA_ANY", "CUDA_3_5", "CUDA_5_2", "CUDA_6_1", "CUDA_7_0", "CUDA_8_0"]
    ] = "NONE",
    strict_gpu_type: vh3.Boolean = False,
    ssh_key: vh3.Secret = None,
    yt_token: vh3.Secret = vh3.Factory(lambda: vh3.context.yt_token),
    mr_account: vh3.String = vh3.Factory(lambda: vh3.context.mr_account),
    auto_snapshot: vh3.Integer = 0,
    nodes_count: vh3.Integer = 1,
    additional_layers: vh3.MultipleStrings = (),
    retries_on_job_failure: vh3.Integer = 0,
    timestamp: vh3.Date = vh3.Factory(lambda: vh3.context.timestamp),
    debug_timeout: vh3.Integer = 0,
    user_requested_secret: vh3.Secret = None,
    job_host_tags: vh3.MultipleStrings = (),
    job_scheduler_instance: vh3.String = None,
    job_scheduler_yt_pool_tree: vh3.String = None,
    job_scheduler_yt_pool: vh3.String = None,
    job_scheduler_yt_token: vh3.Secret = None,
    job_mtn_enabled: vh3.Boolean = True,
    job_scheduler_yt_custom_spec: vh3.String = "{}",
    job_layer_yt_path: vh3.MultipleStrings = (),
    job_variables: vh3.MultipleStrings = (),
    script: vh3.Binary = None,
    data: Sequence[vh3.Binary] = (),
    state: vh3.Binary = None,
    volume: Sequence[vh3.Binary] = (),
    params: vh3.JSON = None
) -> Python3DeepLearningOutput:
    """
    Python 3 Deep Learning
    Python code runner with support of tensorflow, numpy, theano, torch, keras and nirvana_dl library.
    See https://wiki.yandex-team.ru/computervision/projects/deeplearning/nirvanadl/ for user manual.
    :param base_layer: Base porto layer
    :param pip: Libraries to Install
      [[List of libraries to be installed using pip install. Some libraries are already available,
      see https://wiki.yandex-team.ru/computervision/projects/deeplearning/nirvanadl/ for details.
      WARNING: use only with explicit version for reproducibility.]]
    :param install_pydl_package: Install nirvana-dl package
    :param run_command: Run command
      [[Custom bash code]]
    :param openmpi_runner: use openmpi runner
    :param openmpi_params: openmpi runner extra args
    :param before_run_command: Before run command
      [[Command which will be executed before run_command]]
    :param after_run_command: After run command
      [[Command which will be executed after run command]]
    :param strict_gpu_type: strict GPU type
    :param ssh_key: SSH Key
      [[Secret with ssh private key to sync logs with remote server]]
    :param yt_token: YT Token
      [[ID of Nirvana Secret with YT access token (https://nda.ya.ru/3RSzVU). Guide to Nirvana Secrets: https://nda.ya.ru/3RSzWZ]]
    :param mr_account: MR Account:
      [[MR Account Name. By default, output tables and directories will be created in some subdirectory of home/<MR Account>/<workflow owner>/nirvana]]
    :param auto_snapshot: Auto Snapshot
      [[Time interval (minutes) to dump snapshots automatically, without explicit python method call. May cause race condition. If 0, option will be disabled]]
    :param nodes_count: Full number of nodes:
      [[Number of nodes. Should be >= 1]]
      Number of nodes. Should be >= 1
    :param additional_layers: Additional porto layers
      [[IDs of porto layers to put on top of base layer]]
    :param timestamp: Timestamp
    :param job_host_tags: master-job-host-tags
    :param job_mtn_enabled:
      https://st.yandex-team.ru/NIRVANA-12358#5eda1eeb6213d14744d9d791
    :param script:
      Tar-archive with source code inside. Unpacked archive will be available by SOURCE_CODE_PATH environment variable.
    :param data:
      Tar archive(s) with various data. Unpacked data will be available by INPUT_PATH environment variable.
      Multiple archives will be concatenated.
    :param state:
      Saved state of the process, will be used as "state" output snapshot.
    :param volume:
      Additional porto volume(s).
    :param params:
      JSON with additional parameters that can be used in your program.
    """
    raise NotImplementedError("Write your local execution stub here")


@vh3.decorator.external_operation("https://nirvana.yandex-team.ru/operation/8507218f-3cf7-484c-bd9f-a5a8ee218e9b")
@vh3.decorator.nirvana_names(max_disk="_max-disk")
@vh3.decorator.nirvana_output_names("data")
@vh3.decorator.nirvana_names_transformer(vh3.name_transformers.snake_to_dash, options=True)
def beggins_pack_train_val_as_npz_to_tar(
    *,
    mr_account: vh3.String = vh3.Factory(lambda: vh3.context.mr_account),
    yt_token: vh3.Secret = vh3.Factory(lambda: vh3.context.yt_token),
    yql_token: vh3.Secret = vh3.Factory(lambda: vh3.context.yql_token),
    train: vh3.MRTable,
    val: vh3.MRTable,
    max_ram: vh3.Integer = 8192,
    max_disk: vh3.Integer = 8192,
    max_disk1: vh3.Integer = 16384
) -> vh3.OptionalOutput[vh3.Binary]:
    """
    Beggins: pack train/val as npz to tar

    pack

    :param mr_account: MR Account:
      [[MR Account Name.
    By default, output tables and directories will be created in some subdirectory of home/<MR Account>/<workflow owner>/nirvana]]
      MR account name (e.g. `rank_machine`) used to build MR output path for this operation.

      See the `mr-output-path` option for more information
    :param yt_token: YT Token:
      [[ID of Nirvana Secret with YT access token (https://nda.ya.ru/3RSzVU).
    Guide to Nirvana Secrets: https://nda.ya.ru/3RSzWZ]]
      YT OAuth Token.

        [Obtain access token](https://nda.ya.ru/3RSzVU), then [create a Nirvana secret](https://nda.ya.ru/3RSzWZ) and [use it here](https://nda.ya.ru/3RSzWb).
        You can [share the secret](https://nda.ya.ru/3RSzWd) with user(s) and/or a staff group.
    :param yql_token: YQL Token:
      [[YQL OAuth Token, see https://wiki.yandex-team.ru/kikimr/yql/userguide/cli/#autentifikacija]]
      YQL OAuth Token, see https://wiki.yandex-team.ru/kikimr/yql/userguide/cli/#autentifikacija
    :param max_disk: DISK Usage, MiB (tar)
      [[Maximum amount of disk space consumed by the job's data, in megabytes]]
      Maximum amount of disk space consumed by the job's data, in megabytes
    """
    raise NotImplementedError("Write your local execution stub here")


class BegginsExtractModelOutput(NamedTuple):
    model_pb: vh3.Binary
    model_description_json: vh3.JSON


@vh3.decorator.external_operation("https://nirvana.yandex-team.ru/operation/2f1fc513-a7f7-446c-b9d1-cb979498132d")
@vh3.decorator.nirvana_output_names(model_pb="model.pb", model_description_json="model_description.json")
def beggins_extract_model(model: vh3.Binary) -> BegginsExtractModelOutput:
    """
    Beggins: extract model

    extract

    :param model:
      tar with `xxx/model.pb` and `xxx/model_description.json`
    """
    raise NotImplementedError("Write your local execution stub here")


@vh3.decorator.external_operation("https://nirvana.yandex-team.ru/operation/a3be8c29-be15-42d5-8d51-f4f381f06560")
@vh3.decorator.nirvana_names(model_pb="model.pb", model_description_json="model_description.json")
@vh3.decorator.nirvana_output_names("end_tag")
def beggins_upload_model_to_sandbox(
    *,
    token: vh3.Secret = vh3.Factory(lambda: vh3.context.sandbox_token),
    model_name: vh3.String,
    model_pb: vh3.Binary,
    model_description_json: vh3.JSON,
    user: vh3.String = "VINS"
) -> vh3.JSON:
    """
    Beggins: upload model to sandbox

    upload model

    :param token: Sandboox OAuth token
      [[see secret docs https://wiki.yandex-team.ru/jandekspoisk/nirvana/vodstvo/rabota-s-sekretami-v-nirvane/]]
    :param model_name: Model name
    :param user: Resource owner
    """
    raise NotImplementedError("Write your local execution stub here")


@vh3.decorator.external_operation("https://nirvana.yandex-team.ru/operation/37c760c9-84dd-43f2-854d-c74f980ae933")
@vh3.decorator.nirvana_output_names("archive")
@vh3.decorator.nirvana_names_transformer(vh3.name_transformers.snake_to_dash, options=True)
def add_to_tar(
    *,
    path: vh3.String,
    file: Union[
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
        vh3.MRTable,
        vh3.TSV,
        vh3.Text,
        vh3.XML,
    ],
    ttl: vh3.Integer = 360,
    max_ram: vh3.Integer = 100,
    max_disk: vh3.Integer = 1024,
    unpack: vh3.Boolean = False,
    job_metric_tag: vh3.String = None,
    archive: vh3.Binary = None
) -> vh3.Binary:
    """
    Add to tar

    Add file to tar archive by given path

    :param path: Path
      [[Path to add file]]
    :param file:
      File to be added
    :param unpack: Unpack
      [[Interpret input archive as a folder]]
    :param job_metric_tag:
      Tag for monitoring of resource usage
    :param archive:
      Tar archive
    """
    raise NotImplementedError("Write your local execution stub here")


class ExtractFromTarOutput(NamedTuple):
    binary_file: vh3.Binary
    exec_file: vh3.Executable
    json_file: vh3.JSON
    tsv_file: vh3.TSV
    text_file: vh3.Text


@vh3.decorator.external_operation("https://nirvana.yandex-team.ru/operation/cb958bd8-4222-421b-8e83-06c01897a725")
@vh3.decorator.nirvana_names(out_type="out_type")
@vh3.decorator.nirvana_names_transformer(vh3.name_transformers.snake_to_dash, options=True)
def extract_from_tar(
    *,
    path: vh3.String,
    out_type: vh3.Enum[Literal["binary", "exec", "json", "tsv", "text"]],
    archive: Union[vh3.Binary, vh3.File],
    ttl: vh3.Integer = 30,
    max_ram: vh3.Integer = 1024,
    max_disk: vh3.Integer = 1024,
    job_metric_tag: vh3.String = None
) -> ExtractFromTarOutput:
    """
    Extract from tar

    Unpack TAR archive and return file or folder content by the given name

    :param path: File path
      [[File path to extract]]
      File name to extract
    :param out_type: Output type
    :param job_metric_tag:
      Tag for monitoring of resource usage
    """
    raise NotImplementedError("Write your local execution stub here")


class PulsarAddInstanceOutput(NamedTuple):
    info: vh3.JSON
    link: vh3.HTML


@vh3.decorator.external_operation("https://nirvana.yandex-team.ru/operation/773c9319-7721-47b7-af2d-399714902d2c")
def pulsar_add_instance(
    *,
    pulsar_token: vh3.Secret = vh3.Factory(lambda: vh3.context.pulsar_token),
    model_name: vh3.String,
    dataset_name: vh3.String,
    model_version: vh3.String = None,
    model_options: vh3.String = None,
    dataset_info: vh3.String = None,
    tags: vh3.MultipleStrings = (),
    user_datetime: vh3.String = None,
    name: vh3.String = None,
    description: vh3.String = None,
    experiment_id: vh3.String = None,
    per_object_data_metainfo: vh3.String = None,
    diff_tool_config: vh3.String = None,
    save_tfevents: vh3.Boolean = False,
    tfevents_ttl: vh3.Integer = None,
    model_uid: vh3.String = None,
    model_scenario: vh3.String = None,
    produced_model_uid: vh3.String = None,
    timestamp: vh3.String = vh3.Factory(lambda: vh3.context.timestamp_string),
    permissions: vh3.String = None,
    metrics: vh3.JSON = None,
    data: Union[vh3.File, vh3.JSON, vh3.MRTable, vh3.TSV, vh3.Text] = None,
    dataset_path: vh3.MRDirectory = None,
    report_url: vh3.JSON = None,
    tfevents: Sequence[Union[vh3.Binary, vh3.File, vh3.MRFile, vh3.MRTable, vh3.Text]] = ()
) -> PulsarAddInstanceOutput:
    """
    Pulsar: Add Instance

    Pulsar. [Wiki](https://wiki.yandex-team.ru/pulsar/). [Change log](https://a.yandex-team.ru/arc/trunk/arcadia/ml/pulsar/nirvana/add_instance_change_log.md).

    :param pulsar_token: Pulsar OAuth token
    :param model_options:
      [[Model options json.]]
      Model options json.
    :param dataset_info:
      [[Dataset info json.]]
      Dataset info json.
    :param user_datetime:
      [[Format: "%Y-%m-%d" or "%Y-%m-%d %H:%M:%S"]]
      Format: "%Y-%m-%d" or "%Y-%m-%d %H:%M:%S"
    :param per_object_data_metainfo:
      [[Format:
    ```
    [
        {
                "name": "name",  # required
                "type": "type",  # optional (default=none)
                "best_value": "Min",  # optional (default=none)
                "show": false,  # optional (default=true)
                "join_type": "inner",  # optional (default=none)
                "aggregate": true  # optional (default=false)
        },
        ...
    ]
    ```]]
      Format:
      ```
      [
        {
                "name": "name",  # required
                "type": "type",  # optional (default=none)
                "best_value": "Min",  # optional (default=none)
                "show": false,  # optional (default=true)
                "join_type": "inner",  # optional (default=none)
                "aggregate": true  # optional (default=false)
        },
        ...
      ]
      ```
    :param diff_tool_config:
      [[Format:
    ```
    {
            "group_order":  ['group_1', 'group_2', ...],
    }
    ```]]
      Format:
      ```
      {
              "group_order":  ['group_1', 'group_2', ...],
      }
      ```
    :param save_tfevents:
      [[If enable - tfevents will be saved as nirvana data to graph quota.]]
      If enable - tfevents will be saved as nirvana data to graph quota.
    :param tfevents_ttl: tfevents_ttl, Days
      [[tfevents ttl in days]]
      tfevents ttl in days
    :param timestamp: Timestamp for caching
      [[Any string used for Nirvana caching only]]
      Any string used for Nirvana caching only
    :param permissions:
      [[Format:
    ```
    {
        "read": ["Yandex", "@login"],
        "write": ["nirvana.user.yandex.staff"],
        "owner": ["@login"]
    }
    ```]]
      Format:
      ```
      {
          "read": ["Yandex", "@login"],
          "write": ["nirvana.user.yandex.staff"],
          "owner": ["@login"]
      }
      ```
    """
    raise NotImplementedError("Write your local execution stub here")


class PulsarAddModelOutput(NamedTuple):
    info: vh3.JSON
    link: vh3.HTML


@vh3.decorator.external_operation("https://nirvana.yandex-team.ru/operation/8a99c4c3-c08c-40ea-832e-2f3facdc9d6b")
def pulsar_add_model(
    *,
    pulsar_token: vh3.Secret = vh3.Factory(lambda: vh3.context.pulsar_token),
    name: vh3.String = None,
    version: vh3.String = None,
    description: vh3.String = None,
    tags: vh3.MultipleStrings = (),
    parent_uid: vh3.String = None,
    model_type: vh3.String = None,
    model_meta: vh3.String = None,
    artifact_ttl: vh3.Integer = None,
    instance_id: vh3.String = None,
    timestamp: vh3.String = vh3.Factory(lambda: vh3.context.timestamp_string),
    artifacts: Sequence[Union[vh3.Binary, vh3.File, vh3.MRDirectory, vh3.MRFile, vh3.MRTable, vh3.Text]] = ()
) -> PulsarAddModelOutput:
    """
    Pulsar: Add Model

    Pulsar. [Wiki](https://wiki.yandex-team.ru/pulsar/). [Change log](https://a.yandex-team.ru/arc/trunk/arcadia/ml/pulsar/nirvana/add_model_change_log.md).

    :param pulsar_token: Pulsar OAuth token
    :param artifact_ttl: artifact_ttl, Days
    """
    raise NotImplementedError("Write your local execution stub here")


class PulsarUpdateInstanceOutput(NamedTuple):
    info: vh3.JSON
    link: vh3.HTML


@vh3.decorator.external_operation("https://nirvana.yandex-team.ru/operation/8ade8f5c-83f8-4e35-93b3-bd382cbb50a2")
def pulsar_update_instance(
    *,
    pulsar_token: vh3.Secret = vh3.Factory(lambda: vh3.context.pulsar_token),
    instance_id: vh3.String = "",
    tags: vh3.MultipleStrings = (),
    user_datetime: vh3.String = None,
    name: vh3.String = None,
    description: vh3.String = None,
    experiment_id: vh3.String = None,
    per_object_data_metainfo: vh3.String = None,
    diff_tool_config: vh3.String = None,
    save_tfevents: vh3.Boolean = False,
    tfevents_ttl: vh3.Integer = None,
    model_uid: vh3.String = None,
    model_scenario: vh3.String = None,
    produced_model_uid: vh3.String = None,
    permissions: vh3.String = None,
    metrics: vh3.JSON = None,
    data: Union[vh3.File, vh3.JSON, vh3.MRTable, vh3.TSV, vh3.Text] = None,
    report_url: vh3.JSON = None,
    tfevents: Sequence[Union[vh3.Binary, vh3.File, vh3.MRFile, vh3.MRTable, vh3.Text]] = ()
) -> PulsarUpdateInstanceOutput:
    """
    Pulsar: Update Instance

    Pulsar. [Wiki](https://wiki.yandex-team.ru/pulsar/). [Change log](https://a.yandex-team.ru/arc/trunk/arcadia/ml/pulsar/nirvana/update_instance_change_log.md).

    :param pulsar_token: Pulsar OAuth token
    :param per_object_data_metainfo:
      [[Format:
    ```
    [
        {
                "name": "name",  # required
                "type": "type",  # optional (default=none)
                "best_value": "Min",  # optional (default=none)
                "show": false,  # optional (default=true)
                "join_type": "inner",  # optional (default=none)
                "aggregate": true,  # optional (default=false)
                "title": "title",  # optional (default=none)
                "tooltip": "tooltip",  # optional (default=none)
                "group_id": "group_1",  # optional (default=none)
        },
        ...
    ]
    ```]]
      Format:
      ```
      [
        {
                "name": "name",  # required
                "type": "type",  # optional (default=none)
                "best_value": "Min",  # optional (default=none)
                "show": false,  # optional (default=true)
                "join_type": "inner",  # optional (default=none)
                "aggregate": true,  # optional (default=false)
                "title": "title",  # optional (default=none)
                "tooltip": "tooltip",  # optional (default=none)
                "group_id": "group_1",  # optional (default=none)
        },
        ...
      ]
      ```
    :param diff_tool_config:
      [[Format:
    ```
    {
            "group_order":  ['group_1', 'group_2', ...],
    }
    ```]]
      Format:
      ```
      {
              "group_order":  ['group_1', 'group_2', ...],
      }
      ```
    :param save_tfevents:
      [[If enable - tfevents will be saved as nirvana data to graph quota.]]
      If enable - tfevents will be saved as nirvana data to graph quota.
    :param tfevents_ttl: tfevents_ttl, Days
      [[tfevents ttl in days]]
      tfevents ttl in days
    :param permissions:
      [[Format:
    ```
    {
        "read": ["Yandex", "@login"],
        "write": ["nirvana.user.yandex.staff"],
        "owner": ["@login"]
    }
    ```]]
      Format:
      ```
      {
          "read": ["Yandex", "@login"],
          "write": ["nirvana.user.yandex.staff"],
          "owner": ["@login"]
      }
      ```
    """
    raise NotImplementedError("Write your local execution stub here")


@vh3.decorator.external_operation("https://nirvana.yandex-team.ru/operation/9a3cd446-bdaf-499d-9bae-6708015968d4")
@vh3.decorator.nirvana_names(body_function="body_function")
@vh3.decorator.nirvana_output_names("output")
@vh3.decorator.nirvana_names_transformer(vh3.name_transformers.snake_to_dash, options=True)
def light_python2_transform_json(
    *,
    ttl: vh3.Integer = 360,
    max_ram: vh3.Integer = 1024,
    cpu_guarantee: vh3.Integer = 1,
    max_disk: vh3.Integer = 1024,
    body_function: vh3.String = "return v",
    input: vh3.JSON = None,
    args: vh3.JSON = None
) -> vh3.JSON:
    """
    [Light] Python2 Transform JSON

    Jinja2, numpy, scipy, scikit-learn, pandas

    :param body_function:
      Function(v) body function
    """
    raise NotImplementedError("Write your local execution stub here")


@vh3.decorator.external_operation("https://nirvana.yandex-team.ru/operation/9614577b-eb46-4e4e-a8c5-eb31e212abe0")
@vh3.decorator.nirvana_names(model_pb="model.pb", model_description_json="model_description.json")
@vh3.decorator.nirvana_output_names("table")
@vh3.decorator.nirvana_names_transformer(vh3.name_transformers.snake_to_dash, options=True)
def beggins_eval_binary_classifier(
    *,
    yt_token: vh3.Secret = vh3.Factory(lambda: vh3.context.yt_token),
    yql_token: vh3.Secret = vh3.Factory(lambda: vh3.context.yql_token),
    model_pb: vh3.Binary,
    model_description_json: vh3.JSON,
    table: vh3.MRTable,
    mr_account: vh3.String = vh3.Factory(lambda: vh3.context.mr_account),
) -> vh3.OptionalOutput[vh3.MRTable]:
    """
    Beggins: eval binary classifier

    eval binary classifier

    :param yt_token: YT Token:
      [[ID of Nirvana Secret with YT access token (https://nda.ya.ru/3RSzVU).
    Guide to Nirvana Secrets: https://nda.ya.ru/3RSzWZ]]
      YT OAuth Token.

        [Obtain access token](https://nda.ya.ru/3RSzVU), then [create a Nirvana secret](https://nda.ya.ru/3RSzWZ) and [use it here](https://nda.ya.ru/3RSzWb).
        You can [share the secret](https://nda.ya.ru/3RSzWd) with user(s) and/or a staff group.
    :param yql_token: YQL Token:
      [[YQL OAuth Token, see https://wiki.yandex-team.ru/kikimr/yql/userguide/cli/#autentifikacija]]
      YQL OAuth Token, see https://wiki.yandex-team.ru/kikimr/yql/userguide/cli/#autentifikacija
    :param mr_account: MR Account:
      [[MR Account Name.
    By default, output tables and directories will be created in some subdirectory of home/<MR Account>/<workflow owner>/nirvana]]
      MR account name (e.g. `rank_machine`) used to build MR output path for this operation.

      See the `mr-output-path` option for more information
    """
    raise NotImplementedError("Write your local execution stub here")


class BegginsAnalyzeThresholdsOutput(NamedTuple):
    notebook: vh3.JSON
    notebook_html: vh3.OptionalOutput[vh3.HTML]


@vh3.decorator.external_operation("https://nirvana.yandex-team.ru/operation/5e1fa95b-0de7-4979-a8eb-b68ac6a72d1e")
@vh3.decorator.nirvana_names_transformer(vh3.name_transformers.snake_to_dash, options=True)
def beggins_analyze_thresholds(
    *,
    yt_token: vh3.Secret = vh3.Factory(lambda: vh3.context.yt_token),
    val: vh3.MRTable,
    test: vh3.MRTable,
    mr_account: vh3.String = vh3.Factory(lambda: vh3.context.mr_account),
) -> BegginsAnalyzeThresholdsOutput:
    """
    Beggins: analyze thresholds

    using notebook

    :param yt_token: YT Token:
      [[ID of Nirvana Secret with YT access token (https://nda.ya.ru/3RSzVU).
    Guide to Nirvana Secrets: https://nda.ya.ru/3RSzWZ]]
      YT OAuth Token.

        [Obtain access token](https://nda.ya.ru/3RSzVU), then [create a Nirvana secret](https://nda.ya.ru/3RSzWZ) and [use it here](https://nda.ya.ru/3RSzWb).
        You can [share the secret](https://nda.ya.ru/3RSzWd) with user(s) and/or a staff group.
    :param mr_account: MR Account:
      [[MR Account Name.
    By default, output tables and directories will be created in some subdirectory of home/<MR Account>/<workflow owner>/nirvana]]
      MR account name (e.g. `rank_machine`) used to build MR output path for this operation.

      See the `mr-output-path` option for more information
    """
    raise NotImplementedError("Write your local execution stub here")


class CatdevboostTrainOutput(NamedTuple):
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


@vh3.decorator.external_operation("https://nirvana.yandex-team.ru/operation/a492a074-aea2-4548-ba6d-49682043dce7")
@vh3.decorator.nirvana_names(
    bootstrap_type="bootstrap_type", od_type="od_type", yt_pool="yt_pool", model_metadata="model_metadata"
)
@vh3.decorator.nirvana_output_names(
    model_bin="model.bin",
    learn_error_log="learn_error.log",
    test_error_log="test_error.log",
    training_log_json="training_log.json",
    plots_html="plots.html",
    tensorboard_log="tensorboard.log",
    training_options_json="training_options.json",
    plots_png="plots.png",
)
@vh3.decorator.nirvana_names_transformer(vh3.name_transformers.snake_to_dash, options=True)
def catdevboost_train(
    *,
    learn: Union[vh3.Binary, vh3.File, vh3.MRTable, vh3.TSV, vh3.Text],
    ttl: vh3.Integer = 3600,
    gpu_type: vh3.Enum[Literal["NONE", "ANY", "CUDA_3_5", "CUDA_5_2", "CUDA_6_1", "CUDA_7_0", "CUDA_8_0"]] = "NONE",
    restrict_gpu_type: vh3.Boolean = False,
    slaves: vh3.Integer = None,
    cpu_guarantee: vh3.Integer = 1600,
    use_catboost_builtin_quantizer: vh3.Boolean = False,
    loss_function: vh3.Enum[
        Literal[
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
    bootstrap_type: vh3.Enum[Literal["Poisson", "Bayesian", "Bernoulli", "MVS", "No"]] = None,
    bagging_temperature: vh3.Number = None,
    subsample: vh3.Number = None,
    sampling_unit: vh3.Enum[Literal["Object", "Group"]] = None,
    rsm: vh3.Number = None,
    leaf_estimation_method: vh3.Enum[Literal["Newton", "Gradient", "Exact"]] = None,
    leaf_estimation_iterations: vh3.Integer = None,
    depth: vh3.Integer = None,
    seed: vh3.Integer = 0,
    create_tensorboard: vh3.Boolean = False,
    use_best_model: vh3.Boolean = True,
    od_type: vh3.Enum[Literal["IncToDec", "Iter"]] = None,
    od_pval: vh3.Number = None,
    eval_metric: vh3.String = None,
    custom_metric: vh3.String = None,
    one_hot_max_size: vh3.Number = None,
    feature_border_type: vh3.Enum[
        Literal["Median", "GreedyLogSum", "UniformAndQuantiles", "MinEntropy", "MaxLogSum"]
    ] = None,
    per_float_feature_quantization: vh3.String = None,
    border_count: vh3.Integer = None,
    target_border: vh3.Number = None,
    has_header: vh3.Boolean = False,
    delimiter: vh3.String = None,
    ignore_csv_quoting: vh3.Boolean = False,
    cv_type: vh3.Enum[Literal["Classical", "Inverted"]] = None,
    cv_fold_index: vh3.Integer = None,
    cv_fold_count: vh3.Integer = None,
    fstr_type: vh3.Enum[Literal["PredictionValuesChange", "LossFunctionChange"]] = None,
    prediction_type: vh3.Enum[Literal["RawFormulaVal", "Probability", "Class"]] = None,
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
    boost_from_average: vh3.Enum[Literal["True", "False"]] = "False",
    job_core_yt_token: vh3.Secret = "",
    test: Union[vh3.Binary, vh3.File, vh3.MRTable, vh3.TSV, vh3.Text] = None,
    cd: Union[vh3.Binary, vh3.File, vh3.MRTable, vh3.TSV, vh3.Text] = None,
    pairs: Union[vh3.Binary, vh3.File, vh3.MRTable, vh3.TSV, vh3.Text] = None,
    test_pairs: Union[vh3.Binary, vh3.File, vh3.MRTable, vh3.TSV, vh3.Text] = None,
    learn_group_weights: Union[vh3.Binary, vh3.File, vh3.MRTable, vh3.TSV, vh3.Text] = None,
    test_group_weights: Union[vh3.Binary, vh3.File, vh3.MRTable, vh3.TSV, vh3.Text] = None,
    catboost_binary: Union[vh3.Binary, vh3.Executable] = None,
    params_file: Union[vh3.JSON, vh3.Text] = None,
    snapshot_file: vh3.Binary = None,
    borders: Union[vh3.File, vh3.TSV, vh3.Text] = None,
    baseline_model: vh3.Binary = None,
    learn_baseline: Union[vh3.Binary, vh3.File, vh3.MRTable, vh3.TSV, vh3.Text] = None,
    test_baseline: Union[vh3.Binary, vh3.File, vh3.MRTable, vh3.TSV, vh3.Text] = None,
    pool_metainfo: vh3.JSON = None
) -> CatdevboostTrainOutput:
    """
    catdevboost_train

    CatBoost is a machine learning algorithm that uses gradient boosting on decision trees

    [Documentation](https://doc.yandex-team.ru/english/ml/catboost/doc/nirvana-operations/catboost__nirvana__train-catboost.html)

    [Change log](https://a.yandex-team.ru/arc/trunk/arcadia/ml/nirvana/catboost/train/change_log.md)

    :param learn:
      Training dataset. [Documentaion](https://tech.yandex.com/catboost/doc/dg/concepts/input-data_values-file-docpage/).
    :param restrict_gpu_type:
      [[Request gpu type exactly as specified]]
      [Doc](https://wiki.yandex-team.ru/nirvana/manual/createoperation/#jobprocessor)
    :param slaves: Number of slaves
      [[or null for auto selection]]
    :param use_catboost_builtin_quantizer: Use CatBoost builtin quantizer
      If checked, do not create quantized pool on YT
    :param loss_function: Loss function
    :param loss_function_param: Loss function param
      [[e.g. `border=0.7` for Logloss]]
    :param iterations: Iterations
    :param learning_rate: Learning rate
      [[0.0314 is a good choice]]
    :param ignored_features: Ignored features
      [[List feature indices to disregard, e.g. `0:5-13:2`]]
    :param l2_leaf_reg: L2 leaf regularization coeff
      [[Any positive value]]
    :param random_strength: Random strength
      [[Coeff at std deviation of score randomization.
    NOT supported for: QueryCrossEntropy, YetiRankPairwise, PairLogitPairwise]]
    :param bootstrap_type: Bootstrap type
      [[Method for sampling the weights of objects.]]
    :param bagging_temperature: Bagging temperature
      [[For Bayesian bootstrap]]
    :param subsample: Sample rate for bagging
    :param sampling_unit: Sampling unit
    :param rsm: Rsm
      [[CPU-only. A value in range (0; 1]]]
    :param leaf_estimation_method: Leaf estimation method
    :param leaf_estimation_iterations: Leaf estimation iterations
    :param depth: Max tree depth
      [[Suggested < 10 on GPU version]]
    :param seed: Random seed
    :param create_tensorboard: Create tensorboard
    :param use_best_model: Use best model
    :param od_type: Overfitting detector type
    :param od_pval: Auto stop PValue
      [[For IncToDec: a small value, [1e-12...1e-2], is a good choice.
    Do not use this this with Iter type overfitting detector.]]
    :param eval_metric: Eval metric (for OD and best model)
      [[Metric name with params, e.g. `Quantile:alpha=0.3`]]
    :param custom_metric: Custom metric
      [[Written during training. E.g. `Quantile:alpha=0.1`]]
    :param one_hot_max_size: One-hot max size
      [[Use one-hot encoding for cat features with number of values <= this size.
    Ctrs are not calculated for such features.]]
      If parameter is specified than features with no more than specified value different values will be
      converted to float features using one-hot encoding. No ctrs will be calculated on this features.
    :param feature_border_type: Border type for num-features
      "feature-border-type", "Should be one of: Median, GreedyLogSum, UniformAndQuantiles, MinEntropy, MaxLogSum"
    :param border_count: Border count for num-features
      [[Must be < 256 for GPU. May be large on CPU.]]
    :param target_border: Border for target binarization
    :param has_header: Has header
    :param delimiter: Delimiter
    :param cv_type: CV type
      [[Classical: train on many, test on 1, Inverted: train on 1, test on many]]
      [Doc](https://doc.yandex-team.ru/ml/catboost/doc/concepts/cli-reference_cross-validation.html)
    :param cv_fold_index: CV fold index
    :param cv_fold_count: CV fold count
    :param fstr_type: Feature importance calculation type
    :param prediction_type: Prediction type
      [[only CPU version]]
    :param output_columns: Output columns
      [[Columns for eval file, e.g. SampleId RawFormulaVal #5:FeatureN
    > https://nda.ya.ru/3VodMf]]
      [Doc](https://doc.yandex-team.ru/ml/catboost/doc/concepts/cli-reference_calc-model.html)
    :param max_ctr_complexity: Max ctr complexity
      [[Maximum number of cat-features in ctr combinations.]]
    :param ctr_leaf_count_limit: ctr leaf count limit (CPU only)
    :param text_processing:  text-processing
    :param args: additional arguments
    :param model_metadata: Model metadata key-value pairs
      [[Key-value pairs, quote key and value for reasonable results]]
    :param inner_options_override: Override options for inner train cube
      [[JSON string, e.g. {"master-max-ram":250000}]]
      Normally this option shall be null. Used to debug/override resource requirements
    :param job_core_yt_token:
      YT token to use when storing coredump

      More info: https://st.yandex-team.ru/YTADMINREQ-25127 https://wiki.yandex-team.ru/nirvana/vodstvo/processory/Job-processor/#sborcoredump-ov
      https://yt.yandex-team.ru/docs/problems/mapreduce_debug#poluchenie-cuda-core-dump-iz-dzhobov-operacii
    :param test:
      Testing dataset. [Documentaion](https://tech.yandex.com/catboost/doc/dg/concepts/input-data_values-file-docpage/).
    :param cd:
      Column description. [Documentation](https://tech.yandex.com/catboost/doc/dg/concepts/input-data_column-descfile-docpage/).
    :param pairs:
      Training pairs. [Documentation](https://tech.yandex.com/catboost/doc/dg/concepts/input-data_pairs-description-docpage/).
    :param test_pairs:
      Testing pairs. [Documentation](https://tech.yandex.com/catboost/doc/dg/concepts/input-data_pairs-description-docpage/).
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
    :param test_group_weights:
      Testing query weights.

      Format:
      ```
      <group_id>\t<weight>\n
      ```

      Example:
      ```
      id1\t0.5
      id2\t0.7
      ```
    :param catboost_binary:
      CatBoost binary.
      Build command:
      ```
      ya make trunk/arcadia/catboost/app/ -r --checkout -DHAVE_CUDA=yes -DCUDA_VERSION=9.0
      ```
    :param params_file:
      JSON file that contains the training parameters, for example:
      ```
      {
          "loss_function": "Logloss",
          "iterations": 400
      }
      ```
    :param snapshot_file:
      Snapshot file. Used for recovering training after an interruption.
    :param borders:
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
    :param learn_baseline:
      Baseline for learn
    :param test_baseline:
      Baseline for test (the first one, if many)
    :param pool_metainfo:
      Metainfo file with feature tags.
    """
    raise NotImplementedError("Write your local execution stub here")


class CatBoostApplyOutput(NamedTuple):
    result: vh3.TSV
    output_yt_table: vh3.MRTable


@vh3.decorator.external_operation("https://nirvana.yandex-team.ru/operation/f7981d64-2eb4-4916-976d-80572ef5cf72")
@vh3.decorator.nirvana_names(yt_pool="yt_pool", model_bin="model.bin")
@vh3.decorator.nirvana_names_transformer(vh3.name_transformers.snake_to_dash, options=True)
def cat_boost_apply(
    *,
    pool: Union[vh3.Binary, vh3.File, vh3.MRTable, vh3.TSV, vh3.Text],
    model_bin: vh3.Binary,
    tree_count_limit: vh3.Integer = None,
    prediction_type: vh3.Enum[Literal["RawFormulaVal", "Class", "Probability"]] = "RawFormulaVal",
    output_columns: vh3.MultipleStrings = (),
    has_header: vh3.Boolean = False,
    delimiter: vh3.String = None,
    ignore_csv_quoting: vh3.Boolean = False,
    args: vh3.String = None,
    yt_token: vh3.Secret = vh3.Factory(lambda: vh3.context.yt_token),
    ttl: vh3.Integer = 360,
    yt_pool: vh3.String = None,
    cpu_guarantee: vh3.Integer = 800,
    cd: Union[vh3.Binary, vh3.File, vh3.MRTable, vh3.TSV, vh3.Text] = None,
    catboost_external_binary: Union[vh3.Binary, vh3.Executable] = None,
    output_yt_table: vh3.MRTable = None
) -> CatBoostApplyOutput:
    """
    CatBoost: Apply

    CatBoost is a machine learning algorithm that uses gradient boosting on decision trees.

    [Doc](https://doc.yandex-team.ru/english/ml/catboost/doc/nirvana-operations/catboost__nirvana__apply-catboost-model.html)

    [Change log](https://a.yandex-team.ru/arc/trunk/arcadia/ml/nirvana/catboost/apply_model/change_log.md)

    :param pool:
      Dataset. [Documentaion](https://tech.yandex.com/catboost/doc/dg/concepts/input-data_values-file-docpage/).
    :param model_bin:
      Trained model file.
    :param tree_count_limit: Tree count limit
      [[The number of trees from the model to use when applying. If specified, the first <value> trees are used.]]
      The number of trees from the model to use when applying. If specified, the first <value> trees are used.
    :param prediction_type: Prediction type
      [[Prediction type. Supported prediction types: `Probability`, `Class`, `RawFormulaVal`.]]
      Prediction type. Supported prediction types: `Probability`, `Class`, `RawFormulaVal`.
    :param output_columns:
      [[A comma-separated list of prediction types. Supported prediction types: `Probability`, `Class`,
      `RawFormulaVal`, `Label`, `DocId`, `GroupId`, `Weight` and other column types from
      [here](https://tech.yandex.com/catboost/doc/dg/concepts/input-data_column-descfile-docpage/#input-data_column-descfile).]]
      A comma-separated list of prediction types. Supported prediction types: `Probability`, `Class`,
      `RawFormulaVal`, `Label`, `DocId`, `GroupId`, `Weight` and other column types from
      [here](https://tech.yandex.com/catboost/doc/dg/concepts/input-data_column-descfile-docpage/#input-data_column-descfile).
    :param has_header:
      [[Read the column names from the first line if this parameter is set to True.]]
      Read the column names from the first line if this parameter is set to True.
    :param delimiter:
      [[The delimiter character used to separate the data in the train/test input file.]]
      The delimiter character used to separate the data in the train/test input file.
    :param args: Additional args
    :param cd:
      Column description. [Documentation](https://tech.yandex.com/catboost/doc/dg/concepts/input-data_column-descfile-docpage/).
    :param catboost_external_binary:
      CatBoost binary.
      Build command:
      ```
      ya make trunk/arcadia/catboost/app/ -r --checkout -DHAVE_CUDA=yes -DCUDA_VERSION=9.0
      ```
    """
    raise NotImplementedError("Write your local execution stub here")


@vh3.decorator.external_operation("https://nirvana.yandex-team.ru/operation/cd871290-57b2-470d-a575-f999abe45bcf")
@vh3.decorator.nirvana_names(max_disk="max-disk", job_metric_tag="job-metric-tag")
@vh3.decorator.nirvana_output_names("end_tag")
def upload_to_sandbox(
    *,
    type: vh3.String,
    user: vh3.String,
    token: vh3.Secret = vh3.Factory(lambda: vh3.context.sandbox_token),
    data: Union[
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
        vh3.MRTable,
        vh3.TSV,
        vh3.Text,
        vh3.XML,
    ],
    sandbox_ttl: vh3.Integer = 1,
    filename: vh3.String = "data",
    attributes: vh3.MultipleStrings = (),
    extract: vh3.Boolean = False,
    do_not_remove: vh3.Boolean = False,
    max_disk: vh3.Integer = 10000,
    job_metric_tag: vh3.String = None
) -> vh3.JSON:
    """
    Upload to sandbox

    :param type: Resource type
    :param user: Resource owner
    :param token: OAuth token (nirvana secret)
      [[see secret docs https://wiki.yandex-team.ru/jandekspoisk/nirvana/vodstvo/rabota-s-sekretami-v-nirvane/]]
    :param sandbox_ttl: Resource TTL
    :param filename: Filename
      [[optional name of file with resource]]
    :param attributes: Resource attributes
      [[each attribute have format name=value]]
      attributes of resource, each attribute have format name=value
    :param extract: Extract tar archive
      [[Eextract input file as tar archive before uploading]]
    :param do_not_remove: Sandbox TTL=inf
    :param job_metric_tag:
      Tag for monitoring of resource usage
    """
    raise NotImplementedError("Write your local execution stub here")


class Python3JsonProcessOutput(NamedTuple):
    out1: vh3.OptionalOutput[vh3.JSON]
    out2: vh3.OptionalOutput[vh3.JSON]
    html_report: vh3.OptionalOutput[vh3.HTML]


@vh3.decorator.external_operation("https://nirvana.yandex-team.ru/operation/ef1681df-f12e-4068-8b1b-ee3080973a07")
@vh3.decorator.nirvana_names(yt_read="yt_read")
@vh3.decorator.nirvana_names_transformer(vh3.name_transformers.snake_to_dash, options=True)
def python3_json_process(
    *,
    ttl: vh3.Integer = 1440,
    max_disk: vh3.Integer = 1024,
    max_ram: vh3.Integer = 1024,
    code: vh3.String = "def main(in1, in2, in3, mr_tables, token1=None, token2=None, param1=None, param2=None, html_file=None):\n    return in1",
    token1: vh3.Secret = None,
    token2: vh3.Secret = None,
    param1: vh3.String = None,
    param2: vh3.String = None,
    yt_read: vh3.Boolean = False,
    in1: Sequence[vh3.JSON] = (),
    in2: Sequence[vh3.JSON] = (),
    in3: Sequence[vh3.JSON] = (),
    mr: Sequence[vh3.MRTable] = (),
    scripts_archive: Sequence[vh3.Binary] = ()
) -> Python3JsonProcessOutput:
    """
    Python3 Json Process

    Выполнение произвольного python3 кода из текстовой опции кубика
    * 3 json входа, 2 json выхода, html выход
      * Все выходы опциональны.
      * Json результат в оба выхода можно возвращать как `return list1, list2`, так и генератором `yield item1, item2`
    * содержит MR вход для того, чтобы можно было передать YT таблицы в отдельный вход
      * Опция yt_read для того, чтобы считать содержимое YT таблиц (как кубик YT Get), в том числе с разных yt кластеров (требует указания YT/YQL токена в опции token1)
        * Данные из первых 3-х YT таблиц будут считаны в `in1, in2, in3`
    * `html_file.write('<html><body>...')` — позволяет возвращать html
    * Поддерживат 2 Secret'а, 2 параметра, которые можно задать из глобальных опций
    * Содержит `python 3.6.9` и много полезных python модулей, включая `yt, nile`
    * Поддерживает архивы (например можно код из аркадии подать на вход)

    Примеры использования кубика: https://nda.ya.ru/t/utq5c4VG3VxH2z
    Про python3 porto-слой: https://clubs.at.yandex-team.ru/python/3062

    :param code: Python code
    :param token1: Token1
      [[Pass any secret token]]
    :param token2: Token2
      [[Pass any secret token]]
    :param param1: Any param 1
      [[Pass any string param]]
    :param param2: Any param 2
      [[Pass any string param]]
    :param yt_read: Read YT tables to inputs
      [[При включенной галке содержимое первых трёх YT табличек из 'mr'-инпута будет считано в in1, in2, in3 как yt.wrapper.format.RowsIterator]]
    """
    raise NotImplementedError("Write your local execution stub here")


@vh3.decorator.external_operation("https://nirvana.yandex-team.ru/operation/6ef6b6f1-30c4-4115-b98c-1ca323b50ac0")
@vh3.decorator.nirvana_names(yt_token="yt-token", base_path="base_path")
@vh3.decorator.nirvana_output_names("outTable")
@vh3.decorator.nirvana_names_transformer(vh3.name_transformers.snake_to_camel)
def get_mr_table(
    *,
    cluster: vh3.Enum[
        Literal[
            "hahn",
            "banach",
            "freud",
            "marx",
            "hume",
            "arnold",
            "markov",
            "bohr",
            "landau",
            "seneca-vla",
            "seneca-sas",
            "seneca-man",
        ]
    ] = vh3.Factory(lambda: vh3.context.mr_default_cluster),
    creation_mode: vh3.Enum[Literal["NO_CHECK", "CHECK_EXISTS"]] = "NO_CHECK",
    table: vh3.String = None,
    yt_token: vh3.Secret = vh3.Factory(lambda: vh3.context.yt_token),
    file_with_table_name: vh3.Text = None,
    base_path: Union[vh3.MRDirectory, vh3.MRFile, vh3.MRTable] = None
) -> vh3.MRTable:
    """
    Get MR Table

    Creates a reference to MR Table, either existing or potential.
      * If input `fileWithTableName` is present, its first line will be used as the table's path. If not, `table` option value will be used instead.
      * If `base_path` input is present, table path will be treated as *relative* and resolved against `base_path`. If not, path will be treated as *absolute*.

    :param cluster: Cluster:
      [[MR Cluster this table is on]]
      MR Cluster name, recognized by MR processor and FML processor.
      * If not set, `base_path`'s cluster will be used
      * If both `cluster` option value and `base_path` input are present, cluster name specified in **option** will be used
    :param creation_mode: Creation Mode:
      [[Actions to take when getting the MR Table]]
      MR Path creation mode. Specifies additional actions to be taken when getting the path
    :param table: Table:
      [[Path to MR Table]]
      Path to MR table. Used when `fileWithTableName` input is absent.
      * If `base_path` input is absent, this is an absolute path.
      * If `base_path` input is present, this is a relative path.
    :param yt_token: YT Token:
      [[(Optional) Token used if Creation Mode is "Check that Path Exists".
    Write the name of Nirvana Secret holding your YT Access Token here.]]
      *(Optional)* YT OAuth Token to use in "Check that Path Exists" Creation Mode. If not specified, MR Processor's token will be used.

      [Obtain access token](https://nda.ya.ru/3RSzVU), then [create a Nirvana secret](https://nda.ya.ru/3RSzWZ) and [use it here](https://nda.ya.ru/3RSzWb).
      You can [share the secret](https://nda.ya.ru/3RSzWd) with user(s) and/or a staff group.
    :param file_with_table_name:
      Text file with MR table path on its first line. If this input is absent, `table` option value will be used instead.
      * If `base_path` input is absent, this is an absolute path.
      * If `base_path` input is present, this is a relative path.
    :param base_path:
      Base path to resolve against.

      If absent, table path is considered absolute.
    """
    raise NotImplementedError("Write your local execution stub here")


class ScraperOverYtDownloaderWithHttpNoApphostFetcherOutput(NamedTuple):
    output_directory: vh3.MRDirectory
    output_table: vh3.MRTable
    error_table: vh3.MRTable


@vh3.decorator.external_operation("https://nirvana.yandex-team.ru/operation/392006bc-0892-4afd-9599-bfa02c60a52d")
@vh3.decorator.nirvana_names(
    scraper_over_yt_pool="ScraperOverYtPool",
    execution_timeout="ExecutionTimeout",
    retries_per_row="RetriesPerRow",
    fetch_timeout="FetchTimeout",
    max_rps="MaxRPS",
    contour_override="contour_override",
    tvm_id="tvmId",
)
@vh3.decorator.nirvana_names_transformer(vh3.name_transformers.snake_to_dash, options=True)
def scraper_over_yt_downloader_with_http_no_apphost_fetcher(
    *,
    soy_token: vh3.Secret = vh3.Factory(lambda: vh3.context.soy_token),
    yt_token: vh3.Secret = vh3.Factory(lambda: vh3.context.yt_token),
    input_table: vh3.MRTable,
    mr_output_path: vh3.String = None,
    mr_account: vh3.String = vh3.Factory(lambda: vh3.context.mr_account),
    yt_pool: vh3.String = None,
    ttl: vh3.Integer = 10000,
    scraper_over_yt_pool: vh3.String = vh3.Factory(lambda: vh3.context.soy_pool),
    execution_timeout: vh3.String = vh3.Factory(lambda: vh3.context.soy_execution_timeout),
    retries_per_row: vh3.Integer = 20,
    fetch_timeout: vh3.Integer = 10000,
    max_rps: vh3.Integer = 20,
    contour_override: vh3.Enum[Literal["1", "2", "6"]] = None,
    timestamp: vh3.Date = vh3.Factory(lambda: vh3.context.timestamp),
    mr_output_ttl: vh3.Integer = None,
    tvm_id: vh3.Integer = None
) -> ScraperOverYtDownloaderWithHttpNoApphostFetcherOutput:
    """
    ScraperOverYt Downloader (with http (no-apphost) fetcher)

    :param soy_token:
      [[SOY OAuth Token.

      [Obtain access token](https://nda.ya.ru/t/EHwQ1H_s3Vtnrg), then [create a Nirvana secret](https://nda.ya.ru/3RSzWZ) and [use it here](https://nda.ya.ru/3RSzWb).
      You can [share the secret](https://nda.ya.ru/3RSzWd) with user(s) and/or a staff group.]]
      SOY OAuth Token.

        [Obtain access token](https://nda.ya.ru/t/EHwQ1H_s3Vtnrg), then [create a Nirvana secret](https://nda.ya.ru/3RSzWZ) and [use it here](https://nda.ya.ru/3RSzWb).
        You can [share the secret](https://nda.ya.ru/3RSzWd) with user(s) and/or a staff group.
    :param yt_token: YT Token:
      [[ID of Nirvana Secret with YT access token (https://nda.ya.ru/3RSzVU).
    Guide to Nirvana Secrets: https://nda.ya.ru/3RSzWZ]]
      YT OAuth Token.

        [Obtain access token](https://nda.ya.ru/3RSzVU), then [create a Nirvana secret](https://nda.ya.ru/3RSzWZ) and [use it here](https://nda.ya.ru/3RSzWb).
        You can [share the secret](https://nda.ya.ru/3RSzWd) with user(s) and/or a staff group.
    :param input_table:
      input table
    :param mr_output_path:
      Directory for output MR tables and directories.

      Limited templating is supported: `${param["..."]}`, `${meta["..."]}`, `${mr_input["..."]}` (path to input MR *directory*) and `${uniq}` (= unique path-friendly string).

      The default template for `mr-output-path` is

              home[#if param["mr-account"] != meta.owner]/${param["mr-account"]}[/#if]/${meta.owner}/nirvana/${meta.operation_uid}

      If output path does not exist, it will be created.

      Temporary directory, `${mr_tmp}`, is derived from output path in an unspecified way. It is ensured that:
        * It will exist before `job-command` is started
        * It need not be removed manually after execution ends. However, you **should** remove all temporary data created in `${mr_tmp}`, even if your command fails
    :param mr_account: MR Account:
      [[MR Account Name.
    By default, output tables and directories will be created in some subdirectory of home/<MR Account>/<workflow owner>/nirvana]]
      MR account name (e.g. `rank_machine`) used to build MR output path for this operation.

      See the `mr-output-path` option for more information
    :param yt_pool: YT Pool:
      [[Pool used by YT scheduler. Leave blank to use default pool.
    This option has no effect on YaMR.]]
      Pool used by [YT operation scheduler](https://nda.ya.ru/3Rk4af). Leave this blank to use default pool.
    :param execution_timeout:
      ttl for download
    :param fetch_timeout:
      [[Fetch timeout for individual requests in millseconds]]
      Fetch timeout for individual requests in millseconds
    :param contour_override:
      [[Empty = Production
    Overrides SOY contour from production, to selected testing contour. Do not set this option unless tou know what you are doing]]
      Empty = Production
      Overrides SOY contour from production, to selected testing contour. Do not set this option unless tou know what you are doing
    :param timestamp: Timestamp
      ts for caching
    """
    raise NotImplementedError("Write your local execution stub here")


class RunBashCommandOutput(NamedTuple):
    out1: vh3.File
    out2: vh3.File
    out3: vh3.File
    out4: vh3.File
    out5: vh3.File


@vh3.decorator.external_operation("https://nirvana.yandex-team.ru/operation/2d349ba2-f84a-4a55-ac2e-19393932f7d3")
@vh3.decorator.nirvana_names_transformer(vh3.name_transformers.snake_to_dash, options=True)
def run_bash_command(
    *,
    ttl: vh3.Integer = 360,
    max_ram: vh3.Integer = 100,
    cpu_guarantee: vh3.Integer = 1,
    max_disk: vh3.Integer = 1024,
    command: vh3.String = "uptime > $out1",
    timestamp: vh3.Date = vh3.Factory(lambda: vh3.context.timestamp),
    in1: Union[vh3.Binary, vh3.Executable, vh3.File, vh3.Text] = None,
    in2: Union[vh3.Binary, vh3.Executable, vh3.File, vh3.Text] = None,
    in3: Union[vh3.Binary, vh3.Executable, vh3.File, vh3.Text] = None,
    in4: Union[vh3.Binary, vh3.Executable, vh3.File, vh3.Text] = None,
    in5: Union[vh3.Binary, vh3.Executable, vh3.File, vh3.Text] = None,
    in6: Union[vh3.Binary, vh3.Executable, vh3.File, vh3.Text] = None,
    in7: Union[vh3.Binary, vh3.Executable, vh3.File, vh3.Text] = None
) -> RunBashCommandOutput:
    """
    Run bash command

    :param command: Bash command
      Bash command. Use variables $In1, $In2... for inputs and $Out1, $Out2 for outputs
    :param timestamp: Timestamp
      Timestamp
    """
    raise NotImplementedError("Write your local execution stub here")


@vh3.decorator.external_operation("https://nirvana.yandex-team.ru/operation/894cb084-f640-445a-a116-83ebc3d50d33")
@vh3.decorator.nirvana_names(file="File")
@vh3.decorator.nirvana_output_names("json")
def convert_any_to_json(
    *,
    file: Union[
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
    ],
    sync: Sequence[
        Union[
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
) -> vh3.JSON:
    """
    Convert any to Json
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


@vh3.decorator.external_graph("https://nirvana.yandex-team.ru/process/979b5bce-6d7c-4cdc-8cb5-a389fefa7243")
@vh3.decorator.nirvana_names(without_part="without_part", arcadia_revision_overwrite="arcadia_revision_overwrite")
@vh3.decorator.nirvana_output_names("embedded")
@vh3.decorator.nirvana_names_transformer(vh3.name_transformers.snake_to_dash, options=True)
def add_zeliboba_embeddings(
    *,
    yt_token: vh3.Secret = vh3.Factory(lambda: vh3.context.yt_token),
    input: vh3.MRTable,
    without_part: vh3.Boolean = False,
    prefix: vh3.String = "text",
    dst_column: vh3.String = "sentence_embedding",
    mr_account: vh3.String = vh3.Factory(lambda: vh3.context.mr_account),
    batch_size: vh3.Integer = 32,
    max_inp_len: vh3.Integer = 2048,
    max_data_size_per_job: vh3.Integer = 1097152,
    yt_pool_tree: vh3.Enum[Literal["gpu_tesla_v100", "gpu_tesla_a100", "gpu_tesla_a100_80g"]] = "gpu_tesla_a100_80g",
    arcadia_revision_overwrite: vh3.Integer = None
) -> vh3.MRTable:
    """
    add-zeliboba-embeddings

    :param prefix: Column to embed
      [[Колонка с одной строкой, которую нужно поскорить.]]
    :param dst_column: Destination Column
    :param mr_account: MR Account
    :param batch_size: Batch Size
    :param max_inp_len: Max inp length in Tokens
    :param max_data_size_per_job: Max data size per job
    :param yt_pool_tree: Yt pool tree
    :param arcadia_revision_overwrite: Arcadia Revision Overwrite
      [[Перетереть каноничную ревизию для бинарника]]
    """
    raise NotImplementedError("Write your local execution stub here")


@vh3.decorator.external_operation("https://nirvana.yandex-team.ru/operation/7031e6fe-b230-4070-9414-d96d8f3def1c")
@vh3.decorator.nirvana_output_names("text")
@vh3.decorator.nirvana_names_transformer(vh3.name_transformers.snake_to_dash, options=True)
def single_option_to_text_optional_output(
    *, text: vh3.String = None, ttl: vh3.Integer = 360, max_ram: vh3.Integer = 100
) -> vh3.OptionalOutput[vh3.Text]:
    """
    Single option to Text (optional output)

    :param text: Text
    """
    raise NotImplementedError("Write your local execution stub here")


@vh3.decorator.external_operation("https://nirvana.yandex-team.ru/operation/3ff69021-7af0-4067-9f13-ffe9ce7bc1b7")
@vh3.decorator.nirvana_output_names("archive")
@vh3.decorator.nirvana_names_transformer(vh3.name_transformers.snake_to_dash, options=True)
def add_to_tar_if_exists(
    *,
    path: vh3.String,
    ttl: vh3.Integer = 360,
    max_ram: vh3.Integer = 100,
    max_disk: vh3.Integer = 1024,
    unpack: vh3.Boolean = False,
    archive: vh3.Binary = None,
    file: Union[
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
        vh3.MRTable,
        vh3.TSV,
        vh3.Text,
        vh3.XML,
    ] = None
) -> vh3.Binary:
    """
    Add to tar if exists

    Add file to tar archive by given path if file is given. Return the original archive else.

    :param path: Path
      [[Path to add file]]
    :param unpack: Unpack
      [[Interpret input archive as a folder]]
    :param archive:
      Tar archive
    :param file:
      File to be added
    """
    raise NotImplementedError("Write your local execution stub here")


@vh3.decorator.external_operation("https://nirvana.yandex-team.ru/operation/b79437fe-b6d5-43ec-a9c9-852fa7074fa8")
@vh3.decorator.nirvana_names(source_name="source_name")
@vh3.decorator.nirvana_output_names("output")
@vh3.decorator.nirvana_names_transformer(vh3.name_transformers.snake_to_dash, options=True)
def process_dataset_manifest(
    *,
    yt_token: vh3.Secret = 'robot-beggins_yt-token',
    source_name: vh3.String = None,
    mr_account: vh3.String = None,
    yt_pool: vh3.String = None,
    ttl: vh3.Integer = 360,
    max_ram: vh3.Integer = 100,
    mr_output_policy: vh3.Enum[Literal["CREATE_ALL", "CREATE_DIR", "DO_NOTHING"]] = "DO_NOTHING",
    mr_transaction_policy: vh3.Enum[Literal["MANUAL", "AUTO"]] = "MANUAL",
    mr_default_cluster: vh3.String = 'hahn',
    restrict_gpu_type: vh3.Boolean = False,
    manifest: Union[vh3.Binary, vh3.File, vh3.JSON] = None,
    standard_dataset: Sequence[vh3.MRTable] = (),
    analytics_basket: Sequence[vh3.MRTable] = ()
) -> vh3.MRTable:
    """
    process_dataset_manifest


    released from https://a.yandex-team.ru/arc/trunk/arcadia/alice/beggins/internal/vh/cmd/process_manifest?rev=9456101

    :param yt_token: YT Token:
      YT OAuth Token.
        [Obtain access token](https://nda.ya.ru/3RSzVU), then [create a Nirvana secret](https://nda.ya.ru/3RSzWZ) and [use it here](https://nda.ya.ru/3RSzWb).
        You can [share the secret](https://nda.ya.ru/3RSzWd) with user(s) and/or a staff group.
      ID of Nirvana Secret with YT access token (https://nda.ya.ru/3RSzVU).
      Guide to Nirvana Secrets: https://nda.ya.ru/3RSzWZ
    :param mr_account: MR Account:
      MR account name (e.g. `rank_machine`) used to build MR output path for this operation.
      See the `mr-output-path` option for more information
      MR Account Name.
      By default, output tables and directories will be created in some subdirectory of home/<MR Account>/<workflow owner>/nirvana
    :param yt_pool: YT Pool:
      Pool used by [YT operation scheduler](https://nda.ya.ru/3Rk4af). Leave this blank to use default pool.
      Pool used by YT scheduler. Leave blank to use default pool.
      This option has no effect on YaMR.
    :param mr_output_policy: MR output policy:
      By default operation creates output objects, you may disable this behavior: https://wiki.yandex-team.ru/nirvana/vodstvo/processory/mrprocessor/#mr-output-policy
    :param mr_transaction_policy: MR transaction policy:
      Nirvana can automatically prepare, ping and finish MR transaction for this operation: https://nda.ya.ru/3UWdsN
    :param mr_default_cluster: mr default cluster
      Default name for MR-cluster. It's mandatory if there is no inputs such as "MR Table", "MR Directory" or "MR File"
    :param restrict_gpu_type:
      If True, the first value from table will be chosen
      gpu_tesla_k40 -- gpu-type = CUDA_3_5;
      gpu_tesla_m40 -- gpu-type = CUDA_5_2
      gpu_geforce_1080ti -- gpu-type = CUDA_6_1 && gpu-max-ram < 11000
      gpu_tesla_p40 -- gpu-type = CUDA_6_1 && gpu-max-ram >= 11000
      gpu_tesla_v100 -- gpu-type = CUDA_7_0
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


@vh3.decorator.external_operation("https://nirvana.yandex-team.ru/operation/f6795057-eaf5-4b9b-8242-1484fbe1ee5d")
@vh3.decorator.nirvana_names(
    import_="import",
    global_="global",
    input0_type="input0_type",
    input1_type="input1_type",
    input2_type="input2_type",
    output_type="output_type",
)
@vh3.decorator.nirvana_output_names("output")
@vh3.decorator.nirvana_names_transformer(vh3.name_transformers.snake_to_dash, options=True)
def python3_any_any_any_to_txt(
    *,
    input0_type: vh3.Enum[
        Literal["file", "none", "txt-gen", "txt-mem", "tsv-gen", "tsv-mem", "json-gen", "json-mem", "xml"]
    ],
    input1_type: vh3.Enum[
        Literal["file", "none", "txt-gen", "txt-mem", "tsv-gen", "tsv-mem", "json-gen", "json-mem", "xml"]
    ],
    input2_type: vh3.Enum[
        Literal["file", "none", "txt-gen", "txt-mem", "tsv-gen", "tsv-mem", "json-gen", "json-mem", "xml"]
    ],
    ttl: vh3.Integer = 360,
    max_ram: vh3.Integer = 1024,
    max_disk: vh3.Integer = 16384,
    import_: vh3.String = None,
    global_: vh3.String = None,
    body: vh3.String = "return v",
    output_type: vh3.Enum[Literal["txt", "file", "none"]] = "txt",
    job_environments: vh3.MultipleStrings = (),
    yt_token: vh3.Secret = None,
    mr_account: vh3.String = None,
    yt_pool: vh3.String = None,
    mr_default_cluster: vh3.Enum[
        Literal[
            "hahn",
            "freud",
            "marx",
            "hume",
            "arnold",
            "markov",
            "bohr",
            "landau",
            "seneca-vla",
            "seneca-sas",
            "seneca-man",
        ]
    ] = "hahn",
    input0: Union[
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
    ] = None,
    input1: Union[
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
    ] = None,
    input2: Union[
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
) -> vh3.Text:
    """
    Python3 any+any+any to txt

    Transform input data using Python3

    Documentation and examples: https://wiki.yandex-team.ru/mlmarines/operations/pythontransform

    =======

    The operation was created by dj/nirvana/nirvana_make tool.

    See https://nda.ya.ru/3UUaov for updating the operation.

    Please, do not update the operation manually!


    Update command:

    dj/nirvana/nirvana_make/operation_make --update data/python_transform/py3_any+any+any_to_txt --root yt://hahn/home/mlmarines/common/nirvana_make/operations --import dj/nirvana/operations


    Svn info:

        URL: svn://arcadia.yandex.ru/arc/trunk/arcadia

        Last Changed Rev: 9079395

        Last Changed Author: arumyan

        Last Changed Date: 2022-01-27T15:45:21.642352Z


    Other info:

        Build by: sandbox

        Top src dir: /place/sandbox-data/tasks/5/1/1201657915/__FUSE/mount_point_89de9e5d-1123-471f-b8c4-4b9aab2f3be5

        Top build dir: /place/sandbox-data/tasks/5/1/1201657915/__FUSE/mount_point_89de9e5d-1123-471f-b8c4-4b9aab2f3be5

        Hostname: linux-ubuntu-12-04-precise

        Host information:

            Linux linux-ubuntu-12-04-precise 4.19.183-42.2mofed #1 SMP Wed Apr 21 14:40:19 UTC 2021 x86_64 x86_64 x86_64 GNU/Linux



    :param input0_type: Input0 type
      [[Type of input0]]
      Type of input0
    :param input1_type: Input1 type
      [[Type of input1]]
      Type of input1
    :param input2_type: Input2 type
      [[Type of input2]]
      Type of input2
    :param import_: Import modules
      [[Modules to import, comma separated]]
      Modules to import, comma separated
    :param global_: Global definitions
      [[Global definitions, multiline allowed]]
      Global definitions, multiline allowed
    :param body: Function(v, w, x) body
      [[Body for function]]
      Body for function
    :param output_type: Output type
      [[Type of output]]
      Type of output
    :param yt_token: YT Token:
      [[ID of Nirvana Secret with YT access token (https://nda.ya.ru/3RSzVU).
    Guide to Nirvana Secrets: https://nda.ya.ru/3RSzWZ]]
      YT OAuth Token.

        [Obtain access token](https://nda.ya.ru/3RSzVU), then [create a Nirvana secret](https://nda.ya.ru/3RSzWZ) and [use it here](https://nda.ya.ru/3RSzWb).
        You can [share the secret](https://nda.ya.ru/3RSzWd) with user(s) and/or a staff group.
    :param mr_account: MR Account:
      [[MR Account Name.
    By default, output tables and directories will be created in some subdirectory of home/<MR Account>/<workflow owner>/nirvana]]
      MR account name (e.g. `rank_machine`) used to build MR output path for this operation.

      See the `mr-output-path` option for more information
    :param yt_pool: YT Pool:
      [[Pool used by YT scheduler. Leave blank to use default pool.
    This option has no effect on YaMR.]]
      Pool used by [YT operation scheduler](https://nda.ya.ru/3Rk4af). Leave this blank to use default pool.
    :param mr_default_cluster: Default YT cluster:
      [[Default YT cluster]]
      Default YT cluster
    :param input0:
      Input0
    :param input1:
      Input1
    :param input2:
      Input2
    """
    raise NotImplementedError("Write your local execution stub here")


@vh3.decorator.external_operation("https://nirvana.yandex-team.ru/operation/0758edd2-23d9-4e45-baa5-b5b5ffd2a926")
@vh3.decorator.nirvana_output_names("result_dict")
@vh3.decorator.nirvana_names_transformer(vh3.name_transformers.snake_to_dash, options=True)
def merge_json_dicts(
    *,
    input_dicts: Sequence[vh3.JSON],
    ttl: vh3.Integer = 360,
    max_ram: vh3.Integer = 1024,
    max_disk: vh3.Integer = 1024,
    indent: vh3.Integer = None
) -> vh3.JSON:
    """
    Merge json dicts

    :param input_dicts:
      Json dicts to merge
    :param indent: Intent:
      Indent in combined dict:
    """
    raise NotImplementedError("Write your local execution stub here")


@vh3.decorator.external_operation("https://nirvana.yandex-team.ru/operation/8cd120a5-2493-4d53-a0d7-40bda334abe1")
@vh3.decorator.nirvana_names(file="File")
@vh3.decorator.nirvana_output_names("Binary_file")
def convert_any_to_binary_data(
    *,
    file: Union[
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
    ],
    sync: Sequence[
        Union[
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
) -> vh3.Binary:
    """
    Convert any to Binary Data
    """
    raise NotImplementedError("Write your local execution stub here")


@vh3.decorator.external_operation("https://nirvana.yandex-team.ru/operation/8167e323-1b0f-406f-8bf6-c911328b384b")
@vh3.decorator.nirvana_output_names("resource_meta")
def ya_upload_with_attrs_and_desc(
    *,
    resource_type: vh3.String,
    resource_owner: vh3.String,
    resource_filename: vh3.String,
    sandbox_token: vh3.Secret = vh3.Factory(lambda: vh3.context.sandbox_token),
    resource: Union[vh3.Binary, vh3.File, vh3.JSON, vh3.TSV, vh3.Text],
    resource_ttl: vh3.String = "1",
    resource_attributes: vh3.MultipleStrings = (),
    resource_description: vh3.String = None,
    do_not_remove: vh3.Boolean = False
) -> vh3.JSON:
    """
    ya upload with attrs and desc

    ya upload with resource attributes and description

    :param resource_type:
      [[Created resource type ]]
      Created resource type
    :param resource_owner:
      [[User name to own canonical data saved to sandbox]]
      User name to own canonical data saved to sandbox
    :param sandbox_token:
      [[oAuth token for sandbox interaction]]
      oAuth token for sandbox interaction
    :param resource_ttl:
      [[Resource TTL in days (pass 'inf' - to mark resource not removable) (default: 1)]]
      Resource TTL in days (pass 'inf' - to mark resource not removable) (default: 1)
    :param resource_attributes:
      [[attributes for sandbox resource]]
      attributes for sandbox resource
    """
    raise NotImplementedError("Write your local execution stub here")


class Jinja2TemplateOutput(NamedTuple):
    output_text: vh3.Text
    output_json: vh3.JSON


@vh3.decorator.external_operation("https://nirvana.yandex-team.ru/operation/f89deced-aca2-474b-9278-d18ba5a7939b")
@vh3.decorator.nirvana_names(json_option="json_option")
@vh3.decorator.nirvana_names_transformer(vh3.name_transformers.snake_to_dash, options=True)
def jinja2_template(
    *,
    context: vh3.JSON,
    template: vh3.Text,
    ttl: vh3.Integer = 360,
    max_ram: vh3.Integer = 100,
    max_disk: vh3.Integer = 1024,
    json_option: vh3.String = "opt",
    filters: vh3.Text = None
) -> Jinja2TemplateOutput:
    """
    Jinja2 Template

    Template variable: use variable 'context'
    Example {{context['your variable']}}

    Custom Filters:  @template_filter(name)
    Example
    @template_filter(name='split')
    def split_filter(value, char=','):
        return value.split(char)
    """
    raise NotImplementedError("Write your local execution stub here")


@vh3.decorator.external_operation("https://nirvana.yandex-team.ru/operation/df2bf83e-c46a-4279-b76d-0d8102d868ef")
@vh3.decorator.nirvana_names(max_ram="max-ram")
@vh3.decorator.nirvana_output_names("output")
def skip_empty_mr_table(
    *,
    yt_token: vh3.Secret = vh3.Factory(lambda: vh3.context.yt_token),
    input: vh3.MRTable,
    ttl: vh3.Integer = 360,
    max_ram: vh3.Integer = 1024,
    skip_non_existing_table: vh3.Boolean = False,
    cache_timestamp: vh3.Date = None
) -> vh3.OptionalOutput[vh3.MRTable]:
    """
    Skip empty MR table
    Skip YT table if it is empty. Source code: https://a.yandex-team.ru/arc/trunk/arcadia/yweb/antiporno/nirvana/operations/skip_empty_table
    :param yt_token: YT Token:
      [[ID of Nirvana Secret with YT access token (https://nda.ya.ru/3RSzVU).
    Guide to Nirvana Secrets: https://nda.ya.ru/3RSzWZ]]
      YT OAuth Token.
        [Obtain access token](https://nda.ya.ru/3RSzVU), then [create a Nirvana secret](https://nda.ya.ru/3RSzWZ) and [use it here](https://nda.ya.ru/3RSzWb).
        You can [share the secret](https://nda.ya.ru/3RSzWd) with user(s) and/or a staff group.
    :param skip_non_existing_table: Skip non-existing table:
      [[Skip if table doesn't exist, fail otherwise]]
      Skip if table doesn't exist, fail otherwise
    :param cache_timestamp: Timestamp:
      [[Used only for cache invalidation]]
    """
    raise NotImplementedError("Write your local execution stub here")


@vh3.decorator.external_operation("https://nirvana.yandex-team.ru/operation/99850f24-3a3c-4e47-b522-89e9a1198745")
@vh3.decorator.nirvana_names_transformer(vh3.name_transformers.snake_to_camel)
def instantiate_mr_table_reactor_artifact(
    *,
    namespace_id: vh3.String = None,
    namespace_path: vh3.String = None,
    artifact_id: vh3.String = None,
    data: vh3.String = None,
    attributes: vh3.String = None,
    user_timestamp: vh3.String = None,
    ttl_days: vh3.Integer = None,
    input: vh3.MRTable = None
) -> None:
    """
    Instantiate Mr Table [Reactor, Artifact]

    Instantiate Mr Table Artifact

    [Documentation](https://wiki.yandex-team.ru/nirvana/reactor/processor)

    :param namespace_id: Namespace ID
      [[Namespace ID of a required Artifact.]]
      Namespace ID of a required Artifact.

      One of the Identification options must be defined in order to execute the operation:
      - Namespace ID
      - Namespace Path
      - Artifact ID
    :param namespace_path: Namespace Path
      [[Namespace Path of a required Artifact.]]
      Namespace Path of a required Artifact.

      One of the Identification options must be defined in order to execute the operation:
      - Namespace ID
      - Namespace Path
      - Artifact ID
    :param artifact_id: Artifact ID
      [[ID of a required Artifact.]]
      ID of a required Artifact.

      One of the Identification options must be defined in order to execute the operation:
      - Namespace ID
      - Namespace Path
      - Artifact ID
    :param data: Data (JSON)
    :param attributes: Attributes (JSON)
    :param user_timestamp: User Time (ISO, UTC+3)
    :param ttl_days: TTL (days)
    :param input:
      Mr Table data which will be published in a requested Artifact.
    """
    raise NotImplementedError("Write your local execution stub here")


@vh3.decorator.external_operation("https://nirvana.yandex-team.ru/operation/e244e508-6c81-4602-9269-39cda449af61")
@vh3.decorator.nirvana_names(
    import_="import",
    global_="global",
    input0_type="input0_type",
    input1_type="input1_type",
    input2_type="input2_type",
    output_type="output_type",
)
@vh3.decorator.nirvana_output_names("output")
@vh3.decorator.nirvana_names_transformer(vh3.name_transformers.snake_to_dash, options=True)
def python3_any_any_any_to_json(
    *,
    input0_type: vh3.Enum[
        Literal["file", "none", "txt-gen", "txt-mem", "tsv-gen", "tsv-mem", "json-gen", "json-mem", "xml"]
    ],
    input1_type: vh3.Enum[
        Literal["file", "none", "txt-gen", "txt-mem", "tsv-gen", "tsv-mem", "json-gen", "json-mem", "xml"]
    ],
    input2_type: vh3.Enum[
        Literal["file", "none", "txt-gen", "txt-mem", "tsv-gen", "tsv-mem", "json-gen", "json-mem", "xml"]
    ],
    ttl: vh3.Integer = 360,
    max_ram: vh3.Integer = 1024,
    max_disk: vh3.Integer = 16384,
    import_: vh3.String = None,
    global_: vh3.String = None,
    body: vh3.String = "return v",
    output_type: vh3.Enum[Literal["json", "json-utf8", "json-pretty", "txt", "file", "none"]] = "json",
    job_environments: vh3.MultipleStrings = (),
    yt_token: vh3.Secret = vh3.Factory(lambda: vh3.context.yt_token),
    mr_account: vh3.String = vh3.Factory(lambda: vh3.context.mr_account),
    yt_pool: vh3.String = None,
    mr_default_cluster: vh3.Enum[
        Literal[
            "hahn",
            "freud",
            "marx",
            "hume",
            "arnold",
            "markov",
            "bohr",
            "landau",
            "seneca-vla",
            "seneca-sas",
            "seneca-man",
        ]
    ] = "hahn",
    input0: Union[
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
    ] = None,
    input1: Union[
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
    ] = None,
    input2: Union[
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
    Python3 any+any+any to json
    Transform input data using Python3
    Documentation and examples: https://wiki.yandex-team.ru/mlmarines/operations/pythontransform
    =======
    The operation was created by dj/nirvana/nirvana_make tool.
    See https://nda.ya.ru/3UUaov for updating the operation.
    Please, do not update the operation manually!
    Update command:
    dj/nirvana/nirvana_make/operation_make --update data/python_transform/py3_any+any+any_to_json --root yt://hahn/home/mlmarines/common/nirvana_make/operations --import dj/nirvana/operations
    Svn info:
        URL: svn://arcadia.yandex.ru/arc/trunk/arcadia
        Last Changed Rev: 9079395
        Last Changed Author: arumyan
        Last Changed Date: 2022-01-27T15:45:21.642352Z
    Other info:
        Build by: sandbox
        Top src dir: /place/sandbox-data/tasks/5/1/1201657915/__FUSE/mount_point_89de9e5d-1123-471f-b8c4-4b9aab2f3be5
        Top build dir: /place/sandbox-data/tasks/5/1/1201657915/__FUSE/mount_point_89de9e5d-1123-471f-b8c4-4b9aab2f3be5
        Hostname: linux-ubuntu-12-04-precise
        Host information:
            Linux linux-ubuntu-12-04-precise 4.19.183-42.2mofed #1 SMP Wed Apr 21 14:40:19 UTC 2021 x86_64 x86_64 x86_64 GNU/Linux
    :param input0_type: Input0 type
      [[Type of input0]]
      Type of input0
    :param input1_type: Input1 type
      [[Type of input1]]
      Type of input1
    :param input2_type: Input2 type
      [[Type of input2]]
      Type of input2
    :param import_: Import modules
      [[Modules to import, comma separated]]
      Modules to import, comma separated
    :param global_: Global definitions
      [[Global definitions, multiline allowed]]
      Global definitions, multiline allowed
    :param body: Function(v, w, x) body
      [[Body for function]]
      Body for function
    :param output_type: Output type
      [[Type of output]]
      Type of output
    :param yt_token: YT Token:
      [[ID of Nirvana Secret with YT access token (https://nda.ya.ru/3RSzVU).
    Guide to Nirvana Secrets: https://nda.ya.ru/3RSzWZ]]
      YT OAuth Token.
        [Obtain access token](https://nda.ya.ru/3RSzVU), then [create a Nirvana secret](https://nda.ya.ru/3RSzWZ) and [use it here](https://nda.ya.ru/3RSzWb).
        You can [share the secret](https://nda.ya.ru/3RSzWd) with user(s) and/or a staff group.
    :param mr_account: MR Account:
      [[MR Account Name.
    By default, output tables and directories will be created in some subdirectory of home/<MR Account>/<workflow owner>/nirvana]]
      MR account name (e.g. `rank_machine`) used to build MR output path for this operation.
      See the `mr-output-path` option for more information
    :param yt_pool: YT Pool:
      [[Pool used by YT scheduler. Leave blank to use default pool.
    This option has no effect on YaMR.]]
      Pool used by [YT operation scheduler](https://nda.ya.ru/3Rk4af). Leave this blank to use default pool.
    :param mr_default_cluster: Default YT cluster:
      [[Default YT cluster]]
      Default YT cluster
    :param input0:
      Input0
    :param input1:
      Input1
    :param input2:
      Input2
    """
    raise NotImplementedError("Write your local execution stub here")


@vh3.decorator.external_operation("https://nirvana.yandex-team.ru/operation/2417849a-4303-11e7-89a6-0025909427cc")
@vh3.decorator.nirvana_output_names("output")
def single_option_to_text_output(input: vh3.String) -> vh3.Text:
    """
    Single Option To Text Output
    Передает указанный в опции текст на выход в формате Text. https://nda.ya.ru/t/spesTmxv3W4w6X
    """
    raise NotImplementedError("Write your local execution stub here")


class AliceQueriesBinaryClassificationOutput(NamedTuple):
    data_output: vh3.OptionalOutput[vh3.JSON]
    toloka_stats: vh3.OptionalOutput[vh3.JSON]


@vh3.decorator.external_operation("https://nirvana.yandex-team.ru/operation/5a2cecae-7b8b-4337-896f-d1961ccaed97")
@vh3.decorator.nirvana_names(
    dc_process_name="dc-process-name",
    classification_task_question="classification-task-question",
    classification_project_instructions="classification-project-instructions",
    mr_account="mr-account",
)
def alice_queries_binary_classification(
    *,
    dc_process_name: vh3.String,
    classification_project_instructions: vh3.String,
    data_input: vh3.JSON,
    classification_task_question: vh3.String = "COMMON QUESTION",
    tom_questions: vh3.String = None,
    yql_token: vh3.Secret = "robot-alice-ue2e_yql_token",
    mr_account: vh3.String = "alice-ue2e",
    cache_sync: vh3.Integer = 0,
    abc_service_id: vh3.String = "1459"
) -> AliceQueriesBinaryClassificationOutput:
    """
    alice queries binary classification

    Бинарная классификация запросов в Толоке по существующей инструкции в Markdown. Подробнее https://wiki.yandex-team.ru/alice/analytics/newscenariousbasket/binaryclassification/

    :param dc_process_name: Название запуска
    :param classification_project_instructions: Инструкция в разметке Markdown
    :param classification_task_question: Вопрос для бинарной классификации
    :param tom_questions: Вопросы для сбора ТоМ
      [[[опционально] Нужны для генерации negative hp путём коверкания вопросов]]
    :param mr_account: MR Account:
      [[MR Account Name.
    By default, output tables and directories will be created in some subdirectory of home/<MR Account>/<workflow owner>/nirvana]]
      MR account name (e.g. `rank_machine`) used to build MR output path for this operation.

      See the `mr-output-path` option for more information
    """
    raise NotImplementedError("Write your local execution stub here")


@vh3.decorator.external_operation("https://nirvana.yandex-team.ru/operation/5fe73009-86d1-4eaf-8c22-50c1f8b3097f")
@vh3.decorator.nirvana_output_names("new_table")
@vh3.decorator.nirvana_names_transformer(vh3.name_transformers.snake_to_dash, options=True)
def mr_write_json_to_directory_create_new(
    *,
    yt_token: vh3.Secret = vh3.Factory(lambda: vh3.context.yt_token),
    mr_default_cluster: vh3.Enum[
        Literal[
            "hahn",
            "freud",
            "marx",
            "hume",
            "arnold",
            "markov",
            "bohr",
            "landau",
        ]
    ] = vh3.Factory(lambda: vh3.context.mr_default_cluster),
    json: vh3.JSON,
    ttl: vh3.Integer = 360,
    max_ram: vh3.Integer = 1024,
    max_disk: vh3.Integer = 100,
    input_format: vh3.Enum[Literal["AUTO", "ARRAY", "LINES"]] = "AUTO",
    string_mode: vh3.Enum[Literal["TEXT", "BINARY"]] = "TEXT",
    destination_directory: vh3.String = None,
    mr_account: vh3.String = None,
    table_writer: vh3.String = None,
    timestamp: vh3.Date = None,
    mr_output_ttl: vh3.Integer = None
) -> vh3.MRTable:
    """
    MR Write JSON to Directory (Create New)
    Creates a new MR Table from array of JSON objects or `
    `-delimited JSON objects.
    Each JSON object `{ "<key_1>": <value_1>, ..., "<key_N>": <value_N> }` represents a single table row with value `<value_1>` in column `<key_1>`, ..., `<value_N>` in column `<key_N>`.
    Missing values are allowed. *E.g.*, `[ { "k1": "v1", "num": 42 }, { "k1": "v2" } ]` represents a perfectly valid two-row table with missing value for `num` in its second row.
    :param yt_token: YT Token:
      [[ID of Nirvana Secret with YT access token (https://nda.ya.ru/3RSzVU).
    Guide to Nirvana Secrets: https://nda.ya.ru/3RSzWZ]]
      YT OAuth Token.
        [Obtain access token](https://nda.ya.ru/3RSzVU), then [create a Nirvana secret](https://nda.ya.ru/3RSzWZ) and [use it here](https://nda.ya.ru/3RSzWb).
        You can [share the secret](https://nda.ya.ru/3RSzWd) with user(s) and/or a staff group.
    :param mr_default_cluster: Destination Cluster:
    :param json:
      JSON data to write, in one of the following formats:
      * Array of JSON objects (`[ { ... }, ... { ... } ]`)
      * `
      `-delimited JSON objects (`{ ... }
       ...
       { ... }`)
    :param input_format: Input Format:
      [["Auto" works well unless your JSON inputs are unusual (e.g., JSON array preceded by extra whitespace)]]
      Input Data Format.
      * **Auto (recommended)**: if first character in `json` input is `[`, treat input as **Array of JSON Objects**. Otherwise, treat it as **`
      `-delimited JSON Objects**.
      * **Array of JSON Objects**. Input *may* be multi-line, pretty-printed, or both.
      * **`
      `-delimited JSON Objects**. Each object *must* occupy a single line. This is a format output by the `jq` utility in certain scenarios.
    :param string_mode: String Mode:
      [["Text (recommended)" mode treats JSON strings as Unicode text, which is typically what you want.
    "Binary Data" mode treats a restricted subset of JSON strings as binary data.
    See https://nda.ya.ru/3SSRik for more information]]
      In **TEXT** mode (**`encode_utf8=%false`**), JSON strings are treated as sequences of Unicode characters, and stored in UTF-8. Using this mode is **highly recommended**.
      In **BINARY** mode (**`encode_utf8=%true`**), JSON strings are treated as sequences of bytes.
      `\u0000` character maps to byte `0x00`, ..., and `\u00FF` maps to `0xFF`. Characters outside of the `\u0000..\u00FF` range are *not allowed*.
      See [YT `json` Format Documentation](https://nda.ya.ru/3SSRik) for more information.
    :param destination_directory: Destination Directory:
      [[If absent, table will be created under MR Account's home directory.
    If MR Account is not set or is `tmp', table will be created under `tmp'.]]
      Destination directory. If not specified, table will be created under the account's home directory:
        * `home/<account name>/<user name>/nirvana/<execution ID>` if MR account name is specified and is not `tmp`
        * `tmp/<user name>/nirvana/<execution ID>` otherwise
    :param mr_account: MR Account:
      [[MR account name (e.g. `rank_machine'). Overrides MR_USER setting for the write operation]]
      MR account name (e.g. `rank_machine`). Overrides `MR_USER` setting for the write operation
    :param table_writer: Custom Table Writer Spec (JSON):
      [[Custom table_writer specification in **JSON** format]]
      Custom `table_writer` specification in JSON format. E.g., set `table-writer` to
      ```
          {"max_row_weight": 134217728}
      ```
      to get maximum row weight of 128M (the current row weight limit).
    :param timestamp: Timestamp:
      [[Set a recent, not previously used timestamp to force "MR Write" to run even if `tsv' was already written to destination directory]]
      Timestamp to control caching of "MR Write" results.
      Set a recent, not previously used timestamp to force "MR Write" to run even if `tsv` was already written to destination directory
    :param mr_output_ttl: MR Table TTL
      TTL for output table in days. Read more details [here](https://nda.ya.ru/3VruLL)
    """
    raise NotImplementedError("Write your local execution stub here")


@vh3.decorator.external_graph("https://nirvana.yandex-team.ru/process/eeb9d8b0-7adb-4e2d-bf0b-acf55fbfeb24")
@vh3.decorator.nirvana_names(max_unique_utterances="max_unique_utterances")
@vh3.decorator.nirvana_output_names("result")
@vh3.decorator.nirvana_names_transformer(vh3.name_transformers.snake_to_dash, options=True)
def alice_logs_miner(
    *,
    yt_token: vh3.Secret = vh3.Factory(lambda: vh3.context.yt_token),
    yql_token: vh3.Secret = vh3.Factory(lambda: vh3.context.yql_token),
    max_unique_utterances: vh3.Integer,
    data: vh3.MRTable,
    mr_account: vh3.String = vh3.Factory(lambda: vh3.context.mr_account),
    positives: vh3.String = vh3.Factory(lambda: vh3.context.positive_honeypots),
    negatives: vh3.String = vh3.Factory(lambda: vh3.context.negative_honeypots),
) -> vh3.OptionalOutput[vh3.MRTable]:
    """
    alice logs miner

    :param yt_token: YT Token:
      [[ID of Nirvana Secret with YT access token (https://nda.ya.ru/3RSzVU).
    Guide to Nirvana Secrets: https://nda.ya.ru/3RSzWZ]]
    :param yql_token: YQL Token:
      [[YQL OAuth Token, see https://wiki.yandex-team.ru/kikimr/yql/userguide/cli/#autentifikacija]]
    :param max_unique_utterances: Max unique utterances
    :param mr_account: MR Account:
      [[MR Account Name.
    By default, output tables and directories will be created in some subdirectory of home/<MR Account>/<workflow owner>/nirvana]]
    """
    raise NotImplementedError("Write your local execution stub here")


class LightGroovyJsonFilterIfOutput(NamedTuple):
    output_true: vh3.OptionalOutput[vh3.JSON]
    output_false: vh3.OptionalOutput[vh3.JSON]


@vh3.decorator.external_operation("https://nirvana.yandex-team.ru/operation/a91d6ec5-502a-4b46-84ef-e4753c3e9778")
def light_groovy_json_filter_if(*, filter: vh3.String, input: vh3.JSON) -> LightGroovyJsonFilterIfOutput:
    """
    [Light] Groovy Json Filter [If]

    Операция для фильтрации JSON-объектов на основе groovy-выражения. В качестве переменной в нем используется  _ (подчеркивание).
    Выходы являются опциональными: если данные для такого выхода не будут записаны, то зависящая от него ветка
    перейдет в статус Skipped, но граф при этом продолжит выполняться.

    https://wiki.yandex-team.ru/hitman/nirvana/json/#groovyjsonfilterif

    :param filter: Groovy filter
      [[Use "_" variable. For example: "_.x % 2 == 1"]]
      Inline groovy predicate to execude on each json object. Use _ variable.
    :param input:
      Input file containing json array or single json element
    """
    raise NotImplementedError("Write your local execution stub here")


@vh3.decorator.external_operation("https://nirvana.yandex-team.ru/operation/f6f9da25-c6b7-46e9-8ab9-6dec3c02e610")
@vh3.decorator.nirvana_names(
    model_head="model_head",
    column_to_embed="column_to_embed",
    destination_column="destination_column",
    batch_size="batch_size",
    max_input_len="max_input_len",
    yt_pool_tree="yt_pool_tree",
    yt_weight="yt_weight",
    other_flags="other_flags",
)
@vh3.decorator.nirvana_output_names("output")
@vh3.decorator.nirvana_names_transformer(vh3.name_transformers.snake_to_dash, options=True)
def zeliboba_inference_embed(
    *,
    yt_token: vh3.Secret = vh3.Factory(lambda: vh3.context.yt_token),
    mr_account: vh3.String = vh3.Factory(lambda: vh3.context.mr_account),
    input_table: vh3.MRTable,
    model: vh3.Enum[Literal["2.0.2 mix emb256", "2.0.2 ict emb256", "base mix emb256"]] = "2.0.2 mix emb256",
    model_head: vh3.Enum[Literal["doc", "query"]] = "query",
    column_to_embed: vh3.String = "text",
    destination_column: vh3.String = "sentence_embedding",
    batch_size: vh3.Integer = 32,
    max_input_len: vh3.Integer = 40,
    yt_pool_tree: vh3.Enum[Literal["gpu_tesla_v100", "gpu_tesla_a100_80g", "gpu_tesla_a100"]] = "gpu_tesla_v100",
    yt_weight: vh3.Integer = 1,
    other_flags: vh3.String = None,
    max_disk: vh3.Integer = 4000,
    max_ram: vh3.Integer = 2000,
    cpu_guarantee: vh3.Integer = 100,
    ttl: vh3.Integer = 20000,
    yt_pool: vh3.String = None,
    mr_output_policy: vh3.Enum[Literal["CREATE_ALL", "CREATE_DIR", "DO_NOTHING"]] = "CREATE_ALL",
    mr_transaction_policy: vh3.Enum[Literal["MANUAL", "AUTO"]] = "MANUAL",
    mr_default_cluster: vh3.String = None,
    restrict_gpu_type: vh3.Boolean = False,
    finetuned_model: vh3.MRFile = None,
    custom_mapper_binary: vh3.Executable = None
) -> vh3.MRTable:
    """
    zeliboba_inference_embed


    released from https://a.yandex-team.ru/arc/trunk/arcadia/ml/zeliboba/nirvana/operations/autopack?rev=9423122

    :param yt_token:
      YT OAuth Token.
        [Obtain access token](https://nda.ya.ru/3RSzVU), then [create a Nirvana secret](https://nda.ya.ru/3RSzWZ) and [use it here](https://nda.ya.ru/3RSzWb).
        You can [share the secret](https://nda.ya.ru/3RSzWd) with user(s) and/or a staff group.
      ID of Nirvana Secret with YT access token (https://nda.ya.ru/3RSzVU).
      Guide to Nirvana Secrets: https://nda.ya.ru/3RSzWZ
    :param mr_account:
      MR account name (e.g. `rank_machine`) used to build MR output path for this operation.
      See the `mr-output-path` option for more information
      MR Account Name.
      By default, output tables and directories will be created in some subdirectory of home/<MR Account>/<workflow owner>/nirvana
    :param yt_pool: YT Pool:
      Pool used by [YT operation scheduler](https://nda.ya.ru/3Rk4af). Leave this blank to use default pool.
      Pool used by YT scheduler. Leave blank to use default pool.
      This option has no effect on YaMR.
    :param mr_output_policy: MR output policy:
      By default operation creates output objects, you may disable this behavior: https://wiki.yandex-team.ru/nirvana/vodstvo/processory/mrprocessor/#mr-output-policy
    :param mr_transaction_policy: MR transaction policy:
      Nirvana can automatically prepare, ping and finish MR transaction for this operation: https://nda.ya.ru/3UWdsN
    :param mr_default_cluster: mr default cluster
      Default name for MR-cluster. It's mandatory if there is no inputs such as "MR Table", "MR Directory" or "MR File"
    :param restrict_gpu_type:
      If True, the first value from table will be chosen
      gpu_tesla_k40 -- gpu-type = CUDA_3_5;
      gpu_tesla_m40 -- gpu-type = CUDA_5_2
      gpu_geforce_1080ti -- gpu-type = CUDA_6_1 && gpu-max-ram < 11000
      gpu_tesla_p40 -- gpu-type = CUDA_6_1 && gpu-max-ram >= 11000
      gpu_tesla_v100 -- gpu-type = CUDA_7_0
    """
    raise NotImplementedError("Write your local execution stub here")


@vh3.decorator.external_operation("https://nirvana.yandex-team.ru/operation/ec4a8561-87ff-4095-afd5-3ca4f6226ea8")
@vh3.decorator.nirvana_names(creation_mode="creationMode")
@vh3.decorator.nirvana_output_names("mr_file")
@vh3.decorator.nirvana_names_transformer(vh3.name_transformers.snake_to_dash, options=True)
def get_mr_file(
    *,
    cluster: vh3.Enum[
        Literal[
            "hahn",
            "banach",
            "freud",
            "marx",
            "hume",
            "arnold",
            "markov",
            "bohr",
            "landau",
        ]
    ] = vh3.Factory(lambda: vh3.context.mr_default_cluster),
    creation_mode: vh3.Enum[Literal["NO_CHECK", "CHECK_EXISTS"]] = "NO_CHECK",
    path: Union[vh3.String, vh3.Text] = None,
    yt_token: vh3.Secret = vh3.Factory(lambda: vh3.context.yt_token),
    base_path: Union[vh3.MRDirectory, vh3.MRFile, vh3.MRTable] = None
) -> vh3.MRFile:
    """
    Get MR File

    Creates a reference to MR File, either existing or potential.
      * If `path` input is present, its first line will be used as the file's path. If not, `path` option value will be used instead.
      * If `base_path` input is present, file path will be treated as *relative* and resolved against `base_path`. If not, path will be treated as *absolute*.

    :param cluster: Cluster:
      [[MR Cluster this file is on]]
      MR Cluster name, recognized by MR processor and FML processor.
      * If not set, `base_path`'s cluster will be used
      * If both `cluster` option value and `base_path` input are present, cluster name specified in **option** will be used
    :param creation_mode: Creation Mode:
      [[Actions to take when getting the MR File]]
      MR Path creation mode. Specifies additional actions to be taken when getting the path
    :param path: Path:
      [[Path to MR File]]
      Path to MR file. Used when `path` input is absent.
      * If `base_path` input is absent, this is an absolute path.
      * If `base_path` input is present, this is a relative path.
    :param path:
      Text file with MR file path on its first line. If this input is absent, `path` option value will be used instead.
      * If `base_path` input is absent, this is an absolute path.
      * If `base_path` input is present, this is a relative path.
    :param yt_token: YT Token:
      [[(Optional) Token used if Creation Mode is "Check that Path Exists".
    Write the name of Nirvana Secret holding your YT Access Token here.]]
      *(Optional)* YT OAuth Token to use in "Check that Path Exists" Creation Mode. If not specified, MR Processor's token will be used.

      [Obtain access token](https://nda.ya.ru/3RSzVU), then [create a Nirvana secret](https://nda.ya.ru/3RSzWZ) and [use it here](https://nda.ya.ru/3RSzWb).
      You can [share the secret](https://nda.ya.ru/3RSzWd) with user(s) and/or a staff group.
    :param base_path:
      Base path to resolve against.

      If absent, file path is considered absolute.
    """
    raise NotImplementedError("Write your local execution stub here")


@vh3.decorator.external_operation("https://nirvana.yandex-team.ru/operation/d64b6a48-a251-4d7f-a84d-9cddce1f1b97")
@vh3.decorator.nirvana_output_names("path")
def mr_optional_table_identity_transform(
    *,
    timestamp: vh3.String = None,
    ts_for_cache: vh3.Integer = None,
    date_ts: vh3.Date = None,
    path: vh3.MRTable = None,
    sync: Sequence[
        Union[
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
) -> vh3.OptionalOutput[vh3.MRTable]:
    """
    MR Optional Table Identity Transform

    Get Identity Transform for your optional MR Table to cancel caching

    :param sync:
      sync input
    """
    raise NotImplementedError("Write your local execution stub here")


class ScrapeTomGraphOutput(NamedTuple):
    positives: vh3.Text
    negatives: vh3.Text
    options: vh3.JSON


@vh3.decorator.external_graph("https://nirvana.yandex-team.ru/process/be216041-af6f-43cb-b169-feb82c6e86aa")
def scrape_tom_graph(*, token: vh3.Secret = vh3.Factory(lambda: vh3.context.nirvana_token), graph_url: vh3.String = "") -> ScrapeTomGraphOutput:
    """
    scrape tom graph

    :param token: OAuth token
      [[Nirvana API autorisation token]]
    """
    raise NotImplementedError("Write your local execution stub here")


@vh3.decorator.external_operation("https://nirvana.yandex-team.ru/operation/ec426d79-8c00-4c2a-984f-1d963803f27d")
@vh3.decorator.nirvana_names(import_="import", global_="global", input_type="input_type", output_type="output_type")
@vh3.decorator.nirvana_output_names("output")
@vh3.decorator.nirvana_names_transformer(vh3.name_transformers.snake_to_dash, options=True)
def python3_any_to_json(
    *,
    input_type: vh3.Enum[
        Literal["file", "none", "txt-gen", "txt-mem", "tsv-gen", "tsv-mem", "json-gen", "json-mem", "xml"]
    ],
    ttl: vh3.Integer = 360,
    max_ram: vh3.Integer = 1024,
    max_disk: vh3.Integer = 16384,
    import_: vh3.String = None,
    global_: vh3.String = None,
    body: vh3.String = "return v",
    output_type: vh3.Enum[Literal["json", "json-utf8", "json-pretty", "txt", "file", "none"]] = "json",
    job_environments: vh3.MultipleStrings = (),
    yt_token: vh3.Secret = None,
    mr_account: vh3.String = None,
    yt_pool: vh3.String = None,
    mr_default_cluster: vh3.Enum[
        Literal[
            "hahn",
            "freud",
            "marx",
            "hume",
            "arnold",
            "markov",
            "bohr",
            "landau",
            "seneca-vla",
            "seneca-sas",
            "seneca-man",
        ]
    ] = "hahn",
    input: Union[
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

        Last Changed Rev: 9079395

        Last Changed Author: arumyan

        Last Changed Date: 2022-01-27T15:45:21.642352Z


    Other info:

        Build by: sandbox

        Top src dir: /place/sandbox-data/tasks/5/1/1201657915/__FUSE/mount_point_89de9e5d-1123-471f-b8c4-4b9aab2f3be5

        Top build dir: /place/sandbox-data/tasks/5/1/1201657915/__FUSE/mount_point_89de9e5d-1123-471f-b8c4-4b9aab2f3be5

        Hostname: linux-ubuntu-12-04-precise

        Host information:

            Linux linux-ubuntu-12-04-precise 4.19.183-42.2mofed #1 SMP Wed Apr 21 14:40:19 UTC 2021 x86_64 x86_64 x86_64 GNU/Linux



    :param input_type: Input type
      [[Type of input]]
      Type of input
    :param import_: Import modules
      [[Modules to import, comma separated]]
      Modules to import, comma separated
    :param global_: Global definitions
      [[Global definitions, multiline allowed]]
      Global definitions, multiline allowed
    :param body: Function(v) body
      [[Body for function]]
      Body for function
    :param output_type: Output type
      [[Type of output]]
      Type of output
    :param yt_token: YT Token:
      [[ID of Nirvana Secret with YT access token (https://nda.ya.ru/3RSzVU).
    Guide to Nirvana Secrets: https://nda.ya.ru/3RSzWZ]]
      YT OAuth Token.

        [Obtain access token](https://nda.ya.ru/3RSzVU), then [create a Nirvana secret](https://nda.ya.ru/3RSzWZ) and [use it here](https://nda.ya.ru/3RSzWb).
        You can [share the secret](https://nda.ya.ru/3RSzWd) with user(s) and/or a staff group.
    :param mr_account: MR Account:
      [[MR Account Name.
    By default, output tables and directories will be created in some subdirectory of home/<MR Account>/<workflow owner>/nirvana]]
      MR account name (e.g. `rank_machine`) used to build MR output path for this operation.

      See the `mr-output-path` option for more information
    :param yt_pool: YT Pool:
      [[Pool used by YT scheduler. Leave blank to use default pool.
    This option has no effect on YaMR.]]
      Pool used by [YT operation scheduler](https://nda.ya.ru/3Rk4af). Leave this blank to use default pool.
    :param mr_default_cluster: Default YT cluster:
      [[Default YT cluster]]
      Default YT cluster
    :param input:
      Input
    """
    raise NotImplementedError("Write your local execution stub here")


@vh3.decorator.external_operation("https://nirvana.yandex-team.ru/operation/7574e736-e85b-4501-8316-aa4a51da4643")
@vh3.decorator.nirvana_names(import_="import", global_="global", input_type="input_type", output_type="output_type")
@vh3.decorator.nirvana_output_names("output")
@vh3.decorator.nirvana_names_transformer(vh3.name_transformers.snake_to_dash, options=True)
def python3_any_to_txt(
    *,
    input_type: vh3.Enum[
        Literal["file", "none", "txt-gen", "txt-mem", "tsv-gen", "tsv-mem", "json-gen", "json-mem", "xml"]
    ],
    ttl: vh3.Integer = 360,
    max_ram: vh3.Integer = 1024,
    max_disk: vh3.Integer = 16384,
    import_: vh3.String = None,
    global_: vh3.String = None,
    body: vh3.String = "return v",
    output_type: vh3.Enum[Literal["txt", "file", "none"]] = "txt",
    job_environments: vh3.MultipleStrings = (),
    yt_token: vh3.Secret = None,
    mr_account: vh3.String = None,
    yt_pool: vh3.String = None,
    mr_default_cluster: vh3.Enum[
        Literal[
            "hahn",
            "freud",
            "marx",
            "hume",
            "arnold",
            "markov",
            "bohr",
            "landau",
            "seneca-vla",
            "seneca-sas",
            "seneca-man",
        ]
    ] = "hahn",
    input: Union[
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
) -> vh3.Text:
    """
    Python3 any to txt

    Transform input data using Python3

    Documentation and examples: https://wiki.yandex-team.ru/mlmarines/operations/pythontransform

    =======

    The operation was created by dj/nirvana/nirvana_make tool.

    See https://nda.ya.ru/3UUaov for updating the operation.

    Please, do not update the operation manually!


    Update command:

    dj/nirvana/nirvana_make/operation_make --update data/python_transform/py3_any_to_txt
      --root yt://hahn/home/mlmarines/common/nirvana_make/operations --import dj/nirvana/operations


    Svn info:

        URL: svn://arcadia.yandex.ru/arc/trunk/arcadia

        Last Changed Rev: 9079395

        Last Changed Author: arumyan

        Last Changed Date: 2022-01-27T15:45:21.642352Z


    Other info:

        Build by: sandbox

        Top src dir: /place/sandbox-data/tasks/5/1/1201657915/__FUSE/mount_point_89de9e5d-1123-471f-b8c4-4b9aab2f3be5

        Top build dir: /place/sandbox-data/tasks/5/1/1201657915/__FUSE/mount_point_89de9e5d-1123-471f-b8c4-4b9aab2f3be5

        Hostname: linux-ubuntu-12-04-precise

        Host information:

            Linux linux-ubuntu-12-04-precise 4.19.183-42.2mofed #1 SMP Wed Apr 21 14:40:19 UTC 2021 x86_64 x86_64 x86_64 GNU/Linux



    :param input_type: Input type
      [[Type of input]]
      Type of input
    :param import_: Import modules
      [[Modules to import, comma separated]]
      Modules to import, comma separated
    :param global_: Global definitions
      [[Global definitions, multiline allowed]]
      Global definitions, multiline allowed
    :param body: Function(v) body
      [[Body for function]]
      Body for function
    :param output_type: Output type
      [[Type of output]]
      Type of output
    :param yt_token: YT Token:
      [[ID of Nirvana Secret with YT access token (https://nda.ya.ru/3RSzVU).
    Guide to Nirvana Secrets: https://nda.ya.ru/3RSzWZ]]
      YT OAuth Token.

        [Obtain access token](https://nda.ya.ru/3RSzVU), then [create a Nirvana secret](https://nda.ya.ru/3RSzWZ) and [use it here](https://nda.ya.ru/3RSzWb).
        You can [share the secret](https://nda.ya.ru/3RSzWd) with user(s) and/or a staff group.
    :param mr_account: MR Account:
      [[MR Account Name.
    By default, output tables and directories will be created in some subdirectory of home/<MR Account>/<workflow owner>/nirvana]]
      MR account name (e.g. `rank_machine`) used to build MR output path for this operation.

      See the `mr-output-path` option for more information
    :param yt_pool: YT Pool:
      [[Pool used by YT scheduler. Leave blank to use default pool.
    This option has no effect on YaMR.]]
      Pool used by [YT operation scheduler](https://nda.ya.ru/3Rk4af). Leave this blank to use default pool.
    :param mr_default_cluster: Default YT cluster:
      [[Default YT cluster]]
      Default YT cluster
    :param input:
      Input
    """
    raise NotImplementedError("Write your local execution stub here")


@vh3.decorator.external_operation("https://nirvana.yandex-team.ru/operation/fcf99a2d-500b-407f-8075-f94d5e54a50e")
@vh3.decorator.nirvana_output_names("json")
@vh3.decorator.nirvana_names_transformer(vh3.name_transformers.snake_to_dash, options=True)
def mr_read_json(
    *,
    yt_token: vh3.Secret = vh3.Factory(lambda: vh3.context.yt_token),
    table: vh3.MRTable,
    ttl: vh3.Integer = 360,
    max_ram: vh3.Integer = 1024,
    max_disk: vh3.Integer = 1024,
    start_row: vh3.Integer = None,
    end_row: vh3.Integer = None,
    output_columns: vh3.MultipleStrings = (),
    output_format: vh3.Enum[Literal["ARRAY", "LINES"]] = "ARRAY",
    string_mode: vh3.Enum[Literal["TEXT", "BINARY"]] = "TEXT",
    timestamp: vh3.Date = vh3.Factory(lambda: vh3.context.timestamp),
    sync: Sequence[
        Union[
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
) -> vh3.JSON:
    """
    MR Read JSON

    Reads an MR table (or a part of it) as JSON (either an array of JSON objects, or `
    `-delimited JSON objects)

    :param yt_token: YT Token:
      [[ID of Nirvana Secret with YT access token (https://nda.ya.ru/3RSzVU).
    Guide to Nirvana Secrets: https://nda.ya.ru/3RSzWZ]]
      YT OAuth Token.

        [Obtain access token](https://nda.ya.ru/3RSzVU), then [create a Nirvana secret](https://nda.ya.ru/3RSzWZ) and [use it here](https://nda.ya.ru/3RSzWb).
        You can [share the secret](https://nda.ya.ru/3RSzWd) with user(s) and/or a staff group.
    :param table:
      MR Table to read data from
    :param start_row: Start Row Index:
      [[Inclusive, counting from 0]]
      Start row index, **inclusive**, counting from `0`. If absent, table will be read from the beginning
    :param end_row: End Row Index:
      [[Exclusive, counting from 0]]
      End row index, **exclusive**, counting from `0`. If absent, table will be read to the end
    :param output_columns: Output Columns:
      [[Columns to output. If not specified, **all** table's columns will be used]]
      Columns to output. If not specified, **all** table's columns will be used
    :param output_format: Output Format:
      [["Array of JSON Objects" is used by robot-hitman's "YT Get".
    "
    -delimited JSON Objects" is more convenient to process using Unix text utilities (sed, awk, grep, jq etc.)]]
      Output Data Format.
      * **Array of JSON Objects** (`[ { ... }, ..., { ... } ]`)
      * **`
      `-delimited JSON Objects** (`{ ... }
       ...
       { ... }`)
    :param string_mode: String Mode:
      [["Text (recommended)" mode treats string column values as UTF-8 encoded text.

    "Binary Data" mode treats a string column values as raw binary data and converts them to JSON strings by mapping 0x00..0xFF byte values to \u0000..\u00FF characters.

    See https://nda.ya.ru/3SSRik for more information]]
      In **TEXT** mode (**`encode_utf8=%false`**), string column values are treated UTF-8 encoded text. Using this mode is **highly recommended**.

      In **BINARY** mode (**`encode_utf8=%true`**), string column values are treated as byte sequences and
      converted to JSON strings as follows: byte `0x00` maps to `\u0000`, ..., and `0xFF` maps to `\u00FF`.

      See [YT `json` Format Documentation](https://nda.ya.ru/3SSRik) for more information.
    :param timestamp: Timestamp:
      [[Set a recent, not previously used timestamp to force "MR Read" to run even if `table' was already read once]]
      Timestamp to control caching of "MR Read" results.

      Set a recent, not previously used timestamp to force "MR Read" to run even if `table` was already read once
    :param sync:
      Synchronization input. Can be used *e.g.* to read the table only after specific data is ready
    """
    raise NotImplementedError("Write your local execution stub here")


@vh3.decorator.external_operation("https://nirvana.yandex-team.ru/operation/3c2bc130-8f5f-4508-a688-23c8c809e73c")
@vh3.decorator.nirvana_names(model_name="model_name", pulsar_token="pulsar_token")
@vh3.decorator.nirvana_output_names("output")
@vh3.decorator.nirvana_names_transformer(vh3.name_transformers.snake_to_dash, options=True)
def dataset_storing(
    *,
    model_name: vh3.String = vh3.Factory(lambda: vh3.context.model_name),
    pulsar_token: vh3.Secret = 'robot-beggins_pulsar-token',
    yt_token: vh3.Secret = 'robot-beggins_yt-token',
    train: vh3.MRTable,
    accept: vh3.MRTable,
    mr_account: vh3.String = None,
    yt_pool: vh3.String = None,
    ttl: vh3.Integer = 360,
    max_ram: vh3.Integer = 1024,
    mr_output_policy: vh3.Enum[Literal["CREATE_ALL", "CREATE_DIR", "DO_NOTHING"]] = "DO_NOTHING",
    mr_transaction_policy: vh3.Enum[Literal["MANUAL", "AUTO"]] = "MANUAL",
    mr_default_cluster: vh3.String = 'hahn',
    restrict_gpu_type: vh3.Boolean = False
) -> vh3.File:
    """
    dataset_storing


    released from https://a.yandex-team.ru/arc/trunk/arcadia/alice/beggins/internal/vh/cmd/process_manifest?rev=9545334

    :param yt_token: YT Token:
      YT OAuth Token.
        [Obtain access token](https://nda.ya.ru/3RSzVU), then [create a Nirvana secret](https://nda.ya.ru/3RSzWZ) and [use it here](https://nda.ya.ru/3RSzWb).
        You can [share the secret](https://nda.ya.ru/3RSzWd) with user(s) and/or a staff group.
      ID of Nirvana Secret with YT access token (https://nda.ya.ru/3RSzVU).
      Guide to Nirvana Secrets: https://nda.ya.ru/3RSzWZ
    :param mr_account: MR Account:
      MR account name (e.g. `rank_machine`) used to build MR output path for this operation.
      See the `mr-output-path` option for more information
      MR Account Name.
      By default, output tables and directories will be created in some subdirectory of home/<MR Account>/<workflow owner>/nirvana
    :param yt_pool: YT Pool:
      Pool used by [YT operation scheduler](https://nda.ya.ru/3Rk4af). Leave this blank to use default pool.
      Pool used by YT scheduler. Leave blank to use default pool.
      This option has no effect on YaMR.
    :param mr_output_policy: MR output policy:
      By default operation creates output objects, you may disable this behavior: https://wiki.yandex-team.ru/nirvana/vodstvo/processory/mrprocessor/#mr-output-policy
    :param mr_transaction_policy: MR transaction policy:
      Nirvana can automatically prepare, ping and finish MR transaction for this operation: https://nda.ya.ru/3UWdsN
    :param mr_default_cluster: mr default cluster
      Default name for MR-cluster. It's mandatory if there is no inputs such as "MR Table", "MR Directory" or "MR File"
    :param restrict_gpu_type:
      If True, the first value from table will be chosen
      gpu_tesla_k40 -- gpu-type = CUDA_3_5;
      gpu_tesla_m40 -- gpu-type = CUDA_5_2
      gpu_geforce_1080ti -- gpu-type = CUDA_6_1 && gpu-max-ram < 11000
      gpu_tesla_p40 -- gpu-type = CUDA_6_1 && gpu-max-ram >= 11000
      gpu_tesla_v100 -- gpu-type = CUDA_7_0
    """
    raise NotImplementedError("Write your local execution stub here")


class ProcessManifestOutput(NamedTuple):
    train: vh3.MRTable
    accept: vh3.MRTable


@vh3.decorator.external_operation("https://nirvana.yandex-team.ru/operation/6ef405b5-6520-47e4-a60e-06e47debe1b0")
@vh3.decorator.nirvana_names(model_name="model_name", pulsar_token="pulsar_token")
@vh3.decorator.nirvana_names_transformer(vh3.name_transformers.snake_to_dash, options=True)
def process_manifest(
    *,
    model_name: vh3.String = vh3.Factory(lambda: vh3.context.model_name),
    pulsar_token: vh3.Secret = 'robot-beggins_pulsar-token',
    yt_token: vh3.Secret = vh3.Factory(lambda: vh3.context.yt_token),
    mr_account: vh3.String = vh3.Factory(lambda: vh3.context.mr_account),
    yt_pool: vh3.String = None,
    ttl: vh3.Integer = 360,
    max_ram: vh3.Integer = 1024,
    mr_output_policy: vh3.Enum[Literal["CREATE_ALL", "CREATE_DIR", "DO_NOTHING"]] = "CREATE_ALL",
    mr_transaction_policy: vh3.Enum[Literal["MANUAL", "AUTO"]] = "MANUAL",
    mr_default_cluster: vh3.String = vh3.Factory(lambda: vh3.context.mr_default_cluster_string),
    restrict_gpu_type: vh3.Boolean = False,
    manifest: Union[vh3.Binary, vh3.File, vh3.JSON] = None,
    dev: Sequence[vh3.MRTable] = (),
    accept: Sequence[vh3.MRTable] = ()
) -> ProcessManifestOutput:
    """
    process_manifest


    released from https://a.yandex-team.ru/arc/trunk/arcadia/alice/beggins/internal/vh/cmd/process_manifest?rev=9545334

    :param yt_token: YT Token:
      YT OAuth Token.
        [Obtain access token](https://nda.ya.ru/3RSzVU), then [create a Nirvana secret](https://nda.ya.ru/3RSzWZ) and [use it here](https://nda.ya.ru/3RSzWb).
        You can [share the secret](https://nda.ya.ru/3RSzWd) with user(s) and/or a staff group.
      ID of Nirvana Secret with YT access token (https://nda.ya.ru/3RSzVU).
      Guide to Nirvana Secrets: https://nda.ya.ru/3RSzWZ
    :param mr_account: MR Account:
      MR account name (e.g. `rank_machine`) used to build MR output path for this operation.
      See the `mr-output-path` option for more information
      MR Account Name.
      By default, output tables and directories will be created in some subdirectory of home/<MR Account>/<workflow owner>/nirvana
    :param yt_pool: YT Pool:
      Pool used by [YT operation scheduler](https://nda.ya.ru/3Rk4af). Leave this blank to use default pool.
      Pool used by YT scheduler. Leave blank to use default pool.
      This option has no effect on YaMR.
    :param mr_output_policy: MR output policy:
      By default operation creates output objects, you may disable this behavior: https://wiki.yandex-team.ru/nirvana/vodstvo/processory/mrprocessor/#mr-output-policy
    :param mr_transaction_policy: MR transaction policy:
      Nirvana can automatically prepare, ping and finish MR transaction for this operation: https://nda.ya.ru/3UWdsN
    :param mr_default_cluster: mr default cluster
      Default name for MR-cluster. It's mandatory if there is no inputs such as "MR Table", "MR Directory" or "MR File"
    :param restrict_gpu_type:
      If True, the first value from table will be chosen
      gpu_tesla_k40 -- gpu-type = CUDA_3_5;
      gpu_tesla_m40 -- gpu-type = CUDA_5_2
      gpu_geforce_1080ti -- gpu-type = CUDA_6_1 && gpu-max-ram < 11000
      gpu_tesla_p40 -- gpu-type = CUDA_6_1 && gpu-max-ram >= 11000
      gpu_tesla_v100 -- gpu-type = CUDA_7_0
    """
    raise NotImplementedError("Write your local execution stub here")


@vh3.decorator.external_operation("https://nirvana.yandex-team.ru/operation/3aae483d-baa2-4ee4-8eb8-ec5fb035f3eb")
@vh3.decorator.nirvana_output_names("text")
@vh3.decorator.nirvana_names_transformer(vh3.name_transformers.snake_to_dash, options=True)
def html_to_text(*, html: vh3.HTML, ttl: vh3.Integer = 360, max_ram: vh3.Integer = 100) -> vh3.Text:
    """
    Html to text
    """
    raise NotImplementedError("Write your local execution stub here")


@vh3.decorator.external_operation("https://nirvana.yandex-team.ru/operation/74a8ca8e-a11d-4dae-ac60-6745e0b66ea3")
@vh3.decorator.nirvana_names(max_ram="max-ram")
def star_trek_comment_with_attachments(
    *,
    issue_key: vh3.String,
    message: vh3.String,
    st_token: vh3.Secret = 'robot-beggins_tracker-token',
    user_agent: vh3.String = 'robot-beggins',
    arcadia_revision: vh3.Integer = 9526045,
    summonees: vh3.MultipleStrings = (),
    max_ram: vh3.Integer = 1024,
    sandbox_oauth_token: vh3.Secret = None,
    attachments: Sequence[vh3.Binary] = ()
) -> None:
    """
    StarTrek comment (with attachments)

    :param arcadia_revision: Arcadia Revision
    :param sandbox_oauth_token: Sandbox OAuth token
      [[To run task on behalf of specific user]]
      Name of the secret containing oauth token of user the sandbox task should be launched from
    :param attachments:
      Data blocks to be attached to the comment as files. Use named links to pass filenames.
    """
    raise NotImplementedError("Write your local execution stub here")


class SvnCheckoutOutput(NamedTuple):
    archive: vh3.Binary
    revision: vh3.Text


@vh3.decorator.external_operation("https://nirvana.yandex-team.ru/operation/1ac20af1-f9bd-40ad-8781-5d77d96dbc93")
@vh3.decorator.nirvana_names(ignore_externals="ignore-externals")
def svn_checkout(
    *,
    arcadia_path: Union[vh3.String, vh3.Text] = None,
    revision: Union[vh3.Integer, vh3.Text] = None,
    depth: vh3.Enum[Literal["infinity", "immediates", "files", "empty"]] = "infinity",
    ignore_externals: vh3.Boolean = False,
    path_prefix: vh3.String = None,
    batch_input: vh3.JSON = None
) -> SvnCheckoutOutput:
    """
    SVN: Checkout

    **Назначение операции**

    Загружает из SVN  содержимое директории по указанному пути и ревизии.
    Для массовой загрузки из разных источников используйте batch-режим, для этого на вход "batch_input" подайте JSON в специальном формате.

    **Описание входов**

    Способы передачи номера ревизии:
     - через вход "revision";
     - через опцию "Revision".

    Способы передачи пути:
      - через вход "arcadia_path";

      - через опцию "Arcadia path”.

    Если одновременно указаны опция и вход, будет использовано значение, взятое из входа.

    **Описание выходов**

    Выход "archive" содержит архив с содержимым запрошенной директории.

    Дополнительный выход "revision" содержит ревизию, из которой был сделан checkout (выход полезен, если не указана опция "Revision" и используется HEAD).


    **Ограничения**

     В batch-режиме игнорируются все опции, кроме ”Depth" и "Ignore Externals”.

    :param arcadia_path: Arcadia path
      [[Относительный путь в аркадии для checkout'а. (arcadia/my/project/path)]]
      Относительный путь в аркадии для checkout'а. (arcadia/my/project/path)
    :param arcadia_path:
      Относительный путь в аркадии для checkout'а. Если не указано - берется из options.
    :param revision: Revision
      Ревизия checkout'а.
    :param revision:
      Ревизия checkout'а. Если не указано - берется из options.
    :param depth: Depth:
      [[Limit operation by depth ARG ('empty', 'files', 'immediates', or 'infinity')]]
    :param ignore_externals: Ignore Externals:
      [[Ignore externals definitions]]
    :param path_prefix: Path prefix
      Path prefix to support branches and tags(i.e. https://arcadia.yandex.ru/arc/branches/remorph/stable-1/arcadia/), set to null if trunk
    :param batch_input:
      Поддержка batch-режима. Пример json-a: https://nirvana.yandex-team.ru/data/0547adaa-bfee-4961-9ba8-09393ca51b7d. Опции кубика игнорируются, за исключением "Depth" и "Ignore Externals"
    """
    raise NotImplementedError("Write your local execution stub here")


@vh3.decorator.external_operation("https://nirvana.yandex-team.ru/operation/7c9d8ea7-eafd-44e8-8cad-e761c6b459a9")
@vh3.decorator.nirvana_names(
    import_="import",
    global_="global",
    input0_type="input0_type",
    input1_type="input1_type",
    input2_type="input2_type",
    output_type="output_type",
)
@vh3.decorator.nirvana_output_names("output")
@vh3.decorator.nirvana_names_transformer(vh3.name_transformers.snake_to_dash, options=True)
def python3_any_any_any_to_binary(
    *,
    input0_type: vh3.Enum[
        Literal["file", "none", "txt-gen", "txt-mem", "tsv-gen", "tsv-mem", "json-gen", "json-mem", "xml"]
    ],
    input1_type: vh3.Enum[
        Literal["file", "none", "txt-gen", "txt-mem", "tsv-gen", "tsv-mem", "json-gen", "json-mem", "xml"]
    ],
    input2_type: vh3.Enum[
        Literal["file", "none", "txt-gen", "txt-mem", "tsv-gen", "tsv-mem", "json-gen", "json-mem", "xml"]
    ],
    ttl: vh3.Integer = 360,
    max_ram: vh3.Integer = 1024,
    max_disk: vh3.Integer = 16384,
    import_: vh3.String = None,
    global_: vh3.String = None,
    body: vh3.String = "return v",
    output_type: vh3.Enum[Literal["file", "none"]] = "file",
    job_environments: vh3.MultipleStrings = (),
    yt_token: vh3.Secret = None,
    mr_account: vh3.String = None,
    yt_pool: vh3.String = None,
    mr_default_cluster: vh3.Enum[
        Literal[
            "hahn",
            "freud",
            "marx",
            "hume",
            "arnold",
            "markov",
            "bohr",
            "landau",
            "seneca-vla",
            "seneca-sas",
            "seneca-man",
        ]
    ] = "hahn",
    input0: Union[
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
    ] = None,
    input1: Union[
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
    ] = None,
    input2: Union[
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
) -> vh3.Binary:
    """
    Python3 any+any+any to binary

    Transform input data using Python3

    Documentation and examples: https://wiki.yandex-team.ru/mlmarines/operations/pythontransform

    =======

    The operation was created by dj/nirvana/nirvana_make tool.

    See https://nda.ya.ru/3UUaov for updating the operation.

    Please, do not update the operation manually!


    Update command:

    dj/nirvana/nirvana_make/operation_make --update data/python_transform/py3_any+any+any_to_binary --root yt://hahn/home/mlmarines/common/nirvana_make/operations --import dj/nirvana/operations


    Svn info:

        URL: svn://arcadia.yandex.ru/arc/trunk/arcadia

        Last Changed Rev: 9079395

        Last Changed Author: arumyan

        Last Changed Date: 2022-01-27T15:45:21.642352Z


    Other info:

        Build by: sandbox

        Top src dir: /place/sandbox-data/tasks/5/1/1201657915/__FUSE/mount_point_89de9e5d-1123-471f-b8c4-4b9aab2f3be5

        Top build dir: /place/sandbox-data/tasks/5/1/1201657915/__FUSE/mount_point_89de9e5d-1123-471f-b8c4-4b9aab2f3be5

        Hostname: linux-ubuntu-12-04-precise

        Host information:

            Linux linux-ubuntu-12-04-precise 4.19.183-42.2mofed #1 SMP Wed Apr 21 14:40:19 UTC 2021 x86_64 x86_64 x86_64 GNU/Linux



    :param input0_type: Input0 type
      [[Type of input0]]
      Type of input0
    :param input1_type: Input1 type
      [[Type of input1]]
      Type of input1
    :param input2_type: Input2 type
      [[Type of input2]]
      Type of input2
    :param import_: Import modules
      [[Modules to import, comma separated]]
      Modules to import, comma separated
    :param global_: Global definitions
      [[Global definitions, multiline allowed]]
      Global definitions, multiline allowed
    :param body: Function(v, w, x) body
      [[Body for function]]
      Body for function
    :param output_type: Output type
      [[Type of output]]
      Type of output
    :param yt_token: YT Token:
      [[ID of Nirvana Secret with YT access token (https://nda.ya.ru/3RSzVU).
    Guide to Nirvana Secrets: https://nda.ya.ru/3RSzWZ]]
      YT OAuth Token.

        [Obtain access token](https://nda.ya.ru/3RSzVU), then [create a Nirvana secret](https://nda.ya.ru/3RSzWZ) and [use it here](https://nda.ya.ru/3RSzWb).
        You can [share the secret](https://nda.ya.ru/3RSzWd) with user(s) and/or a staff group.
    :param mr_account: MR Account:
      [[MR Account Name.
    By default, output tables and directories will be created in some subdirectory of home/<MR Account>/<workflow owner>/nirvana]]
      MR account name (e.g. `rank_machine`) used to build MR output path for this operation.

      See the `mr-output-path` option for more information
    :param yt_pool: YT Pool:
      [[Pool used by YT scheduler. Leave blank to use default pool.
    This option has no effect on YaMR.]]
      Pool used by [YT operation scheduler](https://nda.ya.ru/3Rk4af). Leave this blank to use default pool.
    :param mr_default_cluster: Default YT cluster:
      [[Default YT cluster]]
      Default YT cluster
    :param input0:
      Input0
    :param input1:
      Input1
    :param input2:
      Input2
    """
    raise NotImplementedError("Write your local execution stub here")


class SvnCommitOutput(NamedTuple):
    commited_revision: vh3.Text
    review_link: vh3.OptionalOutput[vh3.Text]


@vh3.decorator.external_operation("https://nirvana.yandex-team.ru/operation/2b0165c8-5c92-4940-888f-93940cb94ad4")
@vh3.decorator.nirvana_names(commit_message="commit_message")
@vh3.decorator.nirvana_names_transformer(vh3.name_transformers.snake_to_dash, options=True)
def svn_commit(
    *,
    working_dir: vh3.Binary,
    commit_message: Union[vh3.String, vh3.Text] = None,
    ignore_externals: vh3.Boolean = False,
    depth: vh3.Enum[Literal["infinity", "immediates", "files", "empty"]] = "infinity",
    add_new_files: vh3.Boolean = True,
    publish_review: vh3.Boolean = False,
    automerge_review: vh3.Boolean = False,
    ssh_private_key: vh3.Secret = None,
    ssh_user: vh3.String = None
) -> SvnCommitOutput:
    """
    SVN: Commit

    **Назначение операции**

    Записывает изменения в SVN-репозиторий.

    **Описание входов**

     - working_dir - архив tar.gz с изменениями в рабочем каталоге.
     - commit_message - список изменений.


    **Описание выходов**

     - commited_revision - номер ревизии. Если получено значение "-1" (сообщение об ошибке), проверьте операцию еще раз.

    **Ограничения**

    Не предусмотрены.

    **Использование ssh ключа**

    По умолчанию коммит делается из-под robot-processorsdk. Чтобы коммитить из-под другого пользователя, нужно передать опцию ssh-private-key  и ssh-user.
    Однако, если вы хотите коммитеть из-под другого робота, ему НЕОБХОДИМО ВЫДАТЬ роль Арканум/Developer в IDM.

    :param working_dir:
      tar.gz archive with working dir with changes
    :param commit_message: Commit Message
      [[Commit message]]
      Commit desctiption message
    :param commit_message:
      Commit message
    :param ignore_externals: \nIgnore Externals
      [[Ignore externals definitions]]
    :param depth: Depth:
      [[Limit operation by depth ARG ('empty', 'files', 'immediates', or 'infinity')]]
    :param add_new_files: Add new files
    :param publish_review: Publish review
    :param automerge_review: Automerge review
    :param ssh_user:
      Used if "ssh-private-key" option provided. Default - workflow owner
    """
    raise NotImplementedError("Write your local execution stub here")


@vh3.decorator.external_operation("https://nirvana.yandex-team.ru/operation/59fe50c4-f293-4c2b-acf1-ff0c703d3f50")
@vh3.decorator.nirvana_names(max_disk="max-disk")
@vh3.decorator.nirvana_output_names("resource")
def get_sandbox_resource(
    *, resource_id: vh3.String, ttl: vh3.Integer = 360, max_disk: vh3.Integer = 2048, do_pack: vh3.Boolean = False
) -> vh3.Binary:
    """
    Get sandbox resource

    Download resource from sandbox

    :param resource_id: Resource id
      Sandbox resource id to download
    :param do_pack: Do pack
      [[Упаковать скачанные ресурсы в один файл, актуально для директорий]]
      Упаковать скачанные ресурсы в один файл, актуально для директорий
    """
    raise NotImplementedError("Write your local execution stub here")
