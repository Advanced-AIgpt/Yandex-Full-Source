import vh3
from vh3 import MRTable, Integer, block_args, Expr
from vh3.decorator import graph

from alice.beggins.internal.vh.flows.common import static_name, EmbedderType
from alice.beggins.internal.vh.operations import ext
import alice.beggins.internal.vh.flows.yql as yql_scripts


def get_map_to_scraper_request(embedder_type: str) -> str:
    return f"""
PRAGMA yt.DefaultCluster = "hahn";

$PythonScript = @@
from urllib.parse import urlencode

def get_row(row_number, text):
    text = text.decode("utf-8")
    rules  =  ['AliceRequest', 'AliceNormalizer', '{embedder_type}']
    wizextra = ['alice_preprocessing=true']
    host = 'beggins.in.alice.yandex.net'
    url = f'http://{{host}}/wizard'
    params = {{
            'text': text,
            'wizclient': 'megamind',
            'tld': 'ru',
            'format': 'json',
            'lang': 'ru',
            'uil': 'ru',
            'dbgwzr': 2,
            'wizextra': ';'.join(wizextra),
            'rwr': rules,
            'markup': '',
        }}
    uri = url + '?' + urlencode(params, True)
    cookies = []
    headers = ["X-Yandex-Internal-Request: 1",]
    method = "GET"
    request_id = str(row_number)
    row = {{"cookies":cookies, "headers":headers, "id":request_id, "method":method, "uri":uri}}
    return row
@@;

$get_row = Python3::get_row(Callable<(Uint64,String?)->Struct<cookies:List<String>,headers:List<String>,id:String,method:String,uri:String>>, $PythonScript);

PRAGMA yt.InferSchema = '1';

INSERT INTO {{{{output1}}}} WITH TRUNCATE
SELECT * FROM (
SELECT $get_row(ROW_NUMBER() over w, text) FROM {{{{input1}}}}
WINDOW
w AS (ROWS BETWEEN UNBOUNDED PRECEDING AND UNBOUNDED FOLLOWING)
) FLATTEN COLUMNS;
"""


def get_map_from_scraper_request(embedder_rule: str) -> str:
    return f"""
PRAGMA yt.DefaultCluster = "hahn";

$PythonScript = @@
from json import loads

def get_row(response):
    response = response.decode("utf-8")
    dict_response = loads(response)

    try:
        text = dict_response['rules']['AliceRequest']['OriginalText']['Text']
    except KeyError:
        text = None

    try:
        norm_text = dict_response['rules']['AliceRequest']['FstText']
    except KeyError:
        norm_text = None

    try:
        embeddings_str  = dict_response['rules']['{embedder_rule}']['SentenceEmbedding']
        sentence_embeddings = tuple(map(float, embeddings_str))
    except KeyError:
        sentence_embeddings = None

    return {{
        'text': text,
        'normalized_text': norm_text,
        'sentence_embedding': sentence_embeddings
    }}
@@;

$get_row = Python3::get_row(Callable<(String?)->Struct<text:String?,normalized_text:String?,sentence_embedding:List<Double>?>>, $PythonScript);

$result =  (
    SELECT $get_row(FetchedResult), id FROM {{{{input1}}}}
);

PRAGMA yt.InferSchema = '1';
PRAGMA OrderedColumns;

INSERT INTO {{{{output1}}}} WITH TRUNCATE
SELECT text, normalized_text, sentence_embedding
FROM $result
FLATTEN COLUMNS
WHERE
    text IS NOT NULL
    AND normalized_text IS NOT NULL
    AND sentence_embedding IS NOT NULL;

INSERT INTO {{{{output2}}}} WITH TRUNCATE
SELECT id, text, normalized_text, sentence_embedding
FROM $result
FLATTEN COLUMNS
WHERE
    text IS NULL
    OR normalized_text IS NULL
    OR sentence_embedding IS NULL;
"""


JOIN_WITH_INPUT_TABLE = """
PRAGMA yt.InferSchema = '99';

INSERT INTO {{output1}} WITH TRUNCATE
SELECT processed.normalized_text AS normalized_text,
       processed.sentence_embedding AS sentence_embedding,
       input_table.*,
FROM {{input1}} AS input_table
JOIN {{input2}} AS processed USING(text)
"""


UNIQUE_SCRIPT = '''
PRAGMA yt.InferSchema = '99';

INSERT INTO {{output1}} WITH TRUNCATE
SELECT text AS text,
FROM {{input1}}
GROUP BY text;
'''


@static_name('scrape queries')
@graph(workflow_id='https://nirvana.yandex-team.ru/flow/2488169c-2aeb-4675-988b-11be4a69a3d8')
def scrape_queries(input_table: MRTable, max_rps: Integer,
                   embedder: EmbedderType) -> MRTable:
    embedder_type = '[#if global["embedder"] == "Beggins"]' \
                    'AliceBegginsEmbedder' \
                    '[#elseif global["embedder"] == "Zeliboba"]' \
                    'AliceZelibobaEmbedder' \
                    '[/#if]'

    unified_data = ext.yql_2(
        input1=[input_table],
        request=UNIQUE_SCRIPT,
        **block_args(name='unify data'),
    ).output1

    requests = ext.yql_2(
        input1=[unified_data],
        request=Expr(get_map_to_scraper_request(embedder_type)),
        **block_args(name='map requests to scraper'),
    ).output1

    responses = ext.scraper_over_yt_downloader_with_http_no_apphost_fetcher(
        input_table=requests,
        max_rps=max_rps,
        **block_args(name='scrape requests')
    ).output_table

    samples = ext.yql_2(
        input1=[responses],
        request=Expr(get_map_from_scraper_request(embedder_type)),
        **block_args(name='map responses from scraper'),
    ).output1

    return ext.yql_2(
        input1=[input_table],
        input2=[samples],
        request=JOIN_WITH_INPUT_TABLE,
        **block_args(name='join with meta'),
    ).output1


@static_name('scrape queries with cache')
@graph(workflow_id='https://nirvana.yandex-team.ru/flow/4f9ae8bf-046b-4595-87ab-bfd135be453e')
def scrape_queries_with_cache(input_table: MRTable, max_rps: Integer = 1000,
                              embedder: EmbedderType = vh3.Factory(lambda: vh3.context.embedder)) -> MRTable:
    cache_directory = '[#if global["embedder"] == "Beggins"]' \
                      '//home/alice/beggins/scraper_cache/beggins' \
                      '[#elseif global["embedder"] == "Zeliboba"]' \
                      '//home/alice/beggins/scraper_cache/zeliboba' \
                      '[/#if]'

    tables = ext.yql_2(
        input1=[input_table],
        request=Expr(yql_scripts.get_separate_table_script(cache_directory)),
        **block_args(name='separate table by presence in cache'),
    )

    uncached = ext.skip_empty_mr_table(input=tables.output1)
    cached = tables.output2

    scraped = scrape_queries(
        input_table=uncached,
        max_rps=max_rps,
        embedder=embedder,
    )

    # TODO(andreyshspb): MEGAMIND-3925 Move cache update reaction outside personal quota

    return ext.yql_2(
        input1=[scraped, cached],
        request=Expr(yql_scripts.MERGE_RESULTS_SCRIPT),
        **block_args(name='convert results to correct type'),
    ).output1
