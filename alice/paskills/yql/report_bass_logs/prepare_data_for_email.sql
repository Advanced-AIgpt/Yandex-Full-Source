USE hahn;

SELECT
    ban.skillId AS skill_id
    , skills.name AS skill_name
    , ban.action AS action
    , ban.source AS source
    , ban.ping_error AS ping_error
    , ban.ping_success AS ping_success
    , ban.error_rate AS error_rate
FROM
	`//home/paskills/skill_quality/stable/actions/latest` AS ban
LEFT JOIN
	`//home/paskills/skills/stable` AS skills
	ON ban.skillId == skills.id
WHERE
    ban.action == 'stop'
;
