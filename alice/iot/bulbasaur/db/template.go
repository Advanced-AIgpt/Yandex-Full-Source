package db

import "text/template"

var SelectUserDevicesSimpleTemplate = template.Must(template.New("sudst").Parse(`
	--!syntax_v1
	PRAGMA TablePathPrefix("%s");
	DECLARE $huid AS Uint64;
{{- if .DeviceID}}
	DECLARE $device_id AS String;
{{- end}}
{{- if .SkillID}}
	DECLARE $skill_id AS String;
{{- end}}
{{- if .HouseholdID}}
	DECLARE $household_id AS String;
{{- end}}
{{- if .RoomID}}
	DECLARE $room_id AS String;
{{- end}}

	SELECT
		id,
		name,
		aliases,
		external_id,
		external_name,
		type,
		original_type,
		skill_id,
		capabilities,
		properties,
		custom_data,
		device_info,
		room_id,
		updated,
		created,
		household_id,
		status,
		internal_config
	FROM
		Devices
	WHERE
		huid == $huid AND
{{- if .DeviceID}}
		id == $device_id AND
{{- end}}
{{- if .SkillID}}
		skill_id == $skill_id AND
{{- end}}
{{- if .HouseholdID}}
		household_id == $household_id AND
{{- end}}
{{- if .RoomID}}
		room_id == $room_id AND
{{- end}}
{{ if .Archived}}
		archived == true;
{{- else}}
		archived == false;
{{- end}}
`))

var SelectUserDevicesTemplate = template.Must(template.New("sudt").Parse(`
	--!syntax_v1
	PRAGMA TablePathPrefix("%s");
	DECLARE $huid AS Uint64;
{{- if .DeviceID}}
	DECLARE $device_id AS String;
{{- end}}
{{- if .SkillID}}
	DECLARE $skill_id AS String;
{{- end}}
{{- if .GroupID}}
	DECLARE $group_id AS String;
{{- end -}}
{{- if .HouseholdID}}
	DECLARE $household_id AS String;
{{- end}}
{{- if .RoomID}}
	DECLARE $room_id AS String;
{{- end}}

	SELECT
		SOME(d.id) as id,
		SOME(d.name),
		SOME(d.aliases),
		SOME(d.external_id),
		SOME(d.external_name),
		SOME(d.type),
		SOME(d.original_type),
		SOME(d.skill_id),
		SOME(r.id),
		SOME(r.name),
		SOME(d.capabilities),
		SOME(d.properties),
		SOME(d.custom_data),
		AGGREGATE_LIST(IF(g.id IS NOT NULL,AsStruct(
			g.id as id,
			g.name as name,
			g.aliases as aliases,
			g.type as type
		))),
		SOME(d.device_info),
		SOME(d.updated),
		SOME(d.created),
		SOME(d.household_id),
		SOME(d.status),
		SOME(f.target_id),
		SOME(d.internal_config)
	FROM
		Devices AS d
	LEFT JOIN
		(SELECT * FROM Rooms WHERE Rooms.archived = false) AS r
		ON
			d.huid = r.huid AND
			d.room_id = r.id
	LEFT JOIN
		DeviceGroups AS dg
		ON
			d.huid = dg.huid AND
			d.id = dg.device_id
	LEFT JOIN
		(SELECT * FROM Groups WHERE Groups.archived = false) AS g
		ON
			dg.huid = g.huid AND
			dg.group_id = g.id
	LEFT JOIN
		(SELECT * FROM Favorites WHERE Favorites.type = "device") AS f
		ON
			d.huid = f.huid AND
			d.id = f.target_id
	WHERE
		d.huid = $huid AND
{{- if .DeviceID}}
		d.id = $device_id AND
{{- end}}
{{- if .SkillID}}
		d.skill_id == $skill_id AND
{{- end}}
{{- if .HouseholdID}}
		d.household_id == $household_id AND
{{- end}}
{{- if .RoomID}}
		d.room_id == $room_id AND
{{- end}}
		d.archived = false
	GROUP BY
		d.id
{{- if .GroupID}}
	HAVING
		COUNT_IF(g.id = $group_id) > 0
{{- end -}}
;
`))

