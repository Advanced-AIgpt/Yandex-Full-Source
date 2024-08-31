package errors

import (
	"fmt"
	"testing"

	"github.com/stretchr/testify/assert"
	"golang.org/x/xerrors"
)

func TestErrors_Error(t *testing.T) {
	t.Run("nil", func(t *testing.T) {
		var errs Errors = nil
		assert.Equal(t, "", errs.Error())
	})
	t.Run("empty", func(t *testing.T) {
		errs := Errors{}
		assert.Equal(t, "", errs.Error())
	})
	t.Run("single error", func(t *testing.T) {
		errs := Errors{xerrors.New(testingStringSomethingHappen)}
		assert.Equal(t, "something happen", errs.Error())
	})
	t.Run("some errors", func(t *testing.T) {
		errs := Errors{xerrors.New(testingStringSomethingHappen), testingStaticError}
		assert.Equal(t, "something happen; test error", errs.Error())
	})
}

func TestErrors_String(t *testing.T) {
	t.Run("nil", func(t *testing.T) {
		var errs Errors = nil
		assert.Equal(t, "", errs.String())
	})
	t.Run("empty", func(t *testing.T) {
		errs := Errors{}
		assert.Equal(t, "", errs.String())
	})
	t.Run("single error", func(t *testing.T) {
		errs := Errors{xerrors.New(testingStringSomethingHappen)}
		assert.Equal(t, "something happen", errs.String())
	})
	t.Run("some errors", func(t *testing.T) {
		errs := Errors{xerrors.New(testingStringSomethingHappen), testingStaticError}
		assert.Equal(t, "something happen\ntest error", errs.String())
	})
}

//nolint:S1025
func TestErrors_Format(t *testing.T) {
	t.Run("nil", func(t *testing.T) {
		var errs Errors = nil
		assert.Equal(t, "", fmt.Sprintf("%s", errs))
		assert.Equal(t, "", fmt.Sprintf("%q", errs))
		assert.Equal(t, "", fmt.Sprintf("%v", errs))
	})
	t.Run("empty", func(t *testing.T) {
		errs := Errors{}
		assert.Equal(t, "", fmt.Sprintf("%s", errs))
		assert.Equal(t, "", fmt.Sprintf("%q", errs))
		assert.Equal(t, "", fmt.Sprintf("%v", errs))
	})
	t.Run("single error", func(t *testing.T) {
		errs := Errors{xerrors.New(testingStringSomethingHappen)}
		assert.Equal(t, "something happen", fmt.Sprintf("%s", errs))
		assert.Equal(t, "something happen", fmt.Sprintf("%q", errs))
		assert.Equal(t, "something happen", fmt.Sprintf("%v", errs))
	})
	t.Run("some errors", func(t *testing.T) {
		errs := Errors{xerrors.New(testingStringSomethingHappen), testingStaticError}
		assert.Equal(t, "something happen; test error", fmt.Sprintf("%s", errs))
		assert.Equal(t, "something happen; test error", fmt.Sprintf("%q", errs))
		assert.Equal(t, "something happen; test error", fmt.Sprintf("%v", errs))
	})
}

