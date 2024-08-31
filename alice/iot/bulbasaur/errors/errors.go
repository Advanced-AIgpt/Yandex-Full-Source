package errors

import (
	"fmt"
	"io"
	"strings"

	"golang.org/x/xerrors"
)

type Errors []error

func (es Errors) Error() string {
	errs := make([]string, 0, len(es))
	for _, e := range es {
		errs = append(errs, e.Error())
	}

	return strings.Join(errs, "; ")
}

func (es Errors) String() string {
	errs := make([]string, 0, len(es))
	for _, e := range es {
		errs = append(errs, e.Error())
	}

	return strings.Join(errs, "\n")
}

func (es Errors) Format(s fmt.State, verb rune) {
	switch verb {
	case 'v':
		if s.Flag('+') {
			for i, e := range es {
				if i > 0 {
					_, _ = io.WriteString(s, "; ")
				}
				_, _ = io.WriteString(s, fmt.Sprintf("%+v", e))
			}
			return
		}
		fallthrough
	case 's', 'q':
		_, _ = io.WriteString(s, es.Error())
	}
}

func (es Errors) Is(target error) bool {
	for _, e := range es {
		if xerrors.Is(e, target) {
			return true
		}
	}
	return false
}

// All returns true if all errors match to target error
func (es Errors) All(target error) bool {
	for _, e := range es {
		if !xerrors.Is(e, target) {
			return false
		}
	}
	return true
}

func (es Errors) As(target interface{}) bool {
	for _, e := range es {
		if xerrors.As(e, target) {
			return true
		}
	}
	return false
}

func (es Errors) Add(otherErr error) Errors {
	combinedErrs := es
	switch typedErr := otherErr.(type) {
	case Errors:
		for _, err := range typedErr {
			combinedErrs = combinedErrs.Add(err)
		}
	default:
		combinedErrs = append(combinedErrs, otherErr)
	}
	return combinedErrs
}
