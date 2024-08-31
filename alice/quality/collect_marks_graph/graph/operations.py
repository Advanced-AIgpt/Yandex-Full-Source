import vh3
import typing


@vh3.decorator.external_operation("https://nirvana.yandex-team.ru/operation/83f0cf88-63d9-11e6-a050-3c970e24a776")
@vh3.decorator.nirvana_output_names("moved")
@vh3.decorator.nirvana_names_transformer(vh3.name_transformers.snake_to_dash, options=True)
def mr_move_table(
        *,
        yt_token: vh3.Secret = vh3.Factory(lambda: vh3.context.yt_token),
        src: vh3.MRTable,
        dst_path: vh3.String = None,
        force: vh3.Boolean = True,
        timestamp: vh3.String = vh3.Factory(lambda: vh3.context.timestamp),
        preserve_account: vh3.Boolean = True,
        preserve_expiration_time: vh3.Boolean = False
) -> vh3.MRTable:
    """
    MR Move Table

    Moves MR tables on the same cluster.

    :param yt_token: YT Token:
      Name of Nirvana Secret holding your YT Access Token
    :param src: Source table.
    :param dst_path: Destination Path:
      Path to destination table.

      If not specified, the source table will be moved to a subdirectory of MR Account's home directory, e.g. `//home/<MR Account>/<Workflow Owner>/nirvana/<Execution ID>'
    :param force: Force:
      Overwrite destination table if it already exists
    :param timestamp: Timestamp:
      Set a recent, not previously used timestamp to force "MR Move" to run even if `src' table was already moved to `dst' once
    :param preserve_account: Preserve account
      Preserve account of moved nodes
    :param preserve_expiration_time: Preserve expiration time
      Preserve expiration time of moved nodes
    """
    raise NotImplementedError("Write your local execution stub here")


class Yql1Output(typing.NamedTuple):
    output1: vh3.MRTable
    directory: vh3.MRDirectory


@vh3.decorator.external_operation("https://nirvana.yandex-team.ru/operation/bca6e17a-068c-4031-bc44-5ad64dd71c52")
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
def yql_1(
        *,
        mr_account: vh3.String = vh3.Factory(lambda: vh3.context.mr_account),
        yt_token: vh3.Secret = vh3.Factory(lambda: vh3.context.yt_token),
        yql_token: vh3.Secret = vh3.Factory(lambda: vh3.context.yql_token),
        request: vh3.String = "INSERT INTO {{output1}} SELECT * FROM {{input1}};",
        py_code: vh3.String = None,
        py_export: vh3.MultipleStrings = (),
        py_version: vh3.Enum[typing.Literal["Python2", "ArcPython2", "Python3"]] = "Python3",
        mr_default_cluster: vh3.Enum[
            typing.Literal[
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
        yt_pool: vh3.String = vh3.Factory(lambda: vh3.context.yt_pool),
        ttl: vh3.Integer = 7200,
        max_ram: vh3.Integer = 256,
        max_disk: vh3.Integer = 1024,
        timestamp: vh3.String = vh3.Factory(lambda: vh3.context.timestamp),
        param: vh3.MultipleStrings = (),
        mr_output_path: vh3.String = None,
        yt_owners: vh3.String = None,
        use_account_tmp: vh3.Boolean = False,
        code_revision: vh3.String = None,
        code_work_dir: vh3.String = None,
        arcanum_token: vh3.Secret = None,
        svn_user_name: vh3.String = None,
        svn_user_id_rsa: vh3.Secret = None,
        svn_operation_source: vh3.MultipleStrings = (),
        yql_operation_title: vh3.String = "YQL Nirvana Operation: {{nirvana_operation_url}}",
        mr_output_ttl: vh3.Integer = None,
        job_metric_tag: vh3.String = None,
        mr_transaction_policy: vh3.Enum[typing.Literal["MANUAL", "AUTO"]] = "AUTO",
        input1: typing.Sequence[vh3.MRTable] = (),
        files: typing.Sequence[
            typing.Union[
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
) -> Yql1Output:
    """
    YQL 1

    Apply YQL script on MapReduce

    Code: https://a.yandex-team.ru/arc/trunk/arcadia/dj/nirvana/operations/yql/yql

    User guide: https://wiki.yandex-team.ru/nirvana-ml/ml-marines/#yql

    :param mr_account: MR Account:
      MR Account Name.
      By default, output tables and directories will be created in some subdirectory of home/<MR Account>/<workflow owner>/nirvana
    :param yt_token: YT Token:
      ID of Nirvana Secret with YT access token (https://nda.ya.ru/3RSzVU).
      Guide to Nirvana Secrets: https://nda.ya.ru/3RSzWZ
    :param yql_token: YQL Token:
      YQL OAuth Token, see https://wiki.yandex-team.ru/kikimr/yql/userguide/cli/#autentifikacija
    :param request: Request
      YQL request
    :param py_code: Python Code
      Python user defined functions definition
    :param py_export: Python Export
      Python user defined functions declaration
    :param py_version: Python Version
      Python user defined functions version, https://clubs.at.yandex-team.ru/yql/2400
    :param mr_default_cluster: Default YT cluster:
      Default YT cluster
    :param yt_pool: YT Pool:
      Pool used by YT scheduler. Leave blank to use default pool.
      This option has no effect on YaMR.
    :param timestamp: Timestamp for caching
      Any string used for Nirvana caching only
    :param param: Parameters
      List of 'name=value' items which could be accessed as {{param[name]}}
    :param mr_output_path: MR Output Path:
      Directory for output MR tables and directories.
      Limited templating is supported: `${param["..."]}`, `${meta["..."]}`, `${mr_input["..."]}` (path to input MR *directory*) and `${uniq}` (= unique path-friendly string).
    :param yt_owners: YT Owners
      Additional YT users allowed to read and manage operations
    :param use_account_tmp: Use tmp in account
      Use tmp folder in account but not in //tmp for avoid fails due to tmp overquota, recommended for production processes
    :param code_revision: Code default revision
      Default code revision for {{arcadia:/...}}
    :param code_work_dir: Code default directory
      Default code working directory for {{./...}}
    :param arcanum_token: Arcanum Token
      Arcanum Token, see https://wiki.yandex-team.ru/arcanum/api/
    :param svn_user_name: SVN User name
      SVN user name for operation source and {{arcadia:/...}}
    :param svn_user_id_rsa: SVN User private key
      SVN user private key for operation source and {{arcadia:/...}}
    :param svn_operation_source: SVN Operation source
      The YQL operation source path on SVN, should start with arcadia:/ or svn+ssh://, may contain @revision
    :param yql_operation_title: YQL Operation title
      YQL operation title for monitoring
    :param mr_output_ttl: MR Output TTL, days:
      TTL in days for mr-output-path directory and outputs which are inside the directory
    :param job_metric_tag: Job metric tag
      Tag for monitoring of resource usage
    :param mr_transaction_policy: MR Transaction policy
      Transaction policy, in auto policy yql operations are canceled when nirvana workflow in canceled
    :param input1: Input 1
    :param files: Attached files: if link_name is specified it is interpreted as file name, otherwise the input is unpacked as tar archive
    """
    raise NotImplementedError("Write your local execution stub here")


class Yql2Output(typing.NamedTuple):
    output1: vh3.MRTable
    output2: vh3.MRTable
    directory: vh3.MRDirectory


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
        request: vh3.String = "INSERT INTO {{output1}} SELECT * FROM {{input1}};\nINSERT INTO {{output2}} SELECT * FROM {{input2}};",
        py_code: vh3.String = None,
        py_export: vh3.MultipleStrings = (),
        py_version: vh3.Enum[typing.Literal["Python2", "ArcPython2", "Python3"]] = "Python3",
        mr_default_cluster: vh3.Enum[
            typing.Literal[
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
        yt_pool: vh3.String = vh3.Factory(lambda: vh3.context.yt_pool),
        ttl: vh3.Integer = 7200,
        max_ram: vh3.Integer = 256,
        max_disk: vh3.Integer = 1024,
        timestamp: vh3.String = vh3.Factory(lambda: vh3.context.timestamp),
        param: vh3.MultipleStrings = (),
        mr_output_path: vh3.String = None,
        yt_owners: vh3.String = None,
        use_account_tmp: vh3.Boolean = False,
        code_revision: vh3.String = None,
        code_work_dir: vh3.String = None,
        arcanum_token: vh3.Secret = None,
        svn_user_name: vh3.String = None,
        svn_user_id_rsa: vh3.Secret = None,
        svn_operation_source: vh3.MultipleStrings = (),
        yql_operation_title: vh3.String = "YQL Nirvana Operation: {{nirvana_operation_url}}",
        mr_output_ttl: vh3.Integer = None,
        job_metric_tag: vh3.String = None,
        mr_transaction_policy: vh3.Enum[typing.Literal["MANUAL", "AUTO"]] = "AUTO",
        input1: typing.Sequence[vh3.MRTable] = (),
        input2: typing.Sequence[vh3.MRTable] = (),
        files: typing.Sequence[
            typing.Union[
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
      MR Account Name.
      By default, output tables and directories will be created in some subdirectory of home/<MR Account>/<workflow owner>/nirvana
    :param yt_token: YT Token:
      ID of Nirvana Secret with YT access token (https://nda.ya.ru/3RSzVU).
      Guide to Nirvana Secrets: https://nda.ya.ru/3RSzWZ
    :param yql_token: YQL Token:
      YQL OAuth Token, see https://wiki.yandex-team.ru/kikimr/yql/userguide/cli/#autentifikacija
    :param request: Request
      YQL request
    :param py_code: Python Code
      Python user defined functions definition
    :param py_export: Python Export
      Python user defined functions declaration
    :param py_version: Python Version
      Python user defined functions version, https://clubs.at.yandex-team.ru/yql/2400
    :param mr_default_cluster: Default YT cluster:
      Default YT cluster
    :param yt_pool: YT Pool:
      Pool used by YT scheduler. Leave blank to use default pool.
      This option has no effect on YaMR.
    :param timestamp: Timestamp for caching
      Any string used for Nirvana caching only
    :param param: Parameters
      List of 'name=value' items which could be accessed as {{param[name]}}
    :param mr_output_path: MR Output Path:
      Directory for output MR tables and directories.
      Limited templating is supported: `${param["..."]}`, `${meta["..."]}`, `${mr_input["..."]}` (path to input MR *directory*) and `${uniq}` (= unique path-friendly string).
    :param yt_owners: YT Owners
      Additional YT users allowed to read and manage operations
    :param use_account_tmp: Use tmp in account
      Use tmp folder in account but not in //tmp for avoid fails due to tmp overquota, recommended for production processes
    :param code_revision: Code default revision
      Default code revision for {{arcadia:/...}}
    :param code_work_dir: Code default directory
      Default code working directory for {{./...}}
    :param arcanum_token: Arcanum Token
      Arcanum Token, see https://wiki.yandex-team.ru/arcanum/api/
    :param svn_user_name: SVN User name
      SVN user name for operation source and {{arcadia:/...}}
    :param svn_user_id_rsa: SVN User private key
      SVN user private key for operation source and {{arcadia:/...}}
    :param svn_operation_source: SVN Operation source
      The YQL operation source path on SVN, should start with arcadia:/ or svn+ssh://, may contain @revision
    :param yql_operation_title: YQL Operation title
      YQL operation title for monitoring
    :param mr_output_ttl: MR Output TTL, days:
      TTL in days for mr-output-path directory and outputs which are inside the directory
    :param job_metric_tag: Job metric tag
      Tag for monitoring of resource usage
    :param mr_transaction_policy: MR Transaction policy
      Transaction policy, in auto policy yql operations are canceled when nirvana workflow in canceled
    :param input1: Input 1
    :param input2: Input 2
    :param files: Attached files: if link_name is specified it is interpreted as file name, otherwise the input is unpacked as tar archive
    """
    raise NotImplementedError("Write your local execution stub here")


@vh3.decorator.external_operation("https://nirvana.yandex-team.ru/operation/99342463-3ddf-4f62-84ff-d540af3f5e0e")
@vh3.decorator.nirvana_names(
    yt_token="yt-token",
    yql_token="yql-token",
    mr_account="mr-account",
    yt_pool="yt-pool",
    eval_data_folder="eval-data-folder",
)
@vh3.decorator.nirvana_output_names("learn_marks")
def collect_ue2e_marks_for_one_scenario(
    *,
    scenario_name: vh3.String,
    data_part: vh3.String,
    input_basket: vh3.MRTable,
    cache_sync: vh3.Integer = 2021052700009,
    prod_url: vh3.String = "http://vins.hamster.alice.yandex.net/speechkit/app/pa/",
    prod_experiments: vh3.String = "{}",
    hitman_labels: vh3.MultipleStrings = (),
    yt_token: vh3.Secret = vh3.Factory(lambda: vh3.context.yt_token),
    timestamp: vh3.String = vh3.Factory(lambda: vh3.context.timestamp),
    yql_token: vh3.Secret = vh3.Factory(lambda: vh3.context.yql_token),
    mr_account: vh3.String = vh3.Factory(lambda: vh3.context.mr_account),
    yt_pool: vh3.String = vh3.Factory(lambda: vh3.context.yt_pool),
    eval_data_folder: vh3.String,
    hitman_token: vh3.Secret = "robot-alice-ue2e_hitman_token",
) -> vh3.OptionalOutput[vh3.MRTable]:
    """
    Collect ue2e marks and process them from result to [0.0;1.0] system

    :param eval_data_folder: Eval data folder
    :param input_basket:
      Вход для кастомной таблички.
    :param yt_token: YT Token
    :param timestamp: Timestamp
    :param yql_token: YQL Token
    :param mr_account: MR Account
    :param yt_pool: YT Pool
    """
    raise NotImplementedError("Write your local execution stub here")
