USE hahn;
PRAGMA yson.DisableStrict = 'false';

$regex_options = Re2::Options(true as Utf8);
$log_entry_match = Re2::FindAndConsume(
    @@(Skill diagnostic info:\s*{[^}]+})@@
    , $regex_options
);

$parse_entry = ($entry) -> {
    $remove_prefix = Re2::Replace(@@Skill diagnostic info:\s+@@, $regex_options);
    $parsed = Yson::ParseJson($remove_prefix($entry, ''));
    -- RETURN $remove_prefix($entry, '');
    RETURN IF(
        $parsed IS NULL,
        NULL,
        AsStruct(
            Yson::LookupString($parsed, 'client_id') AS client_id
            , COALESCE(
                Yson::LookupString($parsed, 'error_type'), 
                Yson::LookupString($parsed, 'error'),
                ''
            ) as error
            , Yson::LookupString($parsed, 'error_detail') AS error_detail
            , Yson::LookupString($parsed, 'request_id') AS request_id
            , Yson::LookupString($parsed, 'session_id') AS session_id
            , Yson::LookupString($parsed, 'skill_id') AS skill_id
            , Yson::LookupString($parsed, 'source') AS source
            , Yson::LookupInt64 ($parsed, 'timestamp_mcr') AS timestamp_mcr
            , Yson::LookupString($parsed, 'user_id') AS user_id
            , Yson::LookupInt64 ($parsed, 'zora_response_time_mcr') AS zora_response_time_mcr
        )
    );
};

DEFINE ACTION $parse_bass_logs($date) AS

    $log_table = 'logs/bass-vins-log/1d/' || $date;

    $logs = (
        SELECT
            WeakField(content, 'String') AS content
        FROM
            $log_table
    );

    -- split log entries with multiple 'Skill diagnostic info' records
    -- into multiple rows
    $records = (
    FROM $logs
    SELECT
        content
        , $log_entry_match(content) AS log_entries
    WHERE
        content like '%Skill diagnostic info%'
    );

    $parsed = (
    SELECT
        $parse_entry(entry)
    FROM
        $records
    FLATTEN BY log_entries AS entry
    );

    $output_table = 'home/paskills/bass-skill-logs/stable/events/' || $date;

    INSERT INTO $output_table WITH TRUNCATE
    SELECT
        *
    FROM
        $parsed
    FLATTEN COLUMNS
    WHERE
        skill_id IS NOT NULL
    ;
END DEFINE;

$logs_dir = '//logs/bass-vins-log/1d';
$processed_dir = '//home/paskills/bass-skill-logs/stable/events';
$get_table_name = ($path) -> {
    $parts = String::SplitToList($path, '/');
    $parts_size = ListLength($parts);
    RETURN $parts[$parts_size - 1];
};

$missing_dates = (
    SELECT
        AGGREGATE_LIST($get_table_name(logs.Path)) as `date`
    FROM
        FOLDER($logs_dir) AS logs
    LEFT ONLY JOIN
        FOLDER($processed_dir) AS processed
        ON $get_table_name(logs.Path) == $get_table_name(processed.Path)
    WHERE
        logs.Type == 'table'
        AND $get_table_name(logs.Path) >= '2019-01-01'
);

EVALUATE FOR $date IN $missing_dates
DO $parse_bass_logs($date)
;