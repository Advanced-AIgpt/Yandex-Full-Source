USE hahn;
PRAGMA yt.InferSchema;
PRAGMA ClassicDivision = 'false';

PRAGMA Library("library.sql");
IMPORT library SYMBOLS
	$get_table_name,
	$job_log_table,
	$replace_minutes_and_seconds,
	$request_log_dir,
	$aggregation_dir
;

-- **********************************************************************
-- Построение суточных агрегатов с детализацией по типу ошибок в JSON
-- **********************************************************************



$today = CAST(DateTime::TimestampFromMicroSeconds(YQL::Now()) AS Date);
$two_weeks_ago = $today - Interval("P14DT");



$input_tables = (
    SELECT
        Path,
        $get_table_name(Path) AS table_name
    FROM
        FOLDER($request_log_dir) AS logs
	WHERE
        Type == 'table'
);

$output_tables = (
    SELECT
        $get_table_name(Path) AS table_name
    FROM
        FOLDER($aggregation_dir)
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


DEFINE ACTION $aggregate($request_log, $output) AS
	$pre = (
		SELECT
			skill_id
			, problem
			, session_id
			, if(problem is not null, session_id, null) as error_session_id
			, reqid
			, if(problem is not null, reqid, null) as error_reqid
		from $request_log
	);

	$agg = (
		SELECT
			skill_id
			, COUNT(distinct session_id) as sessions
			, COUNT(distinct error_session_id) as error_sessions
			, COUNT(distinct reqid) as requests
			, COUNT(distinct error_reqid) as error_requests
		FROM 
			$pre
		GROUP BY
			skill_id
	);

	$details_pre = (
		SELECT
			Unwrap(skill_id) AS skill_id
			, Unwrap(problem) AS problem
			, COUNT(DISTINCT reqid) as requests
			, COUNT(DISTINCT session_id) as sessions
		FROM
			$pre
		WHERE
			problem is not null
		GROUP BY
			skill_id, problem
	);
	$details = (
		SELECT
			skill_id
			, ToDict(List(AsTuple(problem,  requests))) AS details
			, ToDict(List(AsTuple(problem, sessions))) as session_details
		FROM
			$details_pre
		GROUP BY
			skill_id
	);


	INSERT INTO $output WITH TRUNCATE
	SELECT
		a.skill_id AS skill_id
		, error_sessions
		, sessions
		, NANVL(error_sessions / sessions, 0.0) AS session_error_rate
		, Yson::SerializeJson(Yson::From(d.details)) AS details
		, error_requests
		, requests
		, NANVL(error_requests / requests, 0.0) as request_error_rate
		, Yson::SerializeJson(Yson::From(d.session_details)) AS session_details
	FROM
		$agg AS a
	LEFT JOIN
		$details AS d
		ON a.skill_id == d.skill_id
	ORDER BY
		skill_id
	;

	COMMIT;

END DEFINE;


EVALUATE FOR $table IN $missing_tables
DO $aggregate(
	$table,
	$aggregation_dir || '/' || $get_table_name($table)
);
