package queue

import (
	"encoding/json"
	"reflect"
)

func decodeJSONToInterface(rawJSON []byte, structType reflect.Type) (interface{}, error) {
	ptrType := reflect.PtrTo(structType)

	ptr := reflect.New(ptrType.Elem())
	reflectedPointer := ptr.Interface()

	err := json.Unmarshal(rawJSON, reflectedPointer)
	if err != nil {
		return nil, err
	}

	return ptr.Elem().Interface(), nil
}
