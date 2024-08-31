package common

import (
	"a.yandex-team.ru/alice/iot/bulbasaur/protos"
	"a.yandex-team.ru/library/go/core/xerrors"
	"google.golang.org/protobuf/types/known/anypb"
)

func ExtractContinueArguments(arguments *anypb.Any) (*protos.TContinueArguments, error) {
	var caProto protos.TContinueArguments
	if err := arguments.UnmarshalTo(&caProto); err != nil {
		return nil, xerrors.Errorf("failed to unmarshall continue arguments: %w", err)
	}
	return &caProto, nil
}
