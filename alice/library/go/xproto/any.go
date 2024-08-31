package xproto

import (
	"fmt"

	"google.golang.org/protobuf/types/known/anypb"
)

// MustAny is usually used in combination with anypb.New(protoMessage) ctor
func MustAny(any *anypb.Any, err error) *anypb.Any {
	if err != nil {
		panic(fmt.Sprintf("unexpected error in marshalling any proto message: %v", err))
	}
	return any
}
