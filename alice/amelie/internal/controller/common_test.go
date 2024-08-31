package controller

import (
	"testing"
)

func makeLongString() string {
	var runes []rune
	for i := 0; i < 256; i++ {
		runes = append(runes, 'x')
	}
	return string(runes)
}

func TestNewCallbackWithArg(t *testing.T) {
	t.Run("hashable", func(t *testing.T) {
		for _, cb := range []string{
			shortcutsDeleteCallback,
		} {
			t.Run(cb, func(t *testing.T) {
				s := newCallbackWithArg(cb, getHash(makeLongString()))
				if len([]byte(s)) > 64 {
					t.Fatalf("%s too long", s)
				}
			})
		}
	})
}

func TestParseCallbackWithArg(t *testing.T) {
	t.Run("Ok", func(t *testing.T) {
		expected := getHash("source string")
		actual := parseCallbackWithArg(shortcutsDeleteCallback, newCallbackWithArg(shortcutsDeleteCallback, expected))
		if actual != expected {
			t.Fatalf("%s != %s", actual, expected)
		}
	})
}
