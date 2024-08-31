use hahn;
PRAGMA ClassicDivision = 'false';
PRAGMA yt.InferSchema = '1';
PRAGMA AnsiInForEmptyOrNullableItemsCollections;


$dialogovo_dir = 'home/logfeller/logs/dialogovo-prod-diagnostic-info-log/1d';
$latest_log_table_date = SELECT unwrap(cast(substring(MAX(Path),Length($dialogovo_dir)+1u) as Date)) FROM FOLDER($dialogovo_dir);
$from_log_table_date = $latest_log_table_date - Interval ("P6D");

$is_error = ($error) -> {
    return coalesce($error, '') != '';
};

$public_skills = (
    SELECT
        id
        , name
        , channel
        , category
    FROM
        `home/paskills/skills/stable`
    WHERE
        channel in (
            'aliceSkill',
            'smartHome'
        )
        AND deletedAt IS NULL
        AND onAir = true
        AND hideInStore = false
);

$dialogovo_logs = (
    SELECT
        skill_id
        , session_id
        , source
        ,$is_error(errors) as is_err_bool
        , IF($is_error(errors), 1, 0) AS is_error
    FROM
        RANGE($dialogovo_dir, cast($from_log_table_date as String), cast($latest_log_table_date as String))
);

$stability_stats = select skill_id as id,
1-nanvl(count_if(is_error_user_session)/count_if(user_session), 0.0) as session_success_rate,
1-nanvl(sum(success_user)/sum(all_user), 0.0) as user_success_rate,
1-nanvl(sum(success_ping)/sum(all_ping), 0.0) as ping_success_rate,
from(
    SELECT
        skill_id
        , session_id
        ,min(source) == 'user' as user_session
        ,min(source) == 'user' and bool_or(is_err_bool) AS is_error_user_session
        ,count_if(source == 'ping' and is_err_bool) as success_ping
        ,count_if(source == 'ping') as all_ping
        ,count_if(source == 'user' and is_err_bool) as success_user
        ,count_if(source == 'user') as all_user
        ,count(*) as reqs_cnt
    FROM
        $dialogovo_logs
        group compact by skill_id, session_id
)
group compact by skill_id;
--select * from $stability_stats;

$ratings = (
    SELECT
        skill_id
        , avg(rating) as rating_avg
        , count(*) as rating_count
    FROM hahn.`home/paskills/reviews/stable`
    GROUP compact BY skillId as skill_id
);

$retention_limit = 0.5;
$timespent_limit = 700.0;

$stat = (
    SELECT
        s.skill_id as id
        , s.wau as wau
        , NANVL(Math::Log(cast(s.wau as Double)), 0.0) as log_wau
        , s.mau as mau
        , s.avg_sessions_tpt_duration_w as tpt_duration_w
        , NANVL(Math::Log(cast(s.mau as Double)), 0.0) as log_mau
        , IF(s.dau >= 50 OR s.mau > 300, s.retention_d28_1w_avg, 0) AS retention
        , NANVL(IF(b.rating_count >= 30, b.rating_avg, 0.5 * b.rating_avg), 0.0) AS rating_avg
        , NANVL(Math::Log(b.rating_count), 0.0) as rating_count_log
    FROM `home/paskills/stat/dialogovo_stat_current` as s
    left join $ratings as b on s.skill_id = b.skill_id
    where s.app_group = '_total_'
    and s.skill_id != '_total_'
    and s.app = '_total_'
);

$stat_min_max = (
    SELECT AsStruct(
        MIN(log_wau) as log_wau_min
        , MAX(log_wau) as log_wau_max
        , MIN(log_mau) as log_mau_min
        , MAX(log_mau) as log_mau_max
        , min(tpt_duration_w) as tpt_duration_w_min
        , min_of(max(tpt_duration_w),$timespent_limit) as tpt_duration_w_max
        , MIN(retention) AS retention_min
        , min_of(MAX(retention), $retention_limit) as retention_max
        , MIN(rating_avg) AS rating_avg_min
        , MAX(rating_avg) AS rating_avg_max
        , MIN(rating_count_log) AS rating_count_log_min
        , MAX(rating_count_log) AS rating_count_log_max
    ) FROM
        $stat
);

$stat_scaled = (
    SELECT
        id
        , min_of((retention - $stat_min_max.retention_min) / ($stat_min_max.retention_max - $stat_min_max.retention_min),1) AS retention_scaled
        , (log_wau - $stat_min_max.log_wau_min) / ($stat_min_max.log_wau_max - $stat_min_max.log_wau_min) as log_wau_scaled
        , (log_mau - $stat_min_max.log_mau_min) / ($stat_min_max.log_mau_max - $stat_min_max.log_mau_min) as log_mau_scaled
        , (rating_avg - $stat_min_max.rating_avg_min) / ($stat_min_max.rating_avg_max - $stat_min_max.rating_avg_min) as rating_avg_scaled
        , (rating_count_log - $stat_min_max.rating_count_log_min) / ($stat_min_max.rating_count_log_max - $stat_min_max.rating_count_log_min) as rating_count_log_scaled
        , min_of((tpt_duration_w - $stat_min_max.tpt_duration_w_min) / ($stat_min_max.tpt_duration_w_max - $stat_min_max.tpt_duration_w_min),1) as tpt_duration_w_scaled
        , TableRow() as factors
    FROM
        $stat
);

$alice_prize_winners = (
    SELECT id as skill_id, 1 as winner_score from `//home/paskills/upload/stable/skills`
    with schema Struct<alicePrizeRecievedAt: Optional<String>, id: String> where `alicePrizeRecievedAt` is not null
);

