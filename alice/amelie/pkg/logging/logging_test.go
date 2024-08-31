package logging

import (
	"strings"
	"testing"
)

type (
	Any interface{}

	OuterStruct struct {
		InnerStruct
		Struct                   InnerStruct  `json:"struct"`
		StructHidden             InnerStruct  `json:"struct_hidden" logging:"hidden"`
		StructPtr                *InnerStruct `json:"struct_ptr"`
		StructPtrHidden          *InnerStruct `json:"struct_ptr_hidden" logging:"hidden"`
		StructInterface          Any          `json:"struct_interface"`
		StructInterfaceHidden    Any          `json:"struct_interface_hidden" logging:"hidden"`
		StructInterfacePtr       Any          `json:"struct_interface_ptr"`
		StructInterfaceHiddenPtr Any          `json:"struct_interface_ptr_hidden" logging:"hidden"`
	}

	InnerStruct struct {
		String          string  `json:"string"`
		StringHidden    string  `json:"string_hidden" logging:"hidden"`
		Int32           int32   `json:"int_32"`
		Int32Hidden     int32   `json:"int_32_hidden" logging:"hidden"`
		Bool            bool    `json:"bool"`
		BoolHidden      bool    `json:"bool_hidden" logging:"hidden"`
		StringPtr       *string `json:"string_ptr"`
		StringHiddenPtr *string `json:"string_ptr_hidden" logging:"hidden"`
	}
)

func marshalTraverse(t *testing.T, m map[string]interface{}) {
	for k, v := range m {
		if strings.HasSuffix(k, "hidden") {
			if HiddenPlaceholder != v {
				t.Errorf("%s != %s", HiddenPlaceholder, v)
			}
		}
		if n, ok := v.(map[string]interface{}); ok {
			marshalTraverse(t, n)
		}
	}
}

func TestMarshal(t *testing.T) {
	s := "v"
	is := InnerStruct{
		String:          s,
		StringHidden:    s,
		Int32:           1,
		Int32Hidden:     1,
		Bool:            true,
		BoolHidden:      true,
		StringPtr:       &s,
		StringHiddenPtr: &s,
	}
	v := OuterStruct{
		InnerStruct:              is,
		Struct:                   is,
		StructHidden:             is,
		StructPtr:                &is,
		StructPtrHidden:          &is,
		StructInterface:          is,
		StructInterfaceHidden:    is,
		StructInterfacePtr:       &is,
		StructInterfaceHiddenPtr: &is,
	}
	lv, err := Marshal(v)
	if err != nil {
		t.Errorf("%s != nil", err)
	}
	marshalTraverse(t, lv)
}
