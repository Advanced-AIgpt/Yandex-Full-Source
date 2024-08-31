package queue

import (
	"encoding/json"
	"reflect"
	"testing"

	"github.com/stretchr/testify/assert"
)

func Test_SimpleStructureDecode(t *testing.T) {
	type Something struct {
		Param int    `json:"key1"`
		Test  string `json:"key2"`
	}

	s := Something{42, "hello, world!"}
	rawJSON, err := json.Marshal(s)
	assert.NoError(t, err)

	obj, err := decodeJSONToInterface(rawJSON, reflect.TypeOf(Something{}))
	assert.NoError(t, err)

	assert.IsType(t, Something{}, obj)
	casted, ok := obj.(Something)
	assert.True(t, ok)
	assert.Equal(t, s, casted)
}

func Test_ComplexTypesStructureDecode(t *testing.T) {
	type AliasType string

	type Something struct {
		Slice []uint64  `json:"key1"`
		Alias AliasType `json:"key2"`
	}

	s := Something{[]uint64{1, 2, 42, 100500}, "hello, world!"}
	rawJSON, err := json.Marshal(s)
	assert.NoError(t, err)

	obj, err := decodeJSONToInterface(rawJSON, reflect.TypeOf(Something{}))
	assert.NoError(t, err)

	assert.IsType(t, Something{}, obj)
	casted, ok := obj.(Something)
	assert.True(t, ok)
	assert.Equal(t, s, casted)
}

func Test_ArrayOfStructuresDecode(t *testing.T) {
	type Item struct {
		Value int `json:"subkey1"`
	}

	type Something struct {
		Slice []Item `json:"key1"`
	}

	s := Something{[]Item{{1}, {2}, {42}, {100500}}}
	rawJSON, err := json.Marshal(s)
	assert.NoError(t, err)

	obj, err := decodeJSONToInterface(rawJSON, reflect.TypeOf(Something{}))
	assert.NoError(t, err)

	assert.IsType(t, Something{}, obj)
	casted, ok := obj.(Something)
	assert.True(t, ok)
	assert.Equal(t, s, casted)
}

func Test_JsonWithoutTags(t *testing.T) {
	type Item struct {
		Value int
	}

	type Something struct {
		Slice []Item `json:"key1"`
	}

	s := Something{[]Item{{1}, {2}, {42}, {100500}}}
	rawJSON, err := json.Marshal(s)
	assert.NoError(t, err)

	obj, err := decodeJSONToInterface(rawJSON, reflect.TypeOf(Something{}))
	assert.NoError(t, err)

	assert.IsType(t, Something{}, obj)
	casted, ok := obj.(Something)
	assert.True(t, ok)
	assert.Equal(t, s, casted)
}

func Test_NestedStructureDecode(t *testing.T) {
	type Inner struct {
		Inside float64 `json:"surprise"`
	}

	type Something struct {
		Param int    `json:"key1"`
		Test  string `json:"key2"`
		More  Inner  `json:"key3"`
	}

	s := Something{42, "hello, world!", Inner{Inside: 36.6}}
	rawJSON, err := json.Marshal(s)
	assert.NoError(t, err)

	obj, err := decodeJSONToInterface(rawJSON, reflect.TypeOf(Something{}))
	assert.NoError(t, err)

	assert.IsType(t, Something{}, obj)
	casted, ok := obj.(Something)
	assert.True(t, ok)
	assert.Equal(t, s, casted)
}
