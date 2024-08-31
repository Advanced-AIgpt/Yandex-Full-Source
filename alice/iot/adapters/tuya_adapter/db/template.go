package db

import (
	"text/template"
)

var selectCustomControlsTemplate = template.Must(template.New("scct").Parse(`
	--!syntax_v1
	PRAGMA TablePathPrefix("%s");
{{- if .UserID}}
	DECLARE $huid AS Uint64;
{{- end}}
{{- if .DeviceID}}
	DECLARE $id AS String;
{{- end}}

	SELECT
		id,
		name,
		device_type,
		buttons
	FROM
		CustomControls
	WHERE
{{- if .UserID}}
		huid = $huid AND
{{- end}}
{{- if .DeviceID}}
		id = $id AND
{{- end}}
		archived = false;
`))