var SelectUserDeviceGroupsTemplate = template.Must(template.New("sudgt").Parse(`
	--!syntax_v1
	PRAGMA TablePathPrefix("%s");
	DECLARE $huid AS Uint64;
{{- if .DeviceID}}
	DECLARE $device_id AS String;
{{- end -}}
{{- if .GroupID}}
	DECLARE $group_id AS String;
{{- end -}}

	SELECT
		device_id,
		group_id
	FROM
		DeviceGroups
	WHERE
		huid == $huid
{{- if .DeviceID}} AND
		device_id == $device_id
{{- end -}}
{{- if .GroupID}} AND
		group_id == $group_id
{{- end -}}
;
`))

var SelectUserGroupsSimpleTemplate = template.Must(template.New("sugst").Parse(`
	--!syntax_v1
	PRAGMA TablePathPrefix("%s");
	DECLARE $huid AS Uint64;
{{- if .GroupID}}
	DECLARE $group_id AS String;
{{- end}}
{{- if .HouseholdID}}
	DECLARE $household_id AS String;
{{- end}}

	SELECT
		id,
		name,
		aliases,
		type,
		household_id
	FROM
		Groups
	WHERE
		huid == $huid AND
{{- if .GroupID}}
		id == $group_id AND
{{- end}}
{{- if .HouseholdID}}
		household_id == $household_id AND
{{- end}}
		archived == false;
`))

var SelectUserGroupsTemplate = template.Must(template.New("sugt").Parse(`
	--!syntax_v1
	PRAGMA TablePathPrefix("%s");
	DECLARE $huid AS Uint64;
	DECLARE $group_id AS String;
	DECLARE $household_id AS String;

	$group_id_with_devices=(SELECT
		Groups.id as group_id,
		AGGREGATE_LIST(gd.device_id) as devices
	FROM
		Groups
	LEFT JOIN
		(SELECT * FROM DeviceGroups WHERE DeviceGroups.huid == $huid) AS gd ON gd.huid == Groups.huid AND gd.group_id == Groups.id
	WHERE
		{{with .GroupID -}}
			Groups.id == $group_id AND
		{{end}}
		Groups.huid == $huid AND
		Groups.archived == false
	GROUP BY
		Groups.id);

	SELECT
		Groups.id as id,
		Groups.name as name,
		Groups.aliases as aliases,
		Groups.type as type,
		gd.devices as devices,
		Groups.household_id as household_id,
		f.target_id
	FROM
		Groups
	JOIN
		$group_id_with_devices AS gd ON gd.group_id == Groups.id
	LEFT JOIN
		(SELECT * FROM Favorites WHERE Favorites.huid = $huid AND Favorites.type == "group") AS f
		ON
			Groups.huid = f.huid AND
			Groups.id = f.target_id
	WHERE
		{{with .GroupID -}}
			Groups.id == $group_id AND
		{{end}}
		{{with .HouseholdID -}}
			Groups.household_id == $household_id AND
		{{end}}
		Groups.huid == $huid AND
		Groups.archived == false;
`))

var SelectUserRoomsSimpleTemplate = template.Must(template.New("surst").Parse(`
	--!syntax_v1
	PRAGMA TablePathPrefix("%s");
	DECLARE $huid AS Uint64;
{{- if .RoomID}}
	DECLARE $room_id AS String;
{{- end}}
{{- if .HouseholdID}}
	DECLARE $household_id AS String;
{{- end}}

	SELECT
		id,
		name,
		household_id
	FROM
		Rooms
	WHERE
		huid == $huid AND
{{- if .RoomID}}
		id == $room_id AND
{{- end}}
{{- if .HouseholdID}}
		household_id == $household_id AND
{{- end}}
		archived == false;
`))

var SelectUserRoomTemplate = template.Must(template.New("surt").Parse(`
	--!syntax_v1
	PRAGMA TablePathPrefix("%s");
	DECLARE $huid AS Uint64;
	DECLARE $room_id AS String;
	DECLARE $household_id AS String;

	$rooms_devices = (
		SELECT
			Devices.room_id as room_id,
			AGGREGATE_LIST(id) AS devices
		FROM
			Devices
		WHERE
			Devices.huid == $huid AND
			Devices.archived == false
		GROUP BY
			Devices.room_id
	);

	SELECT
		id,
		name,
		NVL(rd.devices, ListCreate(List<String>?)) as devices,
		household_id
	FROM
		Rooms
	LEFT JOIN
		$rooms_devices as rd ON rd.room_id == Rooms.id
	WHERE
		{{with .RoomID -}}
		Rooms.id == $room_id AND
		{{end}}
		{{with .HouseholdID -}}
		Rooms.household_id == $household_id AND
		{{end}}
		Rooms.huid == $huid AND
		Rooms.archived == false;
`))

