package libquasar

import "errors"

var (
	ErrNotFound  = errors.New("device not found")
	ErrForbidden = errors.New("forbidden")
	ErrConflict  = errors.New("conflict")
)
