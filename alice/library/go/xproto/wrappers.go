package xproto

import (
	"github.com/golang/protobuf/ptypes/wrappers"
	"google.golang.org/protobuf/types/known/wrapperspb"
)

func WrapDouble(value *float64) *wrappers.DoubleValue {
	if value == nil {
		return nil
	}
	return wrapperspb.Double(*value)
}
