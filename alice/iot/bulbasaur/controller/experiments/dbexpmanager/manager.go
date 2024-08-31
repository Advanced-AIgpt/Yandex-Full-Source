package dbexpmanager

import (
	"context"
	"fmt"
	"sync"
	"time"

	"a.yandex-team.ru/alice/iot/bulbasaur/controller/experiments"

	"a.yandex-team.ru/alice/iot/bulbasaur/db"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/log/ctxlog"
	"a.yandex-team.ru/library/go/core/xerrors"
	"a.yandex-team.ru/library/go/yandex/blackbox"
)

const updatePeriod = time.Minute

type Manager struct {
	experiments map[experiments.Name]experiments.Experiment

	logger         log.Logger
	blackboxClient blackbox.Client
	staffUsers     map[uint64]bool
}

func (m *Manager) ExperimentsForUser(ctx context.Context, user experiments.IUser) experiments.Experiments {
	var res experiments.Experiments
	for expName, exp := range m.experiments {
		if m.IsUserExperimentEnabled(ctx, expName, user) {
			res = append(res, exp)
		}
	}
	return res
}

func (m *Manager) IsExperimentEnabled(name experiments.Name) bool {
	exp, ok := m.experiments[name]
	if !ok {
		return false
	}
	return exp.IsEnabled()
}

func (m *Manager) PublicExperiments() experiments.Experiments {
	var res experiments.Experiments
	for _, exp := range m.experiments {
		if exp.IsEnabled() && exp.IsAllowedForAll() {
			res = append(res, exp)
		}
	}
	return res
}

func (m *Manager) IsUserExperimentEnabled(ctx context.Context, name experiments.Name, user experiments.IUser) bool {
	exp, ok := m.experiments[name]
	if !ok {
		ctxlog.Infof(ctx, m.logger, "manager: experiment with name %s not found", name)
		return false
	}
	if !exp.IsEnabled() {
		return false
	}

	if exp.IsAllowedForAll() || exp.ContainsUserID(user.GetID()) {
		return true
	}

	var userIsStaff bool
	if exp.IsAllowedForStaff() {
		isStaff, isFound := m.staffUsers[user.GetID()]
		if !isFound {
			// we don't know if user is staff - so we check it and save for later
			var err error
			isStaff, err = m.IsStaff(ctx, user)
			if err != nil {
				ctxlog.Warnf(ctx, m.logger, "manager: unable to determine if user %d is staff: %s", user.GetID(), err)
			} else {
				m.staffUsers[user.GetID()] = isStaff
			}
		}
		userIsStaff = isStaff
	}
	return exp.IsAllowedForStaff() && userIsStaff
}

func (m *Manager) SetStaff(userID uint64, isStaff bool) {
	m.staffUsers[userID] = isStaff
}

func (m *Manager) IsStaff(ctx context.Context, user experiments.IUser) (bool, error) {
	bbRequest := blackbox.UserTicketRequest{
		UserTicket: user.GetTicket(),
		UIDs:       []blackbox.ID{user.GetID()},
		Aliases:    []blackbox.UserAlias{blackbox.UserAliasYandexoid},
	}
	resp, err := m.blackboxClient.UserTicket(ctx, bbRequest)
	if err != nil {
		ctxlog.Warnf(ctx, m.logger, "manager: user ticket request error: %v", err)
		return false, err
	}
	if len(resp.Users) == 1 {
		_, isStaff := resp.Users[0].Aliases[blackbox.UserAliasYandexoid]
		return isStaff, nil
	}
	msg := fmt.Sprintf("manager: cannot find user info for user: %d", user.GetID())
	ctxlog.Warn(ctx, m.logger, msg)
	return false, xerrors.New(msg)
}

type ManagerFactory struct {
	logger         log.Logger
	db             db.DB
	blackboxClient blackbox.Client
	ticker         *time.Ticker

	m           sync.RWMutex
	experiments map[experiments.Name]experiments.Experiment
}

func NewManagerFactory(logger log.Logger, db db.DB, blackboxClient blackbox.Client) *ManagerFactory {
	return &ManagerFactory{
		logger:         logger,
		db:             db,
		blackboxClient: blackboxClient,
		experiments:    make(map[experiments.Name]experiments.Experiment),
	}
}

func (m *ManagerFactory) NewManager() experiments.IManager {
	m.m.RLock()
	defer m.m.RUnlock()

	resExperiments := make(map[experiments.Name]experiments.Experiment, len(m.experiments))
	for k, v := range m.experiments {
		resExperiments[k] = v
	}
	return &Manager{
		experiments:    resExperiments,
		logger:         m.logger,
		blackboxClient: m.blackboxClient,
		staffUsers:     make(map[uint64]bool),
	}
}

func (m *ManagerFactory) Start() error {
	m.ticker = time.NewTicker(updatePeriod)

	updateFunc := func() error {
		queryCtx, cancel := context.WithTimeout(context.Background(), 5*time.Second)
		defer cancel()

		allExperiments, err := m.db.SelectAllExperiments(queryCtx)
		if err != nil {
			m.logger.Warnf("failed to fetch experiments: %v", err)
			return err
		}

		m.m.Lock()
		defer m.m.Unlock()
		m.experiments = allExperiments.ToMap()
		return nil
	}

	if err := updateFunc(); err != nil { // has to be sync on start, otherwise we will start without experiments
		return err
	}

	go func() {
		for range m.ticker.C {
			_ = updateFunc()
		}
	}()

	return nil
}

func (m *ManagerFactory) Stop() {
	m.ticker.Stop()
}
