use hahn;

$today = CAST(CurrentUtcDatetime() AS Date);

$min_date = Unwrap($today - Interval('P1D'));
$max_date = Unwrap($today);

$dates = (
select 
    ListMap(
        ListFlatMap(
            ListFromRange(
                CAST($min_date AS Int32),
                CAST($max_date AS Int32)
            ), ($x) -> {RETURN CAST($x AS date)}
        )
        , ($d) -> { RETURN CAST($d AS String); }
    ) as dates
);

$csv_contains = ($csv, $el) -> {
    return $el in String::SplitToList($csv, ',');
};

DEFINE ACTION $build_report($date) AS 

    $log_table = '//home/logfeller/logs/dialogovo-prod-diagnostic-info-log/1d/' || $date; 
    
    $timeouts = (
        SELECT 
            skill_id
            , source
            , COUNT(*) AS count
        FROM 
            $log_table
        WHERE
            $csv_contains(errors, 'TIME_OUT')
        GROUP BY 
            skill_id    
            , source
    );
    
    $http_errors = (
        SELECT 
            skill_id
            , source
            , COUNT(*) AS count
        FROM 
            $log_table
        WHERE
            $csv_contains(errors, 'HTTP_ERROR')
        GROUP BY 
            skill_id    
            , source
    );
    
    $parse_errors = (
        SELECT 
            skill_id
            , source
            , COUNT(*) AS count
        FROM 
            $log_table
        WHERE
            $csv_contains(errors, 'INVALID_RESPONSE')
        GROUP BY 
            skill_id    
            , source
    );        
    
    $validation_errors = (
        SELECT 
            skill_id
            , source
            , COUNT(*) AS count
        FROM 
            $log_table
        WHERE
            $csv_contains(errors, 'INVALID_VALUE')
        GROUP BY 
            skill_id    
            , source
    );
    
    $success = (
        SELECT 
            skill_id
            , source
            , COUNT(*) AS count
        FROM 
            $log_table
        WHERE
            errors = ''
        GROUP BY 
            skill_id    
            , source
    );
    
    $all_skill_ids = (
        SELECT skill_id, source FROM $timeouts
        UNION ALL
        SELECT skill_id, source FROM $http_errors
        UNION ALL
        SELECT skill_id, source FROM $validation_errors
        UNION ALL
        SELECT skill_id, source FROM $parse_errors
        UNION ALL
        SELECT skill_id, source FROM $success
    );
    
    $skill_ids = ( 
        SELECT DISTINCT
            skill_id
            , source
        FROM 
            $all_skill_ids 
        WHERE 
            skill_id IS NOT NULL 
            AND source IS NOT NULL
    );
    
    UPSERT INTO stat.`VoiceTech/external_skills/skill_quality/bass_logs/daily` 
    SELECT
        $date AS fielddate
        , Unwrap(skill_ids.skill_id) AS skill_id
        , Unwrap(skill_ids.source) AS source
        , COALESCE(timeouts.count, 0) AS request_error_timeout
        , COALESCE(http_errors.count, 0) AS request_error_http
        , COALESCE(parse_errors.count, 0) AS request_error_parse
        , COALESCE(validation_errors.count, 0) AS request_error_responsevalidation
        , COALESCE(success.count, 0) AS request_success
        , COALESCE(skills.name, '') AS name
    FROM
        $skill_ids AS skill_ids
    LEFT JOIN
        $timeouts AS timeouts
        ON skill_ids.skill_id == timeouts.skill_id
        AND skill_ids.source == timeouts.source
    LEFT JOIN
        $http_errors AS http_errors
        ON skill_ids.skill_id = http_errors.skill_id
        AND skill_ids.source == http_errors.source
    LEFT JOIN
        $parse_errors AS parse_errors
        ON skill_ids.skill_id = parse_errors.skill_id
        AND skill_ids.source == parse_errors.source
    LEFT JOIN
        $validation_errors AS validation_errors
        ON skill_ids.skill_id = validation_errors.skill_id
        AND skill_ids.source == validation_errors.source
    LEFT JOIN
        $success AS success
        ON skill_ids.skill_id = success.skill_id
        AND skill_ids.source == success.source
    LEFT JOIN
        `home/paskills/skills/stable` AS skills
        ON skill_ids.skill_id = skills.id
    ;

END DEFINE;

EVALUATE FOR $date IN $dates
DO $build_report($date)
;
