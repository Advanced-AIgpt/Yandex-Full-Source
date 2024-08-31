USE hahn;
PRAGMA yt.InferSchema;
PRAGMA ClassicDivision = 'false';

PRAGMA Library("library.sql");
IMPORT library SYMBOLS
	$get_table_name,
	$job_log_table,
	$parse_logs,
	$requests_dir,
	$responses_dir,
	$logs_daily_dir
;

$today = CAST(DateTime::TimestampFromMicroSeconds(YQL::Now()) AS Date);
$two_weeks_ago = $today - Interval("P14DT");


-- **********************************************************************
-- Загрузка дневных логов
--
-- Запрос парсит дневные таблицы логов в таблицы запросов и ответов
-- **********************************************************************

-- ищем необработанные таблицы

$input_tables = (
    SELECT
        Path,
        $get_table_name(Path) as table_name
    FROM
        FOLDER($logs_daily_dir) AS logs
);

$output_tables = (
    SELECT
        $get_table_name(Path) as table_name
    FROM
        FOLDER($requests_dir)
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


EVALUATE
FOR $table IN $missing_tables
DO $parse_logs(
	$table,
	$requests_dir || '/' || $get_table_name($table),
 	$responses_dir || '/' || $get_table_name($table)
)
;
