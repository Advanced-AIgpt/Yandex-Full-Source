from typing import NamedTuple, Literal, Sequence, Union

import vh3


class Yql2Output(NamedTuple):
    output1: vh3.OptionalOutput[vh3.MRTable]
    output2: vh3.OptionalOutput[vh3.MRTable]
    directory: vh3.OptionalOutput[vh3.MRDirectory]


@vh3.decorator.external_operation("https://nirvana.yandex-team.ru/operation/5a637316-9863-4644-8e7d-c2fa77584679")
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
    mr_account: vh3.String,
    yt_token: vh3.Secret,
    yql_token: vh3.Secret,
    request: vh3.String = "INSERT INTO {{output1}} SELECT * FROM {{input1}};\nINSERT INTO {{output2}} SELECT * FROM {{input2}};",
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
    ] = "hahn",
    yt_pool: vh3.String = None,
    ttl: vh3.Integer = 7200,
    max_ram: vh3.Integer = 256,
    max_disk: vh3.Integer = 1024,
    timestamp: vh3.String = None,
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
