package updates

import (
	"context"
	"github.com/stretchr/testify/suite"
	"testing"

	xtesting "a.yandex-team.ru/alice/iot/bulbasaur/testing"
	"a.yandex-team.ru/alice/library/go/xiva"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/xerrors"
	xiva2 "a.yandex-team.ru/library/go/yandex/xiva"
)

type UpdatesSuite struct {
	suite.Suite
	Ctx        context.Context
	XivaMock   *xiva.MockClient
	Controller *Controller
	Logger     log.Logger
	UserID     uint64
}

func (s *UpdatesSuite) SetupTest() {
	s.Ctx = context.Background()
	s.XivaMock = xiva.NewMockClient()
	s.UserID = 8992923
	logger, _ := xtesting.ObservedLogger()
	s.Logger = logger
	s.Controller = &Controller{
		xivaClient: s.XivaMock,
		dbClient:   nil, // TODO: add mock
		logger:     s.Logger,
	}
}

func (s *UpdatesSuite) TestHasActiveSubscribers() {
	testCases := []struct {
		Name      string
		XivaSubs  []xiva2.Subscription
		XivaError error
		Expected  bool
	}{
		{
			Name: "has several active subs",
			XivaSubs: []xiva2.Subscription{
				{ID: "2323"}, {ID: "2355223"},
			},
			XivaError: nil,
			Expected:  true,
		},
		{
			Name: "has one active sub",
			XivaSubs: []xiva2.Subscription{
				{ID: "2323"},
			},
			XivaError: nil,
			Expected:  true,
		},
		{
			Name:      "has no one active sub",
			XivaSubs:  []xiva2.Subscription{},
			XivaError: nil,
			Expected:  false,
		},
		{
			Name:      "has no one active subs with nil",
			XivaSubs:  nil,
			XivaError: nil,
			Expected:  false,
		},
		{
			Name:      "xiva returns error cant skip",
			XivaSubs:  nil,
			XivaError: xerrors.New("some xiva err"),
			Expected:  true, // fallback for error
		},
	}
	for _, tc := range testCases {
		s.Run(tc.Name, func() {
			s.XivaMock.Subscriptions = tc.XivaSubs
			s.XivaMock.Error = tc.XivaError
			s.Equal(tc.Expected, s.Controller.HasActiveMobileSubscriptions(s.Ctx, s.UserID))
		})
	}
}

func TestUpdatesController(t *testing.T) {
	suite.Run(t, new(UpdatesSuite))
}
