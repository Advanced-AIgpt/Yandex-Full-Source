select count(*) from Devices where user_id is null;

update Devices on
select UserDevices.user_id as user_id, UserDevices.device_id as id
from Devices
join UserDevices on UserDevices.device_id = Devices.id
where Devices.user_id is null
limit 10;
