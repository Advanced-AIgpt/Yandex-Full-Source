package tools

import (
	"os"
	"strings"

	"a.yandex-team.ru/library/go/core/xerrors"
)

func CheckEnvVariables(variables []string) error {
	var missing []string
	for _, env := range variables {
		if _, ok := os.LookupEnv(env); !ok {
			missing = append(missing, env)
		}
	}
	if len(missing) > 0 {
		return xerrors.Errorf("env variables not set: %s", strings.Join(missing, ";"))
	}
	return nil
}
