USE hahn;
PRAGMA yt.InferSchema;

PRAGMA Library("library.sql");
IMPORT library SYMBOLS
	$get_table_name
;

-- **********************************************************************
-- Построение отчётов
-- **********************************************************************


$today = CAST(DateTime::TimestampFromMicroSeconds(YQL::Now()) AS Date);
$two_weeks_ago = CAST($today - Interval("P14DT") AS String);

DEFINE ACTION $drop_table($table) AS
    DROP TABLE $table;
END DEFINE;


$tables = (
    SELECT Path FROM FOLDER([home/paskills/skill_errors/requests])
    UNION ALL
    SELECT Path FROM FOLDER([home/paskills/skill_errors/responses])
    UNION ALL
    SELECT Path FROM FOLDER([home/paskills/skill_errors/request_log])
);

$tables = (
	SELECT
		List(Path)
	FROM
		$tables
	WHERE
		$get_table_name(Path) < $two_weeks_ago
);

EVALUATE FOR $table IN $tables
DO $drop_table($table);