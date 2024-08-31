PRAGMA yt.Pool = 'paskills';
PRAGMA ClassicDivision = 'false';
PRAGMA yson.DisableStrict = 'true';

use hahn;

$categories = AsDict(
    AsTuple("business_finance", "Бизнес и финансы"), 
    AsTuple("communication", "Общение"), 
    AsTuple("connected_car", "Авто"), 
    AsTuple("education_reference", "Образование"), 
    AsTuple("food_drink", "Еда и напитки"), 
    AsTuple("games_trivia_accessories", "Игры и развлечения"), 
    AsTuple("health_fitness", "Спорт и здоровье"), 
    AsTuple("kids", "Для детей"), 
    AsTuple("lifestyle", "Культура"), 
    AsTuple("local", "Поиск и быстрые ответы"), 
    AsTuple("movies_tv", "Видео"), 
    AsTuple("music_audio", "Аудио и подкасты"), 
    AsTuple("news", "Новости"), 
    AsTuple("productivity", "Продуктивность"), 
    AsTuple("shopping", "Покупки"), 
    AsTuple("smart_home", "Умный дом"), 
    AsTuple("travel_transportation", "Путешествия"), 
    AsTuple("utilities", "Управление"), 
    AsTuple("weather", "Погода")
);

$get_category = ($x) -> { RETURN DictLookup($categories, $x) };

$skills = (
    SELECT 
        t.id as id
        , t.name as name
        , activationPhrases
        , t.slug as slug
        , i.url as logo
        , channel
        , Yson::ConvertToString(Yson::YPath(publishingSettings, "/description")) ?? "" AS description
        , $get_category(Yson::LookupString(publishingSettings, "category")) AS category
    FROM `home/paskills/upload/stable/skills` AS t
    LEFT JOIN `home/paskills/upload/stable/images` AS i ON i.id = t.logoId
    WHERE 
        t.onAir 
        AND NOT t.hideInStore 
        AND t.deletedAt is null
        AND Yson::Contains(publishingSettings, 'category')    
);

$to_words = AugurTokenizer::ToWords("", "", "none");
$to_words_norm = AugurTokenizer::ToWords("", "lemmer-norm", "none");

$replace_punctuation = Re2::Replace(@@[^\p{L}\p{M}0-9]+@@, Re2::Options(false AS CaseSensitive));

$join_yson_array = ($arr) -> {
    return String::JoinFromList(Yson::ConvertToStringList($arr), " ");
};

$sub_result = (
    SELECT
        id
        , name
        , activationPhrases
        , slug
        , description
        , category
        , channel
        , logo
        , ListFilter(
            ListUniq(
                ListExtend(
                    $to_words($replace_punctuation(name, " "), "ru"),
                    $to_words($replace_punctuation($join_yson_array(activationPhrases), " "), "ru"),
                        
                    $to_words_norm($replace_punctuation(name, " "), "ru"),
                    $to_words_norm($replace_punctuation($join_yson_array(activationPhrases), " "), "ru"),
                )
            ),
            ($x) -> { RETURN Unicode::GetLength(CAST($x as Utf8)) > 2; }
        ) as strong_keywords_arr
        , ListFilter(
            ListUniq(
                ListExtend(
                    $to_words($replace_punctuation(description, " "), "ru"),
                    $to_words(category, "ru"),

                    $to_words_norm($replace_punctuation(description, " "), "ru"),
                    $to_words_norm(category, "ru"),
                )
            ),
            ($x) -> { RETURN Unicode::GetLength(CAST($x as Utf8)) > 2; }
        ) as weak_keywords_arr
    FROM $skills
);

$sub_result_strong_keywords = (
    SELECT
        id
        , name as nameOrig
        , name || slug || "-s" as name
        , activationPhrases
        , slug
        , description
        , category
        , channel
        , logo
        , String::JoinFromList(strong_keywords_arr, " ") as keywords
        , true as is_strong_keywords
    FROM $sub_result
);

$custom_filter = ($arr1, $arr2) -> {
    return ListFilter(
        $arr1,
        ($str) -> { return NOT ListHas($arr2, $str); }
    )
};

$sub_result_weak_keywords = (
    SELECT
        id
        , name as nameOrig
        , name || slug || "-w" as name
        , activationPhrases
        , slug
        , description
        , category
        , channel
        , logo
        , String::JoinFromList($custom_filter(weak_keywords_arr, strong_keywords_arr ), " ") as keywords
        , false as is_strong_keywords
    FROM $sub_result
);

$skills_suggest_intermediate = (
    select * from $sub_result_strong_keywords
    union all
    select * from $sub_result_weak_keywords
);

$create_skill_data = ($name, $slug, $logo, $channel) -> {
    $data = AsStruct($slug as slug, $name as name, $logo as logo, $channel as channel);
    $result = AsStruct("skill" as type, $data as data);
    
    RETURN Yson::SerializeJson(Yson::From($result));
};

$last_table_name = (select MAX(Path) from folder('//home/business-chat/prod/export/ranking'));

$skill_stats = (
    SELECT 
        *
    FROM `//home/paskills/stat/dialogovo_stat_current_with_score`
    WHERE
        skill_id != '_total_' 
        AND app_group = '_total_' 
        AND app = '_total_'    
);

$max_dau = (SELECT MAX(dau_1w_avg) from $skill_stats);

$skills_suggest = (
    SELECT 
        CAST(name AS Utf8) as name
        , CAST(keywords AS Utf8) as keywords
        , IF(
            Yson::ConvertToString(channel) == "organizationChat"
            , sc.score ?? 0
            , (skill_score.dau_1w_avg ?? 0) / $max_dau + IF(is_strong_keywords, 2, 1)
        ) as weight
        , $create_skill_data(nameOrig, slug, logo, channel) AS data 
    FROM
        $skills_suggest_intermediate AS s
    LEFT JOIN
        CONCAT($last_table_name) AS sc 
        ON s.id = sc.org_id
    LEFT JOIN
        $skill_stats AS skill_score
        ON s.id == skill_score.skill_id
);

$categories = AsDict(
    AsTuple("business_finance", "Бизнес и финансы"), 
    AsTuple("communication", "Общение"), 
    AsTuple("connected_car", "Авто"), 
    AsTuple("education_reference", "Образование"), 
    AsTuple("food_drink", "Еда и напитки"), 
    AsTuple("games_trivia_accessories", "Игры и развлечения"), 
    AsTuple("health_fitness", "Спорт и здоровье"), 
    AsTuple("kids", "Для детей"), 
    AsTuple("lifestyle", "Культура"), 
    AsTuple("local", "Поиск и быстрые ответы"), 
    AsTuple("movies_tv", "Видео"), 
    AsTuple("music_audio", "Аудио и подкасты"), 
    AsTuple("news", "Новости"), 
    AsTuple("productivity", "Продуктивность"), 
    AsTuple("shopping", "Покупки"), 
    AsTuple("smart_home", "Умный дом"), 
    AsTuple("travel_transportation", "Путешествия"), 
    AsTuple("utilities", "Управление"), 
    AsTuple("weather", "Погода")
);

$create_category_data = ($title, $type) -> {
    $data = AsStruct($type as type, $title as title);
    $result = AsStruct("category" as type, $data as data);
    
    RETURN Yson::SerializeJson(Yson::From($result)); 
};

$categories_suggest = (
    SELECT 
        name
        , String::JoinFromList($to_words(name, "ru"), " ") AS keywords
        , 4 as weight 
        , $create_category_data(name, slug) as data 
    FROM (
        SELECT a.0 as slug, a.1 as name FROM (
            SELECT DictItems($categories) AS a
        ) FLATTEN BY a
    )
);

$suggest_data = (
    SELECT * from $categories_suggest
    UNION ALL
    SELECT * from $skills_suggest
);

$suggest_data_unicode = (
    SELECT
        CAST(name as Utf8) as name
        , CAST(keywords as Utf8) as keywords
        , data
        , weight
    FROM $suggest_data
);

-- Промежуточная таблица нужна, чтобы в результирующей отсутствовало поле _yql_column_0 которое появлялось при ORDER BY
$intermediate_suggests_table = 'home/paskills/suggests/intermediate';
$intermediate_suggests_table_schema = Struct<name: Utf8, keywords: Utf8, weight:Double, data:String>;

INSERT INTO $intermediate_suggests_table WITH TRUNCATE
SELECT * FROM $suggest_data_unicode AS suggest_data
ORDER BY suggest_data.weight DESC;

COMMIT;

INSERT INTO `home/paskills/suggests/stable` WITH TRUNCATE
SELECT name, keywords, weight, data FROM $intermediate_suggests_table;

-- Очищаю промежуточную таблицу, чтобы не занимать место
INSERT INTO $intermediate_suggests_table WITH TRUNCATE 
SELECT * FROM AS_TABLE(ListCreate($intermediate_suggests_table_schema));
