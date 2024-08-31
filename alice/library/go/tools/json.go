package tools

import (
	"bytes"
	"encoding/json"
)

func PrettyJSONMessage(prefix, indent string, message json.RawMessage) string {
	var buf bytes.Buffer
	if err := json.Indent(&buf, message, prefix, indent); err != nil {
		return string(message)
	}
	return buf.String()
}
