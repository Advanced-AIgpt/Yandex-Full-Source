package jsonmatcher

import (
	"bytes"
	"encoding/json"
	"fmt"
	"reflect"
	"strings"

	"a.yandex-team.ru/library/go/core/xerrors"
)

// taken from testify
func ObjectsAreEqual(expected, actual interface{}) bool {
	if expected == nil || actual == nil {
		return expected == actual
	}

	exp, ok := expected.([]byte)
	if !ok {
		return reflect.DeepEqual(expected, actual)
	}

	act, ok := actual.([]byte)
	if !ok {
		return false
	}
	if exp == nil || act == nil {
		return exp == nil && act == nil
	}
	return bytes.Equal(exp, act)
}

func prettyJSONMessage(message json.RawMessage) string {
	var buf bytes.Buffer
	_ = json.Indent(&buf, message, "", "\t")
	return buf.String()
}

func checkNullJSONMismatch(f, s json.RawMessage) bool {
	return (string(f) == "null") != (string(s) == "null")
}

func JSONContentsMatch(expected, actual string) error {
	expectedBytes, actualBytes := []byte(expected), []byte(actual)

	var expectedAsInterface, actualAsInterface interface{}
	if eParseErr := json.Unmarshal(expectedBytes, &expectedAsInterface); eParseErr != nil {
		return xerrors.Errorf("invalid expected json: %w", eParseErr)
	}
	if aParseErr := json.Unmarshal(actualBytes, &actualAsInterface); aParseErr != nil {
		return xerrors.Errorf("invalid actual json: %w", aParseErr)
	}

	// valid jsons -> check if maps
	var expectedAsMap, actualAsMap map[string]json.RawMessage
	eNotMapErr := json.Unmarshal(expectedBytes, &expectedAsMap)
	aNotMapErr := json.Unmarshal(actualBytes, &actualAsMap)

	if eNotMapErr == nil && aNotMapErr == nil {
		var (
			expectedKeys   = make([]string, 0)
			actualKeys     = make([]string, 0)
			missingKeys    = make([]string, 0)
			unexpectedKeys = make([]string, 0)

			mismatchedKeys = make([]string, 0)
		)

		for key, expectedValue := range expectedAsMap {
			expectedKeys = append(expectedKeys, key)
			if actualValue, ok := actualAsMap[key]; ok {
				if err := JSONContentsMatch(string(expectedValue), string(actualValue)); err != nil {
					mismatchedKeys = append(mismatchedKeys, key)
				}
			} else {
				missingKeys = append(missingKeys, key)
			}
		}
		for key := range actualAsMap {
			if _, ok := expectedAsMap[key]; !ok {
				unexpectedKeys = append(unexpectedKeys, key)
			}
			actualKeys = append(actualKeys, key)
		}

		if len(missingKeys) == 0 && len(unexpectedKeys) == 0 && len(mismatchedKeys) == 0 {
			return nil
		}

		var mismatchedMessageBuilder strings.Builder
		if len(mismatchedKeys) != 0 {
			mismatchedMessageBuilder.WriteByte('\n')
			for _, mismatchedKey := range mismatchedKeys {

				mismatchedMessageBuilder.WriteString(fmt.Sprintf("key: %s\n"+
					"expected: %s\n"+
					"actual: %s\n",
					mismatchedKey, prettyJSONMessage(expectedAsMap[mismatchedKey]), prettyJSONMessage(actualAsMap[mismatchedKey])))
			}
		}
		err := xerrors.Errorf("jsonContentsMatchError: matched maps diff\n"+
			"Expected keys, len %d: %v\n"+
			"Actual keys, len %d: %v\n"+
			"Missing keys: %v\n"+
			"Unexpected keys: %v\n\n"+
			"Mismatched keys: %s",
			len(expectedKeys), expectedKeys,
			len(actualKeys), actualKeys,
			missingKeys, unexpectedKeys, mismatchedMessageBuilder.String())

		return err
	}

	// valid json, but not maps -> check if slices
	var expectedAsSlice, actualAsSlice []json.RawMessage
	eNotSliceErr := json.Unmarshal(expectedBytes, &expectedAsSlice)
	aNotSliceErr := json.Unmarshal(actualBytes, &actualAsSlice)

	if eNotSliceErr == nil && aNotSliceErr == nil {
		if checkNullJSONMismatch(expectedBytes, actualBytes) {
			err := xerrors.Errorf("jsonContentsMatchError: matched slices diff\n"+
				"Expected json: %s\n"+
				"Actual json: %s", expectedBytes, actualBytes,
			)
			return err
		}
		notFound := make([]bool, len(expectedAsSlice))
		used := make([]bool, len(actualAsSlice))
		for i, ve := range expectedAsSlice {
			var found bool
			for j, va := range actualAsSlice {
				if used[j] {
					continue
				}
				if valueMatchResult := JSONContentsMatch(string(ve), string(va)); valueMatchResult == nil {
					used[j] = true
					found = true
					break
				}
			}
			if !found {
				notFound[i] = true
			}
		}

		var (
			missingValues    = make([]string, 0)
			unexpectedValues = make([]string, 0)
		)

		for i := range used {
			if !used[i] {
				unexpectedValues = append(unexpectedValues, prettyJSONMessage(actualAsSlice[i]))
			}
		}
		for i := range notFound {
			if notFound[i] {
				missingValues = append(missingValues, prettyJSONMessage(expectedAsSlice[i]))
			}
		}
		if len(missingValues) == 0 && len(unexpectedValues) == 0 {
			return nil
		}
		err := xerrors.Errorf("jsonContentsMatchError: matched slices diff\n"+
			"Expected values, len %d: %v\n"+
			"Actual values, len %d: %v\n"+
			"Missing values: %v\n"+
			"Unexpected values: %v",
			len(expected), expectedAsSlice,
			len(actual), actualAsSlice,
			missingValues, unexpectedValues)
		return err
	}

	// valid jsons, but not maps or slices -> compare as objects
	if !ObjectsAreEqual(expectedAsInterface, actualAsInterface) {
		return xerrors.Errorf("expected object %q not equal to actual object %q", prettyJSONMessage(json.RawMessage(expected)), prettyJSONMessage(json.RawMessage(actual)))
	}
	return nil
}
