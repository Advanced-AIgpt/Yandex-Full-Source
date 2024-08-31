package uuid

import (
	"github.com/gofrs/uuid"
)

func New() string {
	return uuid.Must(uuid.NewV4()).String()
}
