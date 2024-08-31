package sdk

import (
	"encoding/json"

	"a.yandex-team.ru/alice/iot/bulbasaur/megamind/common"
	"a.yandex-team.ru/alice/iot/bulbasaur/protos"
	"google.golang.org/protobuf/types/known/anypb"
)

// Note: wrappers of raw protobuf structs are required for correct json-marshalling.
// Also ensure that all fields of your implementation can be correctly unmarshalled from JSON.
type UniversalContinueArguments interface {
	ProcessorName() string
	IsUniversalContinueArguments()
}

func IsContinueArguments(any *anypb.Any, r UniversalContinueArguments) bool {
	args, err := common.ExtractContinueArguments(any)
	if err != nil {
		return false
	}
	return r.ProcessorName() == args.GetUniversalContinueArguments().GetProcessorName()
}

func UnmarshalContinueArguments(any *anypb.Any, r UniversalContinueArguments) error {
	args, err := common.ExtractContinueArguments(any)
	if err != nil {
		return err
	}
	return json.Unmarshal(args.GetUniversalContinueArguments().GetPayloadJSON(), r)
}

func MarshalContinueArguments(r UniversalContinueArguments) (*anypb.Any, error) {
	payloadJSON, err := json.Marshal(r)
	if err != nil {
		return nil, err
	}
	args := &protos.TContinueArguments{
		Value: &protos.TContinueArguments_UniversalContinueArguments{
			UniversalContinueArguments: &protos.UniversalContinueArguments{
				ProcessorName: r.ProcessorName(),
				PayloadJSON:   payloadJSON,
			},
		},
	}
	return anypb.New(args)
}
