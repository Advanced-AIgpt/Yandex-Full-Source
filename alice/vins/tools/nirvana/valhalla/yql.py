# coding: utf-8


SPLIT_WORDS = '''$isTagged = ($t) -> {
    RETURN $t LIKE "B-%" OR $t LIKE "I-%";
};
$hasAnyTags = ($f) -> {
    RETURN ListHasItems(ListFilter(Yson::ConvertToStringList(Yson::YPath(
        Yson::Parse($f), "/nlu_tags"
    ), Yson::Options(false as Strict)), $isTagged));
};

$script_split = "def split(text): return list(enumerate(filter(None, text.split(' '))))";
$split = Python3::split(Callable<(Utf8?)->List<Tuple<Int32, Utf8>>?>, $script_split);
$smart_split = ($text, $indicator) -> {
    return IF($indicator, $split(CAST($text as Utf8)), AsList(AsTuple(1, CAST($text AS Utf8))));
};

INSERT INTO {{output1}} WITH TRUNCATE
SELECT
    `words`.1 AS `text`,
    `words`.0 AS `word_order`,
    `request_id`
FROM (
    SELECT
        *
    FROM (
        SELECT
            $smart_split(`text`, $hasAnyTags(`additional_options`)) AS `words`,
            `request_id`
        FROM {{input1}} as `source`
    )
    FLATTEN LIST BY (`words`)
) AS `resulting_query`;
'''

JOIN_WORDS = '''PRAGMA yt.InferSchema = '1';

$script = @@
import json

def create(item):
    return [item]

def add(state, item):
    return state + [item]

def merge(state_a, state_b):
    return state_a + state_b

def get_result(state):
    return " ".join(z[0] for z in sorted(state, key=lambda x: x[1]))

def serialize(state):
    return json.dumps(state)

def deserialize(serialized):
    return json.loads(serialized)
@@;

$create = Python3::create(Callable<(Tuple<Utf8?, Int32>)->Resource<Python3>>, $script);
$add = Python3::add(Callable<(Resource<Python3>, Tuple<Utf8?, Int32>)->Resource<Python3>>, $script);
$merge = Python3::merge(Callable<(Resource<Python3>, Resource<Python3>)->Resource<Python3>>, $script);
$get_result = Python3::get_result(Callable<(Resource<Python3>)->Utf8>, $script);
$serialize = Python3::serialize(Callable<(Resource<Python3>)->Utf8>, $script);
$deserialize = Python3::deserialize(Callable<(String)->Resource<Python3>>, $script);

$udaf_factory = AGGREGATION_FACTORY(
    "UDAF",
    $create,
    $add,
    $merge,
    $get_result,
    $serialize,
    $deserialize
);

INSERT INTO {{output1}} WITH TRUNCATE
SELECT
    `texts`.*,
    `source`.`text` AS `original_text`,
    `source`.* WITHOUT `source`.`text`, `source`.`request_id`
FROM (
        SELECT
            `request_id`,
            AGGREGATE_BY((CAST(`text` AS Utf8), Unwrap(CAST(`word_order` AS Int32))), $udaf_factory) AS `text`
        FROM {{input1}}
        GROUP BY `request_id`
    ) AS `texts`
    LEFT JOIN {{input2}} AS `source`
        ON `texts`.`request_id` == `source`.`request_id`;
'''

MERGE_REQUEST_RESULT = '''PRAGMA SimpleColumns;
PRAGMA yt.InferSchema = '1';

INSERT INTO {{output1}} WITH TRUNCATE
SELECT
    lhs.request AS request,
    lhs.response AS sample_features,
    rhs.*
FROM (
    SELECT
        Yson::Serialize(Yson::ParseJson(postdata)) as request,
        FetchedResult as response,
        cast(id as String) as request_id
    FROM {{input2}}
) AS lhs
INNER JOIN {{input1}} AS rhs
USING (request_id)
WHERE rhs.text != '';
'''
