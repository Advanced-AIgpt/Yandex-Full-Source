select count(*) from Scenarios where user_id is null;

update Scenarios on
select UserScenarios.user_id as user_id, UserScenarios.scenario_id as id
from Scenarios
join UserScenarios on UserScenarios.scenario_id = Scenarios.id
where Scenarios.user_id is null
limit 10;
