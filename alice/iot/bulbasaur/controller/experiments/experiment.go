package experiments

import (
	"context"
	"fmt"
	"strings"

	"a.yandex-team.ru/alice/library/go/userctx"
)

type Experiment struct {
	name            Name
	enabled         bool
	allowStaffUsers bool
	allowAll        bool

	// contains all users from WithUserIDs. Not usable to serialize, because groups merged to the field too.
	usersSet map[uint64]struct{}
}

func NewExperiment(name Name, isEnabled bool, userIDs []uint64, allowStaff bool, allowAll bool) *Experiment {
	return (&Experiment{
		name:            name,
		enabled:         isEnabled,
		allowStaffUsers: allowStaff,
		allowAll:        allowAll,
		usersSet:        make(map[uint64]struct{}),
	}).WithUserIDs(userIDs...)
}

func (e *Experiment) GetName() Name {
	return e.name
}

func (e *Experiment) IsEnabled() bool {
	return e.enabled
}

func (e *Experiment) IsAllowedForStaff() bool {
	return e.allowStaffUsers
}

func (e *Experiment) IsAllowedForAll() bool {
	return e.allowAll
}

func (e *Experiment) ContainsUserID(userID uint64) bool {
	_, ok := e.usersSet[userID]
	return ok
}

func (e *Experiment) WithUserIDs(userIDs ...uint64) *Experiment {
	for _, id := range userIDs {
		e.usersSet[id] = struct{}{}
	}
	return e
}

func (e *Experiment) String() string {
	return fmt.Sprintf("'%s' (enabled: %t, staff: %t, all: %t, usersCount: %v)", e.name, e.enabled, e.allowStaffUsers, e.allowAll, len(e.usersSet))
}

type Experiments []Experiment

func (e Experiments) ToMap() map[Name]Experiment {
	result := make(map[Name]Experiment)
	for _, exp := range e {
		result[exp.name] = exp
	}
	return result
}

func (e Experiments) Names() ExperimentNames {
	names := make([]Name, 0, len(e))
	for _, exp := range e {
		names = append(names, exp.name)
	}
	return names
}

func (e Experiments) String() string {
	exps := make([]string, 0, len(e))
	for _, exp := range e {
		exps = append(exps, exp.String())
	}
	return strings.Join(exps, "; ")
}

type Name string

func (n Name) IsEnabled(ctx context.Context) bool {
	manager, err := ManagerFromContext(ctx)
	if err != nil || manager == nil {
		return false
	}

	// todo: this is horrendous
	// check all experiments and make them use IsEnabledForUser where user is a crucial part of experiment

	// wow, amazing and untraceable
	ctxUser, _ := userctx.GetUser(ctx) // also works for empty users,

	return manager.IsUserExperimentEnabled(ctx, n, ctxUser)
}

func (n Name) IsEnabledForUser(ctx context.Context, user userctx.User) bool {
	manager, err := ManagerFromContext(ctx)
	if err != nil || manager == nil {
		return false
	}

	return manager.IsUserExperimentEnabled(ctx, n, user)
}

func (n Name) String() string {
	return string(n)
}

type ExperimentNames []Name

func (en ExperimentNames) Contains(need Name) bool {
	for _, item := range en {
		if item == need {
			return true
		}
	}
	return false
}
