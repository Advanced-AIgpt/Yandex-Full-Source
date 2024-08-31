package tools

import (
	"encoding/json"
	"fmt"

	"google.golang.org/protobuf/encoding/protojson"
	"google.golang.org/protobuf/proto"

	"a.yandex-team.ru/library/go/core/log"
)

func ProtoJSONLogField(key string, m proto.Message) log.Field {
	jsonBytes, err := protojson.Marshal(m)
	if err != nil {
		return log.String(key, fmt.Sprintf("unable to marshal proto message to json: %v", err))
	}
	return log.Any(key, json.RawMessage(jsonBytes))
}