func TestErrors_Is(t *testing.T) {
	t.Run("nil", func(t *testing.T) {
		var errs Errors = nil
		assert.False(t, xerrors.Is(errs, xerrors.New(testingStringSomethingHappen)))
		assert.False(t, xerrors.Is(errs, testingStaticError))
		assert.False(t, xerrors.Is(errs, testingErrorType{}))
		assert.False(t, xerrors.Is(errs, testingErrorType{testingStringTestError}))
	})
	t.Run("empty", func(t *testing.T) {
		errs := Errors{}
		assert.False(t, xerrors.Is(errs, xerrors.New(testingStringSomethingHappen)))
		assert.False(t, xerrors.Is(errs, testingStaticError))
		assert.False(t, xerrors.Is(errs, testingErrorType{}))
		assert.False(t, xerrors.Is(errs, testingErrorType{testingStringTestError}))
	})
	t.Run("single error", func(t *testing.T) {
		// warn: xerrors.New('a') != xerrors.New('a')
		errs := Errors{xerrors.New(testingStringSomethingHappen)}
		assert.False(t, xerrors.Is(errs, xerrors.New(testingStringSomethingHappen)))
		assert.False(t, xerrors.Is(errs, testingStaticError))
		assert.False(t, xerrors.Is(errs, testingErrorType{}))
		assert.False(t, xerrors.Is(errs, testingErrorType{testingStringTestError}))
	})
	t.Run("some errors", func(t *testing.T) {
		t.Run("xerrors.New", func(t *testing.T) {
			// warn: xerrors.New('a') != xerrors.New('a')
			errs := Errors{xerrors.New(testingStringSomethingHappen), testingStaticError}
			assert.False(t, xerrors.Is(errs, xerrors.New(testingStringSomethingHappen)))
			assert.True(t, xerrors.Is(errs, testingStaticError))
			assert.False(t, xerrors.Is(errs, testingErrorType{}))
			assert.False(t, xerrors.Is(errs, testingErrorType{testingStringTestError}))
		})
		t.Run("static error", func(t *testing.T) {
			errs := Errors{xerrors.New(testingStringSomethingHappen), testingStaticError}
			assert.False(t, xerrors.Is(errs, xerrors.New(testingStringSomethingHappen)))
			assert.True(t, xerrors.Is(errs, testingStaticError))
			assert.False(t, xerrors.Is(errs, testingErrorType{}))
			assert.False(t, xerrors.Is(errs, testingErrorType{testingStringTestError}))
		})
		t.Run("static error", func(t *testing.T) {
			errs := Errors{xerrors.New(testingStringSomethingHappen), testingErrorType{testingStringTestError}}
			assert.False(t, xerrors.Is(errs, xerrors.New(testingStringSomethingHappen)))
			assert.False(t, xerrors.Is(errs, testingStaticError))
			assert.False(t, xerrors.Is(errs, testingErrorType{}))
			assert.True(t, xerrors.Is(errs, testingErrorType{testingStringTestError}))
		})
		t.Run("mixed errors", func(t *testing.T) {
			errs := Errors{xerrors.New(testingStringSomethingHappen), testingStaticError, testingErrorType{testingStringTestError}}
			assert.False(t, xerrors.Is(errs, xerrors.New(testingStringSomethingHappen)))
			assert.True(t, xerrors.Is(errs, testingStaticError))
			assert.False(t, xerrors.Is(errs, testingErrorType{}))
			assert.True(t, xerrors.Is(errs, testingErrorType{testingStringTestError}))
		})
	})
}

func TestErrors_As(t *testing.T) {
	t.Run("nil", func(t *testing.T) {
		var errs Errors = nil
		var te testingErrorType
		assert.False(t, xerrors.As(errs, &te))
		assert.Equal(t, testingErrorType{}, te)
	})
	t.Run("empty", func(t *testing.T) {
		errs := Errors{}
		var te testingErrorType
		assert.False(t, xerrors.As(errs, &te))
		assert.Equal(t, testingErrorType{}, te)
	})
	t.Run("single error", func(t *testing.T) {
		errs := Errors{xerrors.New(testingStringSomethingHappen)}
		var te testingErrorType
		assert.False(t, xerrors.As(errs, &te))
		assert.Equal(t, testingErrorType{}, te)
	})
	t.Run("some errors", func(t *testing.T) {
		t.Run("no match", func(t *testing.T) {
			errs := Errors{xerrors.New(testingStringSomethingHappen), testingStaticError}
			var te testingErrorType
			assert.False(t, xerrors.As(errs, &te))
			assert.Equal(t, testingErrorType{}, te)
		})
		t.Run("single match", func(t *testing.T) {
			errs := Errors{xerrors.New(testingStringSomethingHappen), testingErrorType{testingStringTestError}}
			var te testingErrorType
			assert.True(t, xerrors.As(errs, &te))
			assert.Equal(t, testingErrorType{"test error"}, te)
		})
		t.Run("multi match", func(t *testing.T) {
			errs := Errors{xerrors.New(testingStringSomethingHappen), testingErrorType{"1"}, testingErrorType{"2"}}
			var te testingErrorType
			assert.True(t, xerrors.As(errs, &te))
			assert.Equal(t, testingErrorType{"1"}, te)
		})
	})
}
