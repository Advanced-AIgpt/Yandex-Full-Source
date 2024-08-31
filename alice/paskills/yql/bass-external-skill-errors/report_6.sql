USE hahn;
PRAGMA yt.InferSchema;
PRAGMA ClassicDivision = 'false';

PRAGMA Library("library.sql");
IMPORT library SYMBOLS
	$get_table_name,
	$aggregation_dir
;


$today = CAST(DateTime::TimestampFromMicroSeconds(YQL::Now()) AS Date);
$two_weeks_ago = CAST($today - Interval("P14DT") AS String);


$skills = (
    SELECT
        id
        , String::ToLower(developerName) as developerName
        , name
        , look
    FROM
        [home/paskills/skills/stable]
);

$tables = (
	SELECT
		List(Path)
	FROM
		FOLDER($aggregation_dir)
	WHERE
		Type == 'table'
		AND $get_table_name(Path) >= $two_weeks_ago
);

$data = (
	SELECT
		$get_table_name(TablePath()) AS fielddate
		, skill_id
		, error_sessions
		, sessions
		, session_error_rate
		, session_details
		, error_requests
		, requests
		, details
		, request_error_rate
	FROM
		EACH($tables) AS t
);


INSERT INTO [home/paskills/skill_errors/reports/stat] WITH TRUNCATE
SELECT
	fielddate
	, skill_id
	, IF(s.name IS NOT NULL AND s.name != '', s.name, skill_id) as skill_name
	, error_sessions
	, sessions
	, session_details
	, error_requests
	, requests
	, details
	, IF(error_sessions > 0 OR error_requests > 0, 1, 0) AS has_errors
	, IF(requests > 10 AND error_requests / requests > 0.05, 1, 0) AS should_ban
FROM
	$data AS t
LEFT JOIN
	$skills AS s
	ON s.id == t.skill_id
WHERE
	skill_id NOT IN ('', '-1')
	AND skill_id IS NOT NULL
	AND s.look != "internal"
ORDER BY
	skill_id, fielddate
;
