package sharing

import (
	"context"

	"a.yandex-team.ru/alice/iot/bulbasaur/controller/sharing/model"
	"a.yandex-team.ru/alice/iot/bulbasaur/model"
	"a.yandex-team.ru/alice/library/go/timestamp"
)

type IController interface {
	// StartVoiceprintScenarioOnDevice starts voiceprint scenario for user on speaker.
	StartVoiceprintScenarioOnDevice(ctx context.Context, user model.User, deviceID string) error

	// DeleteVoiceprintFromDevice sends sk-directive to speaker for removing user voiceprint
	DeleteVoiceprintFromDevice(ctx context.Context, user model.User, deviceID string) error

	// GetHouseholdResidents returns the list of users that have rights on that household
	GetHouseholdResidents(ctx context.Context, user model.User, householdID string) ([]sharingmodel.HouseholdResident, error)

	// LeaveSharedHousehold deletes guest from shared household (as guest)
	LeaveSharedHousehold(ctx context.Context, guest model.User, householdID string) error

	// RenameSharedHousehold renames shared household for guest
	RenameSharedHousehold(ctx context.Context, guestID uint64, householdID string, householdName string) error

	// DeleteGuestsFromHousehold deletes guests from shared household (as owner)
	DeleteGuestsFromHousehold(ctx context.Context, owner model.User, guestIDs []uint64, householdID string) error

	// GetHouseholdSharingLink creates and stores link in database
	// if link already exist - prolongs it and returns old link expiration time
	GetHouseholdSharingLink(ctx context.Context, owner model.User, householdID string) (model.HouseholdSharingLink, timestamp.PastTimestamp, error)

	// AcceptSharingLink decrypts linkID, checks it, stores user and creates householdInvitation
	AcceptSharingLink(ctx context.Context, guest model.User, encryptedLinkID string) (model.HouseholdInvitation, error)

	// DeleteSharingLinks invalidates all actual sharing links to particular household
	DeleteSharingLinks(ctx context.Context, owner model.User, householdID string) error

	// AcceptHouseholdInvitation validates invitation and stores shared household for guest
	AcceptHouseholdInvitation(ctx context.Context, guest model.User, invitationID string, householdName string) error

	// DeclineHouseholdInvitation validates invitation and deletes it
	DeclineHouseholdInvitation(ctx context.Context, guest model.User, invitationID string) error

	// RevokeGuestsHouseholdInvitations deletes invitations by owner and guests
	RevokeGuestsHouseholdInvitations(ctx context.Context, owner model.User, householdID string, guestIDs []uint64) error

	// GetHouseholdInvitation gets invitation and checks it before return
	GetHouseholdInvitation(ctx context.Context, user model.User, invitationID string) (model.HouseholdInvitation, error)

	// GetSharingUser receives user with all blackbox info: avatar, email, etc
	GetSharingUser(ctx context.Context, user model.User, sharingUserID uint64) (sharingmodel.User, error)

	// GetSharingUsers receives users with all blackbox info: avatar, email, etc
	GetSharingUsers(ctx context.Context, user model.User, userIDs []uint64) (sharingmodel.Users, error)

	// deprecated
	// AcceptDeviceSharing starts voiceprint scenario for guest on owner speaker (user).
	// Then sends guest token to the shared device
	AcceptDeviceSharing(ctx context.Context, guest model.User, ownerPUID uint64, deviceID string) error

	// deprecated
	// RevokeDeviceSharing sends sk-directive to speaker for removing user voiceprint
	RevokeDeviceSharing(ctx context.Context, guest model.User, ownerPUID uint64, deviceID string) error
}
