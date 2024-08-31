package errors

import "golang.org/x/xerrors"

var (
	testingStringSomethingHappen = "something happen"
	testingStringTestError       = "test error"
)

var testingStaticError = xerrors.New("test error")

type testingErrorType struct {
	msg string
}

func (t testingErrorType) Error() string {
	return t.msg
}
