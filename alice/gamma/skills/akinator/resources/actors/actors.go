package actors

import (
	"encoding/json"

	"golang.org/x/xerrors"

	"a.yandex-team.ru/library/go/core/resource"
)

type Actor struct {
	ID          string   `json:"id"`
	Hints       []string `json:"hints"`
	NameToSay   string   `json:"say"`
	NameToWrite string   `json:"write"`
}

func GetActors() (actors []Actor, err error) {
	if err = json.Unmarshal(resource.Get("actorsData.json"), &actors); err != nil {
		return nil, xerrors.Errorf("Invalid resource actors")
	}
	return actors, nil
}

func GetPatterns() (patterns map[string]map[string][]string, err error) {
	if err := json.Unmarshal(resource.Get("actorsPatterns.json"), &patterns); err != nil {
		return nil, xerrors.Errorf("Invalid resource")
	}
	return patterns, nil
}
