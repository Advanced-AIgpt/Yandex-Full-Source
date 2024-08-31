USE hahn;
PRAGMA yt.InferSchema;
PRAGMA ClassicDivision = 'false';

PRAGMA Library("library.sql");
IMPORT library SYMBOLS
	$get_table_name,
	$job_log_table,
	$logs_daily_dir
;


-- **********************************************************************
-- Запись последней обработанной даты в лог
-- **********************************************************************



INSERT INTO $job_log_table WITH TRUNCATE
SELECT
 	MAX($get_table_name(Path)) as last_processed_table,
 	YQL::Now() AS utctimestamp
FROM
 	FOLDER($logs_daily_dir)
;