USE hahn;

$tasks_folder = "logs/qloud-router-log/1d";
$results_folder = "home/paskills/stat/search_queries";

DEFINE ACTION $process_task($input_path) AS
    $output_path = String::ReplaceAll($input_path, $tasks_folder, $results_folder);

    INSERT INTO $output_path WITH TRUNCATE
    SELECT 
        q
        , count
    from (
        SELECT 
        q
        , count(*) as count
        from (
            SELECT
                String::Strip(String::ToLower(Url::Decode(Url::GetCGIParam(request, "q")))) as q
            From $input_path
            WHERE
                qloud_project = 'voice-ext' 
                AND 
                qloud_application = 'paskills-int' 
                AND 
                qloud_environment = 'stable'
                AND
                request like '/api/catalogue/v1/dialogs/search%'
        ) as q
        where Unicode::GetLength(CAST(q as Utf8)) > 2
        group by q
    );
END DEFINE;

-- $tasks = (
--     SELECT LIST(Path)
--     from (
--         SELECT Path
--         FROM FOLDER($tasks_folder)
--         WHERE Type == "table"
--     ) as q
-- );

-- EVALUATE FOR $task IN $tasks ?? ListCreate(TypeOf($tasks)) DO $process_task($task);

$dtToday = DateTime::TimestampFromMicroSeconds(YQL::Now());
$dtYesterdayDate = DateTime::ToDate($dtToday - DateTime::IntervalFromDays(1));

DO $process_task($tasks_folder || "/" || $dtYesterdayDate);