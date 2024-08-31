package jsonmatcher

import (
	"testing"

	"github.com/stretchr/testify/assert"
)

func TestJSONContentsMatch(t *testing.T) {
	t.Run("Primitives", func(t *testing.T) {
		assert.NoError(t, JSONContentsMatch(
			"21", "21",
		))
		assert.NoError(t, JSONContentsMatch(
			"3.14", "3.14",
		))
		assert.NoError(t, JSONContentsMatch(
			"true", "true",
		))
		assert.NoError(t, JSONContentsMatch(
			`"wololo"`, `"wololo"`,
		))
		assert.NoError(t, JSONContentsMatch(
			"null", "null",
		))
		assert.Error(t, JSONContentsMatch(
			"null", "iot",
		))
		assert.Error(t, JSONContentsMatch(
			"iot", "null",
		))
		assert.Error(t, JSONContentsMatch(
			`wololo`, `null`,
		))
		assert.Error(t, JSONContentsMatch(
			`null`, `wololo`,
		))
		assert.Error(t, JSONContentsMatch(
			`invalid`, `"somestring"`,
		))
		assert.Error(t, JSONContentsMatch(
			`..???`, `"somestring"`,
		))
		assert.Error(t, JSONContentsMatch(
			`"welp"`, `????!!"`,
		))
		assert.Error(t, JSONContentsMatch(
			`"epic"`, `"cipe"`,
		))
		assert.Error(t, JSONContentsMatch(
			"2.5", "3.5",
		))
		assert.Error(t, JSONContentsMatch(
			"true", "false",
		))
	})

	t.Run("SlicePrimitives", func(t *testing.T) {
		assert.NoError(t, JSONContentsMatch(
			`[21, 21]`, `[21, 21]`,
		))
		assert.NoError(t, JSONContentsMatch(
			`[21, 22]`, `[22, 21]`,
		))
		assert.NoError(t, JSONContentsMatch(
			`[3.14, 1.13]`, `[1.13, 3.14]`,
		))
		assert.NoError(t, JSONContentsMatch(
			`[3.14, 1.13, 1.13]`, `[1.13, 3.14, 1.13]`,
		))
		assert.NoError(t, JSONContentsMatch(
			`[3.14, true, "epic", 777]`, `[777, 3.14, "epic", true]`,
		))

		assert.Error(t, JSONContentsMatch(
			`[3.14, true]`, `[]`,
		))
		assert.Error(t, JSONContentsMatch(
			`[3.14, true, "epic", 777]`, `[777, 3.14, "epic"]`,
		))
		assert.Error(t, JSONContentsMatch(
			`[21, 22]`, `[21, 21]`,
		))
		assert.Error(t, JSONContentsMatch(
			`[3.14, 0]`, `[1.13, 3.14]`,
		))
	})
	t.Run("SliceNulls", func(t *testing.T) {
		assert.NoError(t, JSONContentsMatch(
			`null`, `null`,
		))
		assert.Error(t, JSONContentsMatch(
			`[]`, `null`,
		))
		assert.Error(t, JSONContentsMatch(
			`null`, `[]`,
		))
		assert.NoError(t, JSONContentsMatch(
			`[]`, `[]`,
		))
	})
	t.Run("Compound", func(t *testing.T) {
		assert.NoError(t, JSONContentsMatch(
			`{"epic":"loot", "23":45}`, `{"epic":"loot", "23":45}`,
		))
		assert.NoError(t, JSONContentsMatch(
			`{"epic":"loot", "true":22}`, `{"true":22, "epic":"loot"}`,
		))
		assert.NoError(t, JSONContentsMatch(
			`{"epic":"loot", "wololo":[22, 23, 34]}`, `{"wololo":[23, 22, 34], "epic":"loot"}`,
		))

		assert.Error(t, JSONContentsMatch(
			`{"epic":"loot"}`, `{"true":22, "epic":"loot"}`,
		))
		assert.Error(t, JSONContentsMatch(
			`{"epic":"loot", "welp":[22, 23, 34]}`, `{"wololo":[23, 22, 34], "epic":"loot"}`,
		))
		assert.Error(t, JSONContentsMatch(
			`{"epic":"loot", "wololo":[22, 23, 34]}`, `{"wololo":[23, 22], "epic":"loot"}`,
		))
	})

	t.Run("SliceCompounds", func(t *testing.T) {
		assert.NoError(t, JSONContentsMatch(
			`[21, {"quest":"loot"}]`, `[{"quest":"loot"}, 21]`,
		))

		assert.NoError(t, JSONContentsMatch(
			`[21, {"quest":["loot", 1]}]`, `[{"quest":[1, "loot"]}, 21]`,
		))
	})
}
