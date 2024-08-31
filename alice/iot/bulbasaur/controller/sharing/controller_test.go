package sharing

import (
	"context"
	"fmt"

	"a.yandex-team.ru/alice/iot/bulbasaur/db/dbfiller"
	"a.yandex-team.ru/alice/iot/bulbasaur/model"
	"a.yandex-team.ru/alice/iot/bulbasaur/testing"
	xtestdata "a.yandex-team.ru/alice/iot/bulbasaur/xtest/data"
	"a.yandex-team.ru/alice/library/go/requestid"
)

func (s *Suite) TestSharingController() {
	s.RunControllerTest("HouseholdSharingLink->Invitation->Acceptation", func(ctx context.Context, container TestingContainer, dbfiller *dbfiller.Filler) {
		// preparing data
		alice, err := dbfiller.InsertUser(ctx, model.NewUser("alice"))
		s.Require().NoError(err)
		aliceCurrentHousehold, err := s.dbClient.SelectCurrentHousehold(ctx, alice.ID)
		s.Require().NoError(err)

		// get sharing link
		aliceLink, _, err := container.controller.GetHouseholdSharingLink(ctx, alice.User, aliceCurrentHousehold.ID)
		s.Require().NoError(err)

		bob := model.NewUser("bob")
		// create invitation and decline it
		invitation, err := container.controller.AcceptSharingLink(ctx, bob.User, model.EncodeHouseholdSharingLinkID(aliceLink.ID))
		s.Require().NoError(err)

		err = container.controller.DeclineHouseholdInvitation(ctx, bob.User, invitation.ID)
		s.Require().NoError(err)

		_, err = container.dbClient.SelectUserHousehold(ctx, bob.ID, aliceCurrentHousehold.ID)
		s.Require().Error(err)
		s.ErrorIs(err, &model.UserHouseholdNotFoundError{})

		// now create another invitation and accept it
		invitation, err = container.controller.AcceptSharingLink(ctx, bob.User, model.EncodeHouseholdSharingLinkID(aliceLink.ID))
		s.Require().NoError(err)

		err = container.controller.AcceptHouseholdInvitation(ctx, bob.User, invitation.ID, "Дача")
		s.Require().NoError(err)

		// check alice household for bob
		aliceHouseholdForBob, err := container.dbClient.SelectUserHousehold(ctx, bob.ID, aliceCurrentHousehold.ID)
		s.Require().NoError(err)
		expected := aliceCurrentHousehold
		expected.Name = "Дача"
		expected.SharingInfo = &model.SharingInfo{
			OwnerID:       alice.ID,
			HouseholdID:   aliceCurrentHousehold.ID,
			HouseholdName: "Дача",
		}
		s.Equal(expected, aliceHouseholdForBob)

	})
	s.RunControllerTest("HouseholdAcceptLinkToOwnerAndGuest", func(ctx context.Context, container TestingContainer, dbfiller *dbfiller.Filler) {
		// preparing data
		alice, err := dbfiller.InsertUser(ctx, model.NewUser("alice"))
		s.Require().NoError(err)
		bob, err := dbfiller.InsertUser(ctx, model.NewUser("bob"))
		s.Require().NoError(err)
		aliceCurrentHousehold, err := s.dbClient.SelectCurrentHousehold(ctx, alice.ID)
		s.Require().NoError(err)
		err = s.dbClient.StoreSharedHousehold(ctx, bob.ID, model.SharingInfo{
			OwnerID:       alice.ID,
			HouseholdID:   aliceCurrentHousehold.ID,
			HouseholdName: "Дача",
		})
		s.Require().NoError(err)

		// get sharing link
		aliceLink, _, err := container.controller.GetHouseholdSharingLink(ctx, alice.User, aliceCurrentHousehold.ID)
		s.Require().NoError(err)

		// trying to create invitation to owner by owner
		_, err = container.controller.AcceptSharingLink(ctx, alice.User, model.EncodeHouseholdSharingLinkID(aliceLink.ID))
		s.ErrorIs(err, &model.SharingLinkNeedlessAcceptanceError{})

		// trying to create invitation to guest who already has this household
		_, err = container.controller.AcceptSharingLink(ctx, bob.User, model.EncodeHouseholdSharingLinkID(aliceLink.ID))
		s.ErrorIs(err, &model.SharingLinkNeedlessAcceptanceError{})
	})
	s.RunControllerTest("GetSharingUsersFor0Users", func(ctx context.Context, container TestingContainer, dbfiller *dbfiller.Filler) {
		alice, err := dbfiller.InsertUser(ctx, model.NewUser("alice"))
		s.Require().NoError(err)
		users, err := container.controller.GetSharingUsers(ctx, alice.User, []uint64{})
		s.Require().NoError(err)
		s.Len(users, 0)
	})
	s.RunControllerTest("SharingUsersLimitReached", func(ctx context.Context, container TestingContainer, dbfiller *dbfiller.Filler) {
		bob, err := dbfiller.InsertUser(ctx, model.NewUser("bob"))
		s.Require().NoError(err)
		for i := 0; i < 4; i++ {
			newAlice, err := dbfiller.InsertUser(ctx, model.NewUser(fmt.Sprintf("alice%d", i)))
			s.Require().NoError(err)
			newAliceCurrentHousehold, err := s.dbClient.SelectCurrentHousehold(ctx, newAlice.ID)
			s.Require().NoError(err)
			err = s.dbClient.StoreSharedHousehold(ctx, bob.ID, model.SharingInfo{
				OwnerID:       newAlice.ID,
				HouseholdID:   newAliceCurrentHousehold.ID,
				HouseholdName: testing.RandOnlyCyrillicString(15),
			})
			s.Require().NoError(err)
		}
		carol, err := dbfiller.InsertUser(ctx, model.NewUser("carol"))
		s.NoError(err)
		carolCurrentHousehold, err := s.dbClient.SelectCurrentHousehold(ctx, carol.ID)
		s.NoError(err)
		// get sharing link
		carolLink, _, err := container.controller.GetHouseholdSharingLink(ctx, carol.User, carolCurrentHousehold.ID)
		s.NoError(err)
		// create invitation
		invitation, err := container.controller.AcceptSharingLink(ctx, bob.User, model.EncodeHouseholdSharingLinkID(carolLink.ID))
		s.NoError(err)
		err = container.controller.AcceptHouseholdInvitation(ctx, bob.User, invitation.ID, carolCurrentHousehold.Name)
		s.Error(err)
		s.ErrorIs(err, &model.SharingUsersLimitReachedError{})
	})
	s.RunControllerTest("HouseholdPendingInvitationsAndRevoke", func(ctx context.Context, container TestingContainer, dbfiller *dbfiller.Filler) {
		alice, err := dbfiller.InsertUser(ctx, model.NewUser("alice"))
		s.Require().NoError(err)
		bob, err := dbfiller.InsertUser(ctx, model.NewUser("bob"))
		s.Require().NoError(err)
		aliceCurrentHousehold, err := s.dbClient.SelectCurrentHousehold(ctx, alice.ID)
		s.Require().NoError(err)
		err = s.dbClient.StoreSharedHousehold(ctx, bob.ID, model.SharingInfo{
			OwnerID:       alice.ID,
			HouseholdID:   aliceCurrentHousehold.ID,
			HouseholdName: "Дача",
		})
		s.Require().NoError(err)
		// get sharing link
		aliceLink, _, err := container.controller.GetHouseholdSharingLink(ctx, alice.User, aliceCurrentHousehold.ID)
		s.Require().NoError(err)
		carol, err := dbfiller.InsertUser(ctx, model.NewUser("carol"))
		s.Require().NoError(err)
		// creating invitation
		_, err = container.controller.AcceptSharingLink(ctx, carol.User, model.EncodeHouseholdSharingLinkID(aliceLink.ID))
		s.Require().NoError(err)
		residents, err := container.dbClient.SelectHouseholdResidents(ctx, alice.ID, aliceCurrentHousehold)
		s.NoError(err)
		expected := model.HouseholdResidents{
			{
				ID:   bob.ID,
				Role: model.GuestHouseholdRole,
			},
			{
				ID:   carol.ID,
				Role: model.PendingInvitationHouseholdRole,
			},
			{
				ID:   alice.ID,
				Role: model.OwnerHouseholdRole,
			},
		}
		s.Equal(expected, residents)
		// revoke invitation
		err = container.controller.RevokeGuestsHouseholdInvitations(ctx, alice.User, aliceCurrentHousehold.ID, []uint64{carol.ID})
		s.NoError(err)
		// recheck
		residents, err = container.dbClient.SelectHouseholdResidents(ctx, alice.ID, aliceCurrentHousehold)
		s.NoError(err)
		expected = model.HouseholdResidents{
			{
				ID:   bob.ID,
				Role: model.GuestHouseholdRole,
			},
			{
				ID:   alice.ID,
				Role: model.OwnerHouseholdRole,
			},
		}
		s.Equal(expected, residents)
	})
	s.RunControllerTest("VoiceprintViaSoundSpeakers", func(ctx context.Context, container TestingContainer, dbfiller *dbfiller.Filler) {
		alice, err := dbfiller.InsertUser(ctx, model.NewUser("alice"))
		s.Require().NoError(err)
		miniSpeaker := xtestdata.GenerateSmartSpeaker("mini1", "mini1-ext", "mini1-quasar-id", model.YandexStationMiniDeviceType)
		miniSpeaker, err = dbfiller.InsertDevice(ctx, &alice.User, miniSpeaker)
		s.Require().NoError(err)
		err = container.controller.StartVoiceprintScenarioOnDevice(requestid.WithRequestID(s.context, "voiceprint-via-sound"), alice.User, miniSpeaker.ID)
		s.NoError(err)
		directive := container.notificator.GetDirective("voiceprint-via-sound", alice.ID, "mini1-quasar-id")
		s.Equal("mini1-quasar-id", directive.EndpointID())
		_, ok := directive.(*BioStartSoundEnrollmentSKDirective)
		s.Require().True(ok)
	})
}
