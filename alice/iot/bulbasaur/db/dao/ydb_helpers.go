package dao

import (
	"math"
	"reflect"

	"a.yandex-team.ru/library/go/core/xerrors"

	"a.yandex-team.ru/kikimr/public/sdk/go/ydb"
)

const (
	is64bitPlarform = uint64(^uint(0)) == math.MaxUint64
)

type fieldWrapper struct {
	fieldPointer reflect.Value
}

func wrap(v interface{}) fieldWrapper {
	rv := reflect.ValueOf(v)
	if rv.Kind() != reflect.Ptr {
		panic("failed to wrap non pointer value")
	}
	return fieldWrapper{fieldPointer: rv}
}

// UnmarshalYDB implements ydb.Scanner interface
func (f fieldWrapper) UnmarshalYDB(res ydb.RawValue) error {
	fieldValue := f.fieldPointer.Elem()
	switch kind := fieldValue.Kind(); kind {
	case reflect.String:
		v := res.String()
		fieldValue.SetString(v)
	case reflect.Int:
		switch resVal := res.Any().(type) {
		case int64:
			fieldValue.SetInt(resVal)
		case int:
			if !is64bitPlarform {
				return xerrors.Errorf("reflection int work for 64-bit platform only")
			}
			fieldValue.SetInt(int64(resVal))
		case int32:
			fieldValue.SetInt(int64(resVal))
		case nil:
			// default non optional int
			fieldValue.SetInt(0)
		default:
			return xerrors.Errorf("unexpected ydb int type: '%v' type: '%v'", resVal, reflect.TypeOf(resVal))
		}
	default:
		return xerrors.Errorf("unimplemented kind type for: %v", kind.String())
	}
	return nil
}
