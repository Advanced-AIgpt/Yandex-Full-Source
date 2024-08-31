package mobile

import (
	"a.yandex-team.ru/alice/iot/bulbasaur/model"
	"a.yandex-team.ru/library/go/core/xerrors"
)

func nameValidationErrorToErrorCode(err error) model.ErrorCode {
	var nameLengthError *model.NameLengthError
	switch {
	case xerrors.Is(err, &model.QuasarNameCharError{}):
		validError := &model.QuasarNameCharError{}
		return validError.ErrorCode()
	case xerrors.Is(err, &model.NameCharError{}):
		validError := &model.NameCharError{}
		return validError.ErrorCode()
	case xerrors.As(err, &nameLengthError):
		return nameLengthError.ErrorCode()
	case xerrors.Is(err, &model.NameMinLettersError{}):
		validError := &model.NameMinLettersError{}
		return validError.ErrorCode()
	case xerrors.Is(err, &model.NameEmptyError{}):
		validError := model.NameEmptyError{}
		return validError.ErrorCode()
	default:
		return model.InvalidValue
	}
}
