package localscenarios

import (
	"a.yandex-team.ru/alice/iot/bulbasaur/model"
	xtestdata "a.yandex-team.ru/alice/iot/bulbasaur/xtest/data"
	"a.yandex-team.ru/alice/library/go/requestid"
)

func (s *localScenariosSuite) TestController_SyncLocalScenarios() {
	speakers := xtestdata.WrapDevices(
		xtestdata.GenerateMidiSpeaker("midi-1", "midi-ext-1", "deadbeef"),
	)
	err := s.controller.SyncLocalScenarios(requestid.WithRequestID(s.env.ctx, "sync-1"), 42, model.Scenarios{}, speakers)
	s.Require().NoError(err)

	directive, ok := s.env.notificatorController.SendSpeechkitDirectiveRequests["sync-1"]
	s.Require().True(ok)
	syncScenariosDirective, ok := directive.(*SyncScenariosSpeechkitDirective)
	s.Require().True(ok)
	s.Require().Equal(&SyncScenariosSpeechkitDirective{endpointID: "deadbeef"}, syncScenariosDirective)
}

func (s *localScenariosSuite) TestController_AddLocalScenariosToSpeaker() {
	err := s.controller.AddLocalScenariosToSpeaker(
		requestid.WithRequestID(s.env.ctx, "add-1"), 42, "deadbeef", model.Scenarios{}, model.Devices{},
	)
	s.Require().NoError(err)

	directive, ok := s.env.notificatorController.SendSpeechkitDirectiveRequests["add-1"]
	s.Require().True(ok)
	addScenariosDirective, ok := directive.(*AddScenariosSpeechkitDirective)
	s.Require().True(ok)
	s.Require().Equal(&AddScenariosSpeechkitDirective{endpointID: "deadbeef"}, addScenariosDirective)
}

func (s *localScenariosSuite) TestController_RemoveLocalScenarios() {
	speakers := xtestdata.WrapDevices(
		xtestdata.GenerateMidiSpeaker("midi-1", "midi-ext-1", "deadbeef"),
	)
	scenarioIDs := []string{"1", "2", "3"}
	err := s.controller.RemoveLocalScenarios(
		requestid.WithRequestID(s.env.ctx, "remove-1"), 42, scenarioIDs, speakers,
	)
	s.Require().NoError(err)

	directive, ok := s.env.notificatorController.SendSpeechkitDirectiveRequests["remove-1"]
	s.Require().True(ok)
	addScenariosDirective, ok := directive.(*RemoveScenariosSpeechkitDirective)
	s.Require().True(ok)
	s.Require().Equal(&RemoveScenariosSpeechkitDirective{endpointID: "deadbeef", IDs: scenarioIDs}, addScenariosDirective)
}
