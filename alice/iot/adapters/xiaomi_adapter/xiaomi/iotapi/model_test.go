package iotapi

import (
	"reflect"
	"testing"
)

func TestStatus_XiaomiError(t *testing.T) {
	tests := []struct {
		name string
		s    Status
		want Error
	}{
		{
			name: "http 400, location 4, error code 002",
			s:    -704004002,
			want: Error{
				HTTPCode:  400,
				location:  4,
				errorCode: 2,
			},
		},
		{
			name: "http 500, location 3, error code 015",
			s:    -705003015,
			want: Error{
				HTTPCode:  500,
				location:  3,
				errorCode: 15,
			},
		},
		{
			name: "http 401, location 1, error code 903",
			s:    -704011903,
			want: Error{
				HTTPCode:  401,
				location:  1,
				errorCode: 903,
			},
		},
	}
	for _, tt := range tests {
		t.Run(tt.name, func(t *testing.T) {
			if got := tt.s.XiaomiError(); !reflect.DeepEqual(got, tt.want) {
				t.Errorf("XiaomiError() = %v, want %v", got, tt.want)
			}
		})
	}
}
