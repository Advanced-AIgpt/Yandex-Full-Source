use hahn;
PRAGMA yt.InferSchema = '100';
pragma yson.DisableStrict = 'true';
pragma yt.Pool='paskills';
PRAGMA AnsiInForEmptyOrNullableItemsCollections;

$today = CurrentUtcDate();
$four_weeks_ago = $today - Interval("P28D");

$get_year_month = ($str) -> {
    $splitted = String::SplitToList($str, '-', true, false, 2);
    $year = $splitted[0];
    $month = $splitted[1];
    
    return String::JoinFromList(aslist($year, $month), '-')
};


$skills = (
    select 
        id
        , name
        , slug
        , category
        , developerType
        , $get_year_month(alicePrizeRecievedAt) as alicePrizeRecievedAtDate
        , alicePrizeRecievedAt is not null as alicePriceWinner
    from
        `home/paskills/skills/stable` 
    where 
        channel = 'aliceSkill'
        and hideInStore = false
        and onAir = true
        and deletedAt is null
        and isRecommended = true
        and Yson::Contains(publishingSettings, 'explicitContent')
        and Yson::ConvertToBool(publishingSettings.explicitContent) = false
);

$reviews = (
    SELECT
        skillId as skill_id
        , AVG(rating) AS average_rating
    FROM
        `home/paskills/reviews/stable`
    GROUP BY
        skillId
);

-- навыки сначал публикуются как приватные, а только потом как публичные.
-- поэтому нужно брать только деплои после первого аппрува, но если его не было то просто минимальный деплой
$first_deploy = select itemId as skill_id,
    min(if(createdAt > firstApprovedDate, createdAt)) as firstPublicDeployDate,
    min(firstApprovedDate) as firstApprovedDate,
from (
    -- Ищем дату первого аппрува от модератора
    select o.itemId as itemId,
    o.type as type,
    o.createdAt as createdAt,
    coalesce(min(if(o.type == 'reviewApproved', o.createdAt)) over w, '1900-01-01') as firstApprovedDate,
    from $skills  as s
    join `home/paskills/upload/stable/operations` with schema Struct<itemId: String?, type: String?, createdAt: String?> as o on o.itemId = s.id
    window w as (partition compact by o.itemId)
)
where type = 'deployCompleted'
group compact by itemId;


$dau_avg = select unwrap(max(dau_1w_avg)) as avg_dau, unwrap(max(wau)) as wau, skill_id 
    from `home/paskills/stat/dialogovo_stat_current` as s
    where app_group = '_total_' and app = '_total_' and skill_id != '_total_' and category != '_total_' and dev_type != '_total_'
    -- Если навык в течении дня менял категорию, то он фиигурирует дважды. Будет исправлено в PASKILLS-6246
    group by skill_id;

$skills_summary = 
select s.*, average_rating, wau, avg_dau, firstPublicDeployDate, firstApprovedDate,
from $skills as s
left join $reviews as r on s.id = r.skill_id
left join $first_deploy as dep on s.id = dep.skill_id
left join $dau_avg as a on s.id = a.skill_id
;

-- check for duplicates
DISCARD SELECT id,
Ensure(count(*), count(*)==1, 'Duplicates in $skills_summary for skill id = '||id)
from $skills_summary
group by id;

--------------------------------------------------------------------------------------------------------------------------------------------

$new_skills = (
    select
        id,
        name,
        slug,
        avg_dau,
        wau,
    from 
        $skills_summary
    where firstPublicDeployDate >= CAST($four_weeks_ago as String)
    and not alicePriceWinner
    --order by wau desc limit 30
);

--select * from $first_deploy_in_last_4w;
--select * from $new_skills order by wau desc;

$popular_skills = (
    select
        id,
        name,
        slug,
        avg_dau,
        category,
    from
       $skills_summary
    where
        developerType != 'yandex'
        and avg_dau >= 100
        and average_rating > 4
        and not alicePriceWinner
);

$useful_skills = (
    select
        id,
        name,
        slug,
        avg_dau,
        category,
    from
        $skills_summary
    where
        developerType != 'yandex'
        and category not in ('games_trivia_accessories', 'kids', 'education_reference', 'smart_home')
        and not alicePriceWinner
        and average_rating >= 3.0
);

-- собираем подборки. Навык может быть только в одной подборке одновременно
$new_skills_banner = (
    SELECT 
        id,
        slug,
        avg_dau,
        wau,
    FROM $new_skills
    ORDER BY wau DESC
    LIMIT 20
);

--select * from $new_skills_banner;

$popular_skills_banner = (
    SELECT
        id,
        slug,
        avg_dau,
    FROM
        $popular_skills as popular_skills_banner
    LEFT ONLY JOIN
        $new_skills_banner AS new_skills_banner
        ON new_skills_banner.id == popular_skills_banner.id
    ORDER BY 
        avg_dau DESC
    LIMIT 20
);

--select * from $popular_skills_banner;

$useful_skills_banner = (
    SELECT
        id,
        slug,
        avg_dau,
    FROM
        $useful_skills AS useful_skills
    LEFT ONLY JOIN
        $new_skills_banner AS new_skills_banner
        ON new_skills_banner.id == useful_skills.id
    LEFT ONLY JOIN
        $popular_skills_banner AS popular_skills_banner
        ON popular_skills_banner.id == useful_skills.id
    ORDER BY 
        avg_dau DESC
    LIMIT 20
);

--select * from $useful_skills_banner;

$alice_prize_winners_by_quarter = (
    select AGGREGATE_LIST(slug) as slugs, alicePrizeRecievedAtDate 
    from $skills_summary 
    group by alicePrizeRecievedAtDate
    order by alicePrizeRecievedAtDate desc
    limit 3
);

$alice_prize_winners_slugs = (
    select slugs from $alice_prize_winners_by_quarter flatten by slugs
);

--select * from $alice_prize_winners_slugs;

$shuffle = ($list) -> {
    $now_str = CAST(CurrentUtcDatetime() AS String);
    $with_random = ListMap($list, ($item) -> {
        RETURN AsTuple($item, RANDOM($item || $now_str));
    });
    $sorted = ListSort($with_random, ($t) -> { RETURN $t.1; });
    RETURN ListMap($sorted, ($t) -> { RETURN $t.0; });
};

$banners = (
    SELECT 'new' AS id, Ensure($shuffle(AGGREGATE_LIST(slug)), count(*)>15,'Too few skills in "new" banner') AS slugs FROM $new_skills_banner
    UNION ALL
    SELECT 'popular' AS id, Ensure($shuffle(AGGREGATE_LIST(slug)), count(*)>15,'Too few skills in "popular" banner') AS slugs FROM $popular_skills_banner
    UNION ALL
    SELECT 'useful' AS id, Ensure($shuffle(AGGREGATE_LIST(slug)), count(*)>15,'Too few useful in "new" banner') AS slugs FROM $useful_skills_banner
    UNION ALL
    SELECT 'winners' AS id, Ensure($shuffle(AGGREGATE_LIST(slugs)), count(*)>6,'Too few skills in "winners" banner') as slugs FROM $alice_prize_winners_slugs
);

--select * from $banners;

INSERT INTO `home/paskills/store/stable/collections` WITH TRUNCATE
SELECT
    id
    , slugs
FROM
    $banners
;
