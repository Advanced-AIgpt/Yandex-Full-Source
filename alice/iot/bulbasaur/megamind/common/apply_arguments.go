package common

import (
	"google.golang.org/protobuf/types/known/anypb"

	"a.yandex-team.ru/alice/iot/bulbasaur/protos"
	"a.yandex-team.ru/library/go/core/xerrors"
)

func ExtractApplyArguments(arguments *anypb.Any) (*protos.TApplyArguments, error) {
	var aaProto protos.TApplyArguments
	if err := arguments.UnmarshalTo(&aaProto); err != nil {
		return nil, xerrors.Errorf("failed to unmarshall apply arguments: %w", err)
	}
	return &aaProto, nil
}
