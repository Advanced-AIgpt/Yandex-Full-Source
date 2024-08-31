package data

import (
	"math/rand"

	"a.yandex-team.ru/alice/iot/bulbasaur/model"
)

func GenerateUser() (u model.User) {
	u.ID = uint64(rand.Uint32())<<32 + uint64(rand.Uint32())
	return u
}
