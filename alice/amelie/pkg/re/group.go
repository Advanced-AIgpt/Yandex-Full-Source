package re

import (
	"context"
	"regexp"
)

type NamedGroups map[string]string

type namedGroupsKey struct{}

func MatchNamedGroups(regexp *regexp.Regexp, s string) NamedGroups {
	match := regexp.FindStringSubmatch(s)
	subMatchMap := make(map[string]string)
	for i, name := range regexp.SubexpNames() {
		if i >= len(match) {
			break
		}
		if i != 0 {
			subMatchMap[name] = match[i]
		}
	}
	return subMatchMap
}

func WithNamedGroups(ctx context.Context, groups NamedGroups) context.Context {
	return context.WithValue(ctx, namedGroupsKey{}, groups)
}

func GetNamedGroups(ctx context.Context) NamedGroups {
	groups, _ := ctx.Value(namedGroupsKey{}).(NamedGroups)
	return groups
}