var SelectUserScenariosTemplate = template.Must(template.New("sust").Parse(`
	--!syntax_v1
	PRAGMA TablePathPrefix("%s");
	DECLARE $huid AS Uint64;
{{- if .ScenarioID}}
	DECLARE $scenario_id AS String;
{{- end}}
{{- if .IsActive}}
	DECLARE $is_active AS Bool;
{{- end}}

	SELECT
		s.id as id,
		s.name,
		s.icon,
		s.triggers,
		s.devices,
		s.requested_speaker_capabilities,
		s.steps,
		s.is_active,
		s.effective_time,
		s.push_on_invoke,
		f.target_id
	FROM Scenarios as s
	LEFT JOIN
		(SELECT * FROM Favorites WHERE Favorites.huid = $huid AND Favorites.type == "scenario") AS f
		ON
			s.huid = f.huid AND
			s.id = f.target_id
	WHERE
{{- if .ScenarioID}}
		s.id == $scenario_id AND
{{- end}}
{{- if .IsActive}}
		s.is_active == $is_active AND
{{- end}}
		s.huid == $huid AND
		s.archived == false;
`))

var SelectUserScenariosSimpleTemplate = template.Must(template.New("susst").Parse(`
	--!syntax_v1
	PRAGMA TablePathPrefix("%s");
	DECLARE $huid AS Uint64;
{{- if .ScenarioID}}
	DECLARE $scenario_id AS String;
{{- end}}
{{- if .IsActive}}
	DECLARE $is_active AS Bool;
{{- end}}

	SELECT
		s.id as id,
		s.name,
		s.icon,
		s.triggers,
		s.devices,
		s.requested_speaker_capabilities,
		s.steps,
		s.is_active,
		s.effective_time,
		s.push_on_invoke
	FROM Scenarios as s
	WHERE
{{- if .ScenarioID}}
		s.id == $scenario_id AND
{{- end}}
{{- if .IsActive}}
		s.is_active == $is_active AND
{{- end}}
		s.huid == $huid AND
		s.archived == false;
`))

var SelectUserNetworksTemplate = template.Must(template.New("sunt").Parse(`
	--!syntax_v1
	PRAGMA TablePathPrefix("%s");
	DECLARE $huid AS Uint64;
{{- if .SSID}}
	DECLARE $ssid AS String;
{{- end}}

	SELECT
		ssid,
		password,
		updated
	FROM
		UserNetworks
	WHERE
		huid == $huid AND
{{- if .SSID}}
		ssid == $ssid AND
{{- end}}
		archived == false;
`))

var SelectUserHouseholdsTemplate = template.Must(template.New("suht").Parse(`
	--!syntax_v1
	PRAGMA TablePathPrefix("%s");
	DECLARE $huid AS Uint64;
{{- if .HouseholdID}}
	DECLARE $household_id AS String;
{{- end}}

	SELECT
		id,
		name,
		latitude,
		longitude,
		address,
		short_address
	FROM
		Households
	WHERE
		huid == $huid AND
{{- if .HouseholdID}}
		id == $household_id AND
{{- end}}
		archived == false;
`))

var SelectSharedHouseholdsInvitationsTemplate = template.Must(template.New("sshit").Parse(`
	DECLARE $guest_huid AS UInt64;
	DECLARE $guest_id AS Uint64;
{{- if .HouseholdID}}
	DECLARE $household_id AS String;
{{- end}}
	SELECT
		id, sender_id, household_id, $guest_id AS guest_id
	FROM
		SharedHouseholdsInvitations
	WHERE
		guest_huid == $guest_huid
{{- if .HouseholdID}}
		AND household_id == $household_id
{{- end}}
		;
`))