select stat.id, skills.name, log_wau_scaled, log_mau_scaled, retention_scaled, rating_avg_scaled, rating_count_log_scaled, tpt_duration_w_scaled,
coalesce(winners.winner_score, 0) as winner_score,
COALESCE(skills_stats.ping_success_rate, 1) as ping_success_rate,
COALESCE(skills_stats.user_success_rate, 1) as user_success_rate,
COALESCE(skills_stats.session_success_rate, 1) as session_success_rate,
FROM $public_skills AS skills
join $stat_scaled as stat on skills.id = stat.id
LEFT JOIN $alice_prize_winners as winners
ON stat.id = winners.skill_id
LEFT JOIN
    $stability_stats AS skills_stats
    ON skills_stats.id == stat.id
where skills.channel = 'aliceSkill'
    ;

$stat_scores = (
    SELECT
        id
        , (
            0.4 * log_wau_scaled +
            --0.4 * log_mau_scaled +
            0.7 * retention_scaled +
            0.5 * rating_avg_scaled * (rating_count_log_scaled/2.0+0.5) +
            0.7 * tpt_duration_w_scaled +
            --0.7 * rating_count_log_scaled +
            0.1 * coalesce(winners.winner_score, 0)
        ) AS score
        , ExpandStruct(
            factors
            , retention_scaled as retention_scaled
            , log_wau_scaled as log_wau_scaled
            , log_mau_scaled as log_mau_scaled
            , rating_avg_scaled as rating_avg_scaled
            , (rating_count_log_scaled/2.0+0.5) as rating_count_log_scaled
            , tpt_duration_w_scaled as tpt_duration_w_scaled
        ) as factors
        , AsStruct(
            0.4 * log_wau_scaled as log_wau_score,
            --0.4 * log_mau_scaled as log_mau_score,
            0.7 * retention_scaled as retention_score,
            0.5 * rating_avg_scaled * (rating_count_log_scaled/2.0+0.5) as rating_avg_score,
            0.7 * rating_count_log_scaled as rating_count_score,
            0.6 * tpt_duration_w_scaled as tpt_duration_w_scaled,
            0.1 * coalesce(winners.winner_score, 0) as winner_score
        ) as weighted_factors
    FROM
        $stat_scaled as stat
    LEFT JOIN $alice_prize_winners as winners
    ON stat.id = winners.skill_id
);

$score_boost = AsDict();

$scores = (
    SELECT
        skills.id AS id
        , skills.name as name
        , skills.channel as channel
        , COALESCE(stat_scores.score, 0) * COALESCE(skills_stats.ping_success_rate, 1) * COALESCE(skills_stats.user_success_rate, 1) * COALESCE(skills_stats.session_success_rate, 1) AS score
        , ExpandStruct(
            stat_scores.factors
            , COALESCE(skills_stats.ping_success_rate, 1) as ping_success_rate
            , COALESCE(skills_stats.user_success_rate, 1) as user_success_rate
            , COALESCE(skills_stats.session_success_rate, 1) as session_success_rate
            , COALESCE($score_boost[skills.id], 0.0) as score_boost
        ) as factors
        , weighted_factors
    FROM $public_skills AS skills
    LEFT JOIN
        $stat_scores AS stat_scores
        ON stat_scores.id == skills.id
    LEFT JOIN
        $stability_stats AS skills_stats
        ON skills_stats.id == skills.id

);

$score_min_max = (
    SELECT AsStruct(
        MIN(score) AS min_score
        , MAX(score) AS max_score
    ) FROM $scores
);

$result = SELECT
    id
    , name
    , case channel
        when 'smartHome' then 2.0
        else 1.0
    end + (score - $score_min_max.min_score) / ($score_min_max.max_score - $score_min_max.min_score) + COALESCE($score_boost[id], 0.0) AS score
    , factors
    , weighted_factors
    , s.channel as channel
FROM
    $scores AS s
;

INSERT INTO `home/paskills/store/stable/scores` WITH TRUNCATE
--discard
select
    id
    , name
    , score
    , factors
    , weighted_factors
    , channel
from $result
ORDER BY
    channel,
    score DESC ;

$today = CurrentUtcDate();
$prev_score_by_day = select * from `home/paskills/store/stable/scores_by_day` where fielddate != $today;

INSERT INTO `home/paskills/store/stable/scores_by_day` WITH TRUNCATE
--discard
select unwrap($today) as fielddate
    , id
    , name
    , score
    , factors
    , weighted_factors
    , channel
from $result
union all
select unwrap($today) as fielddate
    , id
    , name
    , score
    , factors
    , weighted_factors
    , channel
from $prev_score_by_day;


-- VALIDATION: try to find atl least 50 skills with scores larger than 1

$skills = (
    select
        scores.id as id
        , scores.name as name
        , score
        , channel
    --from `home/paskills/store/stable/scores_new` as scores
    from `home/paskills/store/stable/scores` as scores
    where channel == 'aliceSkill'
);

DISCARD select Ensure(count(*), count(*) >= 50, 'Failed to find 50 skills with non default score')
from $skills where score > 1.01;

DISCARD select Ensure(null, count(*)>1, 'Duplicates by skill_id ['||id||'] found: '||cast(count(*) as String))
from `home/paskills/store/stable/scores` group compact by id having count(*) > 1;

DISCARD select id, fielddate, Ensure(null, count(*)>1, 'Duplicates by skill_id ['||id||'] at ['||cast(fielddate as String)||'] found: '||cast(count(*) as String))
from `home/paskills/store/stable/scores_by_day` group compact by id, fielddate having count(*) > 1;
