package binder

import (
	"encoding/json"
	"fmt"
	"net/http"
	"reflect"

	"golang.org/x/xerrors"

	bulbasaur "a.yandex-team.ru/alice/iot/bulbasaur/errors"
	"a.yandex-team.ru/library/go/valid"
)

var DefaultValidationContext = valid.NewValidationCtx()

func init() {
	DefaultValidationContext.Add("equal", valid.Equal)
	DefaultValidationContext.Add("notEq", valid.NotEqual)
	DefaultValidationContext.Add("lesser", valid.Lesser)
	DefaultValidationContext.Add("greater", valid.Greater)
	DefaultValidationContext.Add("min", valid.Min)
	DefaultValidationContext.Add("max", valid.Max)
	DefaultValidationContext.Add("required", valid.WrapValidator(func(s string) error {
		if len(s) == 0 {
			return xerrors.New("value is required")
		}
		return nil
	}))
	DefaultValidationContext.Add("notNil", func(value reflect.Value, param string) (err error) {
		defer func() {
			if recover() != nil {
				err = valid.ErrInvalidType
			}
		}()
		if value.IsNil() {
			return xerrors.New("value is nil")
		}
		return nil
	})
}

type SyntaxError struct {
	msg    string
	offset int64
}

func (e *SyntaxError) Error() string {
	return fmt.Sprintf("json: syntax error at %d: %s", e.offset, e.msg)
}

func (e *SyntaxError) HTTPStatus() int {
	return http.StatusBadRequest
}

type UnmarshalTypeError struct {
	value  string
	offset int64
}

func (e *UnmarshalTypeError) Error() string {
	return fmt.Sprintf("json: unexpected type at %d: %s", e.offset, e.value)
}

func (e *UnmarshalTypeError) HTTPStatus() int {
	return http.StatusBadRequest
}

func Bind(ctx *valid.ValidationCtx, data []byte, v interface{}) error {
	var errs bulbasaur.Errors

	if err := json.Unmarshal(data, v); err != nil {
		//do not process further in case we've got invalid JSON
		var (
			jsonSE  *json.SyntaxError
			jsonUTE *json.UnmarshalTypeError
		)
		switch {
		case xerrors.As(err, &jsonSE):
			err = &SyntaxError{
				msg:    jsonSE.Error(),
				offset: jsonSE.Offset,
			}
		case xerrors.As(err, &jsonUTE):
			err = &UnmarshalTypeError{
				value:  jsonUTE.Value,
				offset: jsonUTE.Offset,
			}
		}
		return err
	}

	vctx := valid.NewValidationCtx()
	vctx.Merge(DefaultValidationContext)
	vctx.Merge(ctx)
	if verr := valid.Struct(vctx, v); verr != nil {
		errs = append(errs, verr.(valid.Errors)...)
	}

	if len(errs) > 0 {
		return errs
	}

	return nil
}
