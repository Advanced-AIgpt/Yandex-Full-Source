package sdk

import (
	"encoding/json"

	"google.golang.org/protobuf/types/known/anypb"

	"a.yandex-team.ru/alice/iot/bulbasaur/megamind/common"
	"a.yandex-team.ru/alice/iot/bulbasaur/protos"
)

// Note: wrappers of raw protobuf structs are required for correct json-marshalling.
// Also ensure that all fields of your implementation can be correctly unmarshalled from JSON.
type UniversalApplyArguments interface {
	ProcessorName() string
	IsUniversalApplyArguments()
}

func IsApplyArguments(any *anypb.Any, r UniversalApplyArguments) bool {
	args, err := common.ExtractApplyArguments(any)
	if err != nil {
		return false
	}
	return r.ProcessorName() == args.GetUniversalApplyArguments().GetProcessorName()
}

func UnmarshalApplyArguments(any *anypb.Any, r UniversalApplyArguments) error {
	args, err := common.ExtractApplyArguments(any)
	if err != nil {
		return err
	}
	return json.Unmarshal(args.GetUniversalApplyArguments().GetPayloadJSON(), r)
}

func MarshalApplyArguments(r UniversalApplyArguments) (*anypb.Any, error) {
	payloadJSON, err := json.Marshal(r)
	if err != nil {
		return nil, err
	}
	args := &protos.TApplyArguments{
		Value: &protos.TApplyArguments_UniversalApplyArguments{
			UniversalApplyArguments: &protos.UniversalApplyArguments{
				ProcessorName: r.ProcessorName(),
				PayloadJSON:   payloadJSON,
			},
		},
	}
	return anypb.New(args)
}
