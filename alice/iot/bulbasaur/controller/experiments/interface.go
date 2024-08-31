package experiments

import (
	"context"
)

type IUser interface {
	GetID() uint64
	GetTicket() string
}

type IManagerFactory interface {
	NewManager() IManager
}

type IManager interface {
	PublicExperiments() Experiments
	ExperimentsForUser(ctx context.Context, user IUser) Experiments
	IsExperimentEnabled(name Name) bool
	IsUserExperimentEnabled(ctx context.Context, name Name, user IUser) bool
	IsStaff(ctx context.Context, user IUser) (bool, error)
	SetStaff(userID uint64, isStaff bool)
}

type MockFactory struct {
	NewManagerFunc func() IManager
}

func (mf *MockFactory) NewManager() IManager {
	if mf.NewManagerFunc != nil {
		return mf.NewManagerFunc()
	}
	return make(MockManager)
}

type MockManager map[Name]bool

func (m MockManager) PublicExperiments() Experiments {
	return nil
}

func (m MockManager) ExperimentsForUser(context.Context, IUser) Experiments {
	return nil
}

func (m MockManager) IsStaff(context.Context, IUser) (bool, error) { return false, nil }

func (m MockManager) SetStaff(uint64, bool) {}

func (m MockManager) EnableExperiment(name Name) {
	m[name] = true
}

func (m MockManager) IsExperimentEnabled(name Name) bool {
	return m[name]
}

func (m MockManager) IsUserExperimentEnabled(ctx context.Context, name Name, user IUser) bool {
	return m[name]
}
