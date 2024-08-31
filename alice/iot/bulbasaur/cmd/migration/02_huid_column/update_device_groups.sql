select count(*) from DeviceGroups where user_id is null;

update DeviceGroups on
select UserDevices.user_id as user_id, UserDevices.device_id as device_id, DeviceGroups.group_id as group_id
from DeviceGroups
join UserDevices on UserDevices.device_id = DeviceGroups.device_id
where DeviceGroups.user_id is null
limit 10;
