package swagger

import _ "embed"

// generate swagger spec via go-generate mechanic
//go:generate ya tool swagger generate spec -w ../ -o swagger.json --scan-models

//go:embed swagger.json
var rawSwaggerJSON []byte

func GetRawJSON() []byte {
	return rawSwaggerJSON
}
