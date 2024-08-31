package callback_test

import (
	"os"
	"testing"

	"a.yandex-team.ru/alice/iot/adapters/xiaomi_adapter/controller/callback"
	"a.yandex-team.ru/library/go/core/xerrors"
	"github.com/stretchr/testify/assert"
)

func TestXerrors(t *testing.T) {
	simpleErr := xerrors.New("something bad")
	complexErr := &os.PathError{
		Op:   "op",
		Path: "path",
		Err:  xerrors.New("bad path error"),
	}

	// test that .Is works with simple errors
	simpleCallbackError := callback.NewError(callback.SubscriptionKeyErrorCode, simpleErr)
	assert.True(t, xerrors.Is(simpleCallbackError, simpleErr))

	// test that .As works with complex errors
	complexCallbackError := callback.NewError(callback.SubscriptionKeyErrorCode, complexErr)
	testPathError := &os.PathError{}
	assert.True(t, xerrors.As(complexCallbackError, &testPathError))

	// test that .As works with callback.Error
	wrappedCallbackError := xerrors.Errorf("wrapped error: %w", callback.NewError(callback.SubscriptionKeyErrorCode, xerrors.New("bad wrapped error")))
	var testCallbackError callback.Error
	assert.True(t, xerrors.As(wrappedCallbackError, &testCallbackError))
}
