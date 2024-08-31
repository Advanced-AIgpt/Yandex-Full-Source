USE hahn;
PRAGMA ClassicDivision = 'false';
PRAGMA yt.InferSchema = '100';

$today = CAST(CurrentUtcDatetime() AS Date);
$actions_dir = 'home/paskills/skill_quality/stable/actions';
$today_table = $actions_dir || '/' || CAST($today AS String);
$yesterday = $today - Interval("P1D");
$dialogovo_logs_dir = '//home/logfeller/logs/dialogovo-prod-diagnostic-info-log/1d';
$min_pings_limit = 500;

$ACTION_STOP = 'stop';
$ACTION_WARN = 'warn';
$ACTION_RESTORE_IS_RECOMMENDED = 'restoreIsRecommended';

$SOURCE_PING = 'ping';

$is_error = ($error) -> {
    return coalesce($error, '') != '';
};


$logs_table = (
    SELECT
        AsList(Unwrap(MAX(Path)))
    FROM
        FOLDER($dialogovo_logs_dir)
);

$ping_data = (
    FROM EACH($logs_table)
    SELECT
        skill_id
        , SUM(IF($is_error(errors), 1, 0)) as error_count
        , COUNT(*) AS total
    WHERE
        source = $SOURCE_PING
    GROUP BY
        skill_id
);

$alerts = (
    FROM `home/paskills/upload/stable/emailAlerts`
    SELECT *
    WHERE
        createdAt >= CAST($yesterday AS String)
);

$skills_actions = (
    SELECT
        skill_id AS skillId
        , IF(alerts.skillId is null, $ACTION_WARN, $ACTION_STOP) as `action`
        , $SOURCE_PING as source
        , ping_data.error_count AS ping_error
        , ping_data.total - ping_data.error_count AS ping_success
        , ping_data.error_count / ping_data.total AS error_rate
    FROM
        $ping_data AS ping_data
    JOIN
        `home/paskills/skills/stable` AS skills
        ON skills.id == ping_data.skill_id
    LEFT JOIN
        $alerts AS alerts
        ON alerts.skillId == ping_data.skill_id
    WHERE
        ping_data.total == ping_data.error_count
        AND ping_data.total >= $min_pings_limit
        AND skills.hideInStore = false
        AND skills.onAir = true
        AND skills.channel = 'aliceSkill'
);

$skill_actions_rn = (
    SELECT
        skillId
        , action
        , source
        , ping_error
        , ping_success
        , error_rate
        , row_number() over w as rn
    FROM
        $skills_actions
    WINDOW w AS (PARTITION BY action ORDER BY skillId)
);

-- find skills that were previously banned (warned ot stopped)
-- and restore isRecommended flag if they started answering to pings
$last_7_action_tables = (
    SELECT
        Path AS tbl
    FROM
        FOLDER($actions_dir)
    WHERE
        Path != $actions_dir || '/latest'
    ORDER BY tbl DESC
    LIMIT 7
);

$last_7_action_tables_list = (
    SELECT
        AGGREGATE_LIST(tbl) AS tables
    FROM
        $last_7_action_tables
);

$action_history = (
    SELECT
        TableName() as `date`
        , t.skillId as skillId
        , t.`action` as `action`
    FROM
        EACH($last_7_action_tables_list) AS t
);

$action_history_rn = (
    SELECT
        `date`
        , skillId
        , `action`
        , ROW_NUMBER() OVER w as rn
    FROM
        $action_history
    WINDOW w AS (PARTITION BY skillId ORDER BY `date` DESC)
);

$currently_banned_skills = (
    SELECT
        `date`
        , skillId
        , action
    FROM
        $action_history_rn
    WHERE
        rn = 1
        AND action != $ACTION_RESTORE_IS_RECOMMENDED
);

$skills_to_restore = (
    SELECT
        skill_id AS skillId
        , $ACTION_RESTORE_IS_RECOMMENDED as `action`
        , $SOURCE_PING as source
        , ping_data.error_count AS ping_error
        , ping_data.total - ping_data.error_count AS ping_success
        , ping_data.error_count / ping_data.total AS error_rate
    FROM
        $ping_data AS ping_data
    INNER JOIN
        $currently_banned_skills AS banned
        ON banned.skillId == ping_data.skill_id
    WHERE
        ping_data.total > $min_pings_limit
        AND ping_data.error_count / ping_data.total < 0.1
);

-- prepare skill actions table – decide which skills should be warned, banned or restored
$skills_actions_limited = (
    SELECT
        skillId
        , IF(action == $ACTION_STOP AND rn <= 5, action, $ACTION_WARN) AS action
        , source
        , ping_error
        , ping_success
        , error_rate
    FROM
        $skill_actions_rn
    UNION ALL
    SELECT
        skillId
        , `action`
        , source
        , ping_error
        , ping_success
        , error_rate
    FROM
        $skills_to_restore
);


-- write output tables
INSERT INTO `home/paskills/skill_quality/stable/actions/latest` WITH TRUNCATE
SELECT
    *
FROM $skills_actions_limited
;


INSERT INTO $today_table WITH TRUNCATE
SELECT
    *
FROM $skills_actions_limited
;