USE hahn;

-- ***********************************
-- Глобальные переменные
-- ***********************************

$today = CAST(DateTime::TimestampFromMicroSeconds(YQL::Now()) AS Date);
$two_weeks_ago = $today - Interval("P14DT");

-- **********************************************************************
-- Названия таблиц
-- **********************************************************************

$logs_daily_dir = 'logs/bass-vins-log/1d';
$logs_stream_dir = 'logs/bass-vins-log/stream/5min';

$requests_dir = 'home/paskills/skill_errors/requests';
$responses_dir = 'home/paskills/skill_errors/responses';
$request_log_dir = 'home/paskills/skill_errors/request_log';
$aggregation_dir = 'home/paskills/skill_errors/agg_1d';


$job_log_table = 'home/paskills/skill_errors/daily_job_log';


-- **********************************************************************
-- Лямбда-функции
-- **********************************************************************

$get_table_name = ($table_path) -> {
    $list = String::SplitToList($table_path, '/');
    RETURN $list{ListLength($list) - 1};
};


-- **********************************************************************
-- Регулярки для разбора логов
-- **********************************************************************

$json_capture = Re2::Capture("{.*");
-- источник: https://a.yANDex-team.ru/arc/trunk/arcadia/quality/functionality/cards_service/bass/external_skill/error.h
$problem_types_capture = Re2::Capture("((external_skill_http_error)|(external_skill_http_parse_error)|(external_skill_fetch_timeout)|(bad_url)|(bad_json)|(unknown)|(unsupported_version)|(required_field_missing)|(size_exceeded)|(api)|(invalid_card_type)|(invalid_image_id)|(invalid_card_items))");
$http_code_capture = Re2::Capture("\"code\":[0-9]{3}");
$skill_id_capture = Re2::Capture("{[^{}]*skill_id[^{}]*}");
$session_id_capture = Re2::Capture("{[^{}]*session_id[^{}]*}");
$app_id_capture = Re2::Capture("\"app_id\":\"[^\"]*\"");
$strip_html = Re2::Replace("<.*>");
$replace_minutes_and_seconds = Re2::Replace("\\d\\d:\\d\\dZ?$");
$session_capture = Re2::Capture("{[^}]*session[^}]*\"value\":(null|{[^}]*})[^}]*}");


-- **********************************************************************
-- Функции для разбора логов
-- **********************************************************************

DEFINE ACTION $parse_logs($log_table, $requests_table, $responses_table) AS
	$external_skills = (
		FROM $log_table SELECT
			WeakField(content, "String") AS content,
			WeakField(iso_eventtime, "String") AS time,
			WeakField(reqid, "String") AS reqid
		WHERE
			WeakField(env, "String") in ("production", "priemka")
			AND
			WeakField(content, "String") LIKE "%personal_assistant.scenarios.external_skill%"
	);

    $no_new_lines = (
        SELECT
            time,
            reqid,
            $json_capture($strip_html(String::RemoveAll(content, "\r\n"), ""))._0 AS content,
            content AS original_content
        FROM
            $external_skills
    );

    $parts = (
        SELECT
            time,
            reqid,
            $skill_id_capture(content)._0 AS skill_id,
            IF(
                content like '%problems%',
                COALESCE($problem_types_capture(content)._0, 'UNKNOWN'),
                NULL
            ) AS problem_type,
            CAST(String::ReplaceAll($http_code_capture(content)._0, "\"code\":", "") AS INT) AS problem_http_code,
            $session_capture(content)._0 AS session
        FROM
            $no_new_lines
        WHERE
            content like '%blocks%form%'
    );

    INSERT INTO $responses_table
	SELECT
		time
		, reqid
		, problem_type
		, problem_http_code
		, Yson::ConvertToString(Yson::YPath(Yson::ParseJson(skill_id), "/value")) AS skill_id
		, Yson::ConvertToString(
			Yson::YPath(Yson::ParseJson(session), '/value/id')
		) AS session_id
		, Yson::ConvertToInt64(
			Yson::YPath(Yson::ParseJson(session), '/value/seq')
		) AS session_seq
	FROM
		$parts
    ;

    -- поиск всех запросов; вытаскиваем время, id запроса, id сессии и id клиента


    $client_id_capture = Re2::Capture("\"client_id\":\"[^\"]*\"");
    $client_id_extract_id = Re2::Capture("\"[^\"]*\"$");

    $requests_no_new_lines = (
    SELECT
        time,
        reqid,
        $json_capture($strip_html(String::RemoveAll(content, "\r\n"), ""))._0 AS content,
        $session_capture(content)._0 AS session
    FROM
        $external_skills
    WHERE
        content like 'request%'
    );

	INSERT INTO $requests_table
	SELECT
		time
		, reqid
		, Yson::ConvertToString(Yson::YPath(Yson::ParseJson($skill_id_capture(content)._0), "/value")) AS skill_id
		, String::RemoveAll(String::ToLower($client_id_extract_id($client_id_capture(content)._0)._0), "\"") AS client_id
		, String::RemoveAll($app_id_capture(content)._0, "\"") AS app_id
		, Yson::ConvertToString(
			Yson::YPath(Yson::ParseJson(session), '/value/id')
		) AS session_id
		, Yson::ConvertToInt64(
			Yson::YPath(Yson::ParseJson(session), '/value/seq')
		) AS session_seq
	FROM
		$requests_no_new_lines
	;

END DEFINE;


EXPORT
	$get_table_name,
	$parse_logs,
	$replace_minutes_and_seconds,
	$json_capture,
	$strip_html,
	$session_capture,
	$http_code_capture,
	$skill_id_capture,
	$problem_types_capture,
	$app_id_capture,
	$logs_daily_dir,
	$logs_stream_dir,
	$requests_dir,
	$responses_dir,
	$request_log_dir,
	$aggregation_dir,
	$job_log_table
;

