USE hahn;
PRAGMA yt.InferSchema;
PRAGMA ClassicDivision = 'false';
PRAGMA Library("library.sql");
IMPORT library SYMBOLS
	$get_table_name,
	$job_log_table,
	$replace_minutes_and_seconds,
	$requests_dir,
	$responses_dir,
	$request_log_dir
;


$today = CAST(DateTime::TimestampFromMicroSeconds(YQL::Now()) AS Date);
$two_weeks_ago = $today - Interval("P14DT");


-- **********************************************************************
-- Построение лога запросов по таблицам запросов и ответов
-- **********************************************************************

$input_tables = (
    SELECT
        Path,
        $get_table_name(Path) AS table_name
    FROM
        FOLDER($requests_dir) AS logs
	WHERE
        Type == 'table'
);

$output_tables = (
    SELECT
        $get_table_name(Path) AS table_name
    FROM
        FOLDER($request_log_dir)
	WHERE
        Type == 'table'
);

$last_processed_daily_table = (SELECT MAX(last_processed_table) FROM $job_log_table);

$missing_tables = (
    SELECT
        LIST(Path)
    FROM
        $input_tables AS log
    LEFT ONLY JOIN
        $output_tables AS output
        ON log.table_name == output.table_name
    WHERE
        table_name >= CAST($two_weeks_ago AS STRING)
        OR table_name >= $last_processed_daily_table
);


DEFINE ACTION $merge_request_log($requests_table, $responses_table, $output_table) AS

	INSERT INTO $output_table WITH TRUNCATE
    SELECT
        req.reqid as reqid,
        req.skill_id as skill_id,
        req.session_id as session_id,
        req.session_seq as session_seq,
			if(problem_http_code is not null,
			   String::JoinFromList(AsList(problem_type, cASt(problem_http_code AS string)), "_"),
			   problem_type) AS problem,
        if(problem_type is not null, 1, 0) AS is_error,
        req.time as request_time,
        resp.time as response_time,
        CAST(DateTime::TimestampFromString(resp.time) - DateTime::TimestampFromString(req.time) AS double) / 1000000.0 AS time,
        $replace_minutes_and_seconds(
            DateTime::ToStringUpToSeconds(
                DateTime::FromString(req.time)
            ),
        "00:00") AS hour
	from
		$requests_table AS req
	left join
		$responses_table AS resp
		ON req.reqid == resp.reqid
	where
		client_id != 'developer console'
		and app_id NOT LIKE '%com.yandex.vins.tests%'
	ORDER BY
		skill_id
    ;
END DEFINE;


EVALUATE FOR $table IN $missing_tables
DO $merge_request_log(
	$requests_dir    || '/' || $get_table_name($table),
	$responses_dir   || '/' || $get_table_name($table),
	$request_log_dir || '/' || $get_table_name($table)
);
