package sharing

import (
	"context"

	"a.yandex-team.ru/alice/iot/bulbasaur/controller/notificator"
	"a.yandex-team.ru/alice/iot/bulbasaur/controller/oauth"
	"a.yandex-team.ru/alice/iot/bulbasaur/controller/sharing/model"
	"a.yandex-team.ru/alice/iot/bulbasaur/db"
	"a.yandex-team.ru/alice/iot/bulbasaur/model"
	"a.yandex-team.ru/alice/library/go/libquasar"
	"a.yandex-team.ru/alice/library/go/timestamp"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/log/ctxlog"
	"a.yandex-team.ru/library/go/core/xerrors"
	"a.yandex-team.ru/library/go/yandex/blackbox"
	"golang.org/x/exp/slices"
)

type Controller struct {
	logger          log.Logger
	blackboxClient  blackbox.Client
	oauthController oauth.IController
	quasarClient    libquasar.IClient
	notificator     notificator.IController
	dbClient        db.DB
}

func NewController(
	logger log.Logger,
	blackboxClient blackbox.Client,
	oauthController oauth.IController,
	quasarClient libquasar.IClient,
	notificator notificator.IController,
	dbClient db.DB,
) IController {
	return &Controller{
		logger:          logger,
		blackboxClient:  blackboxClient,
		oauthController: oauthController,
		quasarClient:    quasarClient,
		notificator:     notificator,
		dbClient:        dbClient,
	}
}

// deprecated, only for tests
func (c *Controller) AcceptDeviceSharing(ctx context.Context, guest model.User, ownerPUID uint64, deviceID string) error {
	deviceOwner, err := c.dbClient.SelectUser(ctx, ownerPUID)
	if err != nil {
		return xerrors.Errorf("failed to load device owner %d: %w", ownerPUID, err)
	}

	sharedDevice, err := c.dbClient.SelectUserDevice(ctx, deviceOwner.ID, deviceID)
	if err != nil {
		return xerrors.Errorf("failed to load device %s: %w", deviceID, err)
	}

	if !model.VoiceprintSpeakers[sharedDevice.Type] {
		return xerrors.Errorf("device %s with type %s doesn't support sharing", sharedDevice.ID, sharedDevice.Type)
	}

	// full scheme is here: https://wiki.yandex-team.ru/users/ispetrukhin/multiacc-deivce-sharing/#polucheniepriglashenijapushi
	// 0. ToDo: check sharing rights from guest to the given device  https://st.yandex-team.ru/IOT-1280

	// 1. Go to Passport (oauth) to issue auth code
	tokenType := oauth.OAuthTokenType
	if model.RequireXTokenSpeakers[sharedDevice.Type] {
		tokenType = oauth.XTokenTokenType
	}
	authCode, err := c.oauthController.IssueAuthCodeForYandexIO(ctx, guest, tokenType)
	if err != nil {
		return xerrors.Errorf("failed to issue auth code from oauth api: %w", err)
	}

	// 2. go to quasar backend to encrypt auth code
	quasarData, err := sharedDevice.QuasarCustomData()
	if err != nil {
		return xerrors.Errorf("failed to get quasar custom data from device %s: %w", sharedDevice.ID, err)
	}
	encryptionResult, err := c.quasarClient.EncryptPayload(
		ctx,
		libquasar.EncryptPayloadRequest{
			Payload:    authCode,
			DeviceID:   quasarData.DeviceID,
			Platform:   quasarData.Platform,
			RSAPadding: selectRSAPadding(sharedDevice.Type),
		},
		guest.Ticket,
	)
	if err != nil {
		return xerrors.Errorf("failed to encrypt auth code on quasar backend: %w", err)
	}

	// 3. Send encrypted auth code to the station
	speechkitDirective := newGuestAddAccountSKDirective(quasarData.DeviceID, encryptionResult, tokenType)
	// station primary key is (puid, device_id). always send push with owner puid
	err = c.notificator.SendSpeechkitDirective(ctx, ownerPUID, quasarData.DeviceID, speechkitDirective)
	if err != nil {
		return xerrors.Errorf("failed to send sk-directive via notificator: %w", err)
	}
	return nil
}

func selectRSAPadding(deviceType model.DeviceType) libquasar.RSAPadding {
	// see https://st.yandex-team.ru/QUASARINFRA-149#5ef9d7c7ab548b27dd674f6f
	switch deviceType {
	case model.YandexStationDeviceType, model.YandexStationMiniDeviceType:
		return libquasar.Pkcs1OaepRSAPadding
	default:
		// stations starting from Yandex Max require another RSA Padding
		return libquasar.Pkcs1RSAPadding
	}
}

func newGuestAddAccountSKDirective(endpointID string, encryption libquasar.EncryptPayloadResponse, tokenType oauth.TokenType) notificator.SpeechkitDirective {
	return &AddAccountSKDirective{
		endpointID: endpointID,

		EncryptedCode:       encryption.Base64CipherText,
		EncryptedSessionKey: encryption.Base64SessionKey,
		Signature:           encryption.Base64Signature,
		TokenType:           tokenTypeForDirective(tokenType),
		UserType:            GuestType,
	}
}

func newOwnerAddAccountSKDirective(endpointID string) notificator.SpeechkitDirective {
	return &AddAccountSKDirective{
		endpointID: endpointID,
		UserType:   OwnerType,
	}
}

func newBioStartSoundEnrollmentSKDirective(endpointID string) notificator.SpeechkitDirective {
	return &BioStartSoundEnrollmentSKDirective{
		endpointID: endpointID,
	}
}

// deprecated, only for tests
func (c *Controller) RevokeDeviceSharing(ctx context.Context, guest model.User, ownerPUID uint64, deviceID string) error {
	deviceOwner, err := c.dbClient.SelectUser(ctx, ownerPUID)
	if err != nil {
		return xerrors.Errorf("failed to load device owner %d: %w", ownerPUID, err)
	}

	sharedDevice, err := c.dbClient.SelectUserDevice(ctx, deviceOwner.ID, deviceID)
	if err != nil {
		return xerrors.Errorf("failed to load device %s: %w", deviceID, err)
	}

	quasarData, err := sharedDevice.QuasarCustomData()
	if err != nil {
		return xerrors.Errorf("failed to get quasar custom data from device %s: %w", sharedDevice.ID, err)
	}

	speechkitDirective := &RemoveAccountSKDirective{
		endpointID: quasarData.DeviceID,

		PUID: guest.ID,
	}
	// station primary key is (puid, device_id). always send push with owner puid
	err = c.notificator.SendSpeechkitDirective(ctx, ownerPUID, quasarData.DeviceID, speechkitDirective)
	if err != nil {
		return xerrors.Errorf("failed to send sk-directive via notificator: %w", err)
	}
	return nil
}

func (c *Controller) GetHouseholdResidents(ctx context.Context, user model.User, householdID string) ([]sharingmodel.HouseholdResident, error) {
	household, err := c.dbClient.SelectUserHousehold(ctx, user.ID, householdID)
	if err != nil {
		return nil, xerrors.Errorf("failed to select user %d household %s: %w", user.ID, householdID, err)
	}
	residents, err := c.dbClient.SelectHouseholdResidents(ctx, user.ID, household)
	if err != nil {
		return nil, xerrors.Errorf("failed to select household %s residents: %w", household.ID, err)
	}
	users, err := c.GetSharingUsers(ctx, user, residents.IDs())
	if err != nil {
		return nil, xerrors.Errorf("failed to get sharing users: %w", err)
	}
	return sharingmodel.NewHouseholdResidents(residents, users), nil
}

func (c *Controller) GetSharingUsers(ctx context.Context, user model.User, userIDs []uint64) (sharingmodel.Users, error) {
	if len(userIDs) == 0 {
		return nil, nil
	}
	userInfoRequest := blackbox.UserInfoRequest{
		UIDs:   userIDs,
		UserIP: user.IP,
		Attributes: []blackbox.UserAttribute{
			blackbox.UserAttributeAccountNormalizedLogin,
			blackbox.UserAttributeAvatarDefault,
			blackbox.UserAttributeAccountHavePlus,
		},
		GetPhones: blackbox.GetPhonesAll,
		PhoneAttributes: []blackbox.PhoneAttribute{
			blackbox.PhoneAttributePhoneMaskedFormattedNumber,
			blackbox.PhoneAttributePhoneIsDefault,
		},
		RegName:       true,
		GetPublicName: true,
		Emails:        blackbox.EmailsGetDefault,
	}
	userInfos, err := c.blackboxClient.UserInfo(ctx, userInfoRequest)
	if err != nil {
		return nil, xerrors.Errorf("failed to get user info from blackbox: %w", err)
	}
	result := make(sharingmodel.Users, 0, len(userIDs))
	for _, blackboxUser := range userInfos.Users {
		var user sharingmodel.User
		user.FromBlackboxUser(blackboxUser)
		result = append(result, user)
	}
	return result, nil
}

func (c *Controller) LeaveSharedHousehold(ctx context.Context, guest model.User, householdID string) error {
	household, err := c.dbClient.SelectUserHousehold(ctx, guest.ID, householdID)
	if err != nil {
		return xerrors.Errorf("failed to select shared household %s: %w", householdID, err)
	}
	if household.SharingInfo == nil {
		return xerrors.Errorf("household %s is not shared to user: %w", householdID, &model.SharedHouseholdOwnerLeavingError{})
	}
	return c.dbClient.DeleteSharedHousehold(ctx, household.SharingInfo.OwnerID, guest.ID, householdID)
}

func (c *Controller) DeleteGuestsFromHousehold(ctx context.Context, owner model.User, guestIDs []uint64, householdID string) error {
	_, err := c.dbClient.SelectUserHousehold(ctx, owner.ID, householdID)
	if err != nil {
		return xerrors.Errorf("failed to select household %s: %w", householdID, err)
	}

	// trivial approach for now
	for _, guestID := range guestIDs {
		if err := c.dbClient.DeleteSharedHousehold(ctx, owner.ID, guestID, householdID); err != nil {
			return xerrors.Errorf("failed to delete guest %d from shared household %s: %w", guestID, householdID, err)
		}
	}
	return nil
}

func (c *Controller) StartVoiceprintScenarioOnDevice(ctx context.Context, user model.User, deviceID string) error {
	device, err := c.dbClient.SelectUserDevice(ctx, user.ID, deviceID)
	if err != nil {
		return xerrors.Errorf("failed to load device %s: %w", deviceID, err)
	}

	if !model.VoiceprintSpeakers[device.Type] {
		return xerrors.Errorf("device %s with type %s doesn't support sharing", device.ID, device.Type)
	}

	quasarData, err := device.QuasarCustomData()
	if err != nil {
		return xerrors.Errorf("failed to get quasar custom data from device %s: %w", device.ID, err)
	}

	speakerOwnerID := user.ID
	if device.SharingInfo != nil {
		speakerOwnerID = device.SharingInfo.OwnerID
	}

	// for voiceprint via sound speakers - send bio sound enrollment directive
	if model.VoiceprintViaSoundSpeakers[device.Type] {
		ctxlog.Infof(ctx, c.logger, "device %s voiceprint saving method is sound, send BioStartSoundEnrollment directive", deviceID)
		speechkitDirective := newBioStartSoundEnrollmentSKDirective(quasarData.DeviceID)
		err = c.notificator.SendSpeechkitDirective(ctx, speakerOwnerID, quasarData.DeviceID, speechkitDirective)
		if err != nil {
			return xerrors.Errorf("failed to send sk-directive via notificator: %w", notificatorErrorWrapper(err))
		}
		return nil
	}

	if device.SharingInfo == nil {
		// user is device owner
		// we do not need to encrypt anything in that case
		ctxlog.Infof(ctx, c.logger, "user %d is device %s owner, send add_account directive as owner", user.ID, deviceID)
		speechkitDirective := newOwnerAddAccountSKDirective(quasarData.DeviceID)
		// station primary key is (puid, device_id). always send push with owner puid
		err = c.notificator.SendSpeechkitDirective(ctx, speakerOwnerID, quasarData.DeviceID, speechkitDirective)
		if err != nil {
			return xerrors.Errorf("failed to send sk-directive via notificator: %w", notificatorErrorWrapper(err))
		}
		return nil
	}

	// full scheme is here: https://wiki.yandex-team.ru/users/ispetrukhin/multiacc-deivce-sharing/#polucheniepriglashenijapushi
	// 1. Go to Passport (oauth) to issue auth code
	tokenType := oauth.OAuthTokenType
	if model.RequireXTokenSpeakers[device.Type] {
		tokenType = oauth.XTokenTokenType
	}
	authCode, err := c.oauthController.IssueAuthCodeForYandexIO(ctx, user, tokenType)
	if err != nil {
		return xerrors.Errorf("failed to issue auth code from oauth api: %w", err)
	}

	// 2. go to quasar backend to encrypt auth code
	encryptionResult, err := c.quasarClient.EncryptPayload(
		ctx,
		libquasar.EncryptPayloadRequest{
			Payload:    authCode,
			DeviceID:   quasarData.DeviceID,
			Platform:   quasarData.Platform,
			RSAPadding: selectRSAPadding(device.Type),
		},
		user.Ticket,
	)
	if err != nil {
		return xerrors.Errorf("failed to encrypt auth code on quasar backend: %w", err)
	}

	// 3. Send encrypted auth code to the station
	speechkitDirective := newGuestAddAccountSKDirective(quasarData.DeviceID, encryptionResult, tokenType)
	// station primary key is (puid, device_id). always send push with owner puid
	err = c.notificator.SendSpeechkitDirective(ctx, speakerOwnerID, quasarData.DeviceID, speechkitDirective)
	if err != nil {
		return xerrors.Errorf("failed to send sk-directive via notificator: %w", notificatorErrorWrapper(err))
	}
	return nil
}

func (c *Controller) DeleteVoiceprintFromDevice(ctx context.Context, user model.User, deviceID string) error {
	device, err := c.dbClient.SelectUserDevice(ctx, user.ID, deviceID)
	if err != nil {
		return xerrors.Errorf("failed to load device %s: %w", deviceID, err)
	}

	quasarData, err := device.QuasarCustomData()
	if err != nil {
		return xerrors.Errorf("failed to get quasar custom data from device %s: %w", device.ID, err)
	}

	speechkitDirective := &RemoveAccountSKDirective{
		endpointID: quasarData.DeviceID,

		PUID: user.ID,
	}
	// station primary key is (puid, device_id). always send push with owner puid
	ownerID := user.ID
	if device.SharingInfo != nil {
		ownerID = device.SharingInfo.OwnerID
	}
	err = c.notificator.SendSpeechkitDirective(ctx, ownerID, quasarData.DeviceID, speechkitDirective)
	if err != nil {
		return xerrors.Errorf("failed to send sk-directive via notificator: %w", notificatorErrorWrapper(err))
	}
	return nil
}

func (c *Controller) GetHouseholdSharingLink(ctx context.Context, user model.User, householdID string) (model.HouseholdSharingLink, timestamp.PastTimestamp, error) {
	link, err := c.dbClient.SelectHouseholdSharingLink(ctx, user.ID, householdID)
	if err != nil {
		if xerrors.Is(err, &model.SharingLinkDoesNotExistError{}) {
			link = model.NewHouseholdSharingLink(ctx, user.ID, householdID)
		} else {
			return model.HouseholdSharingLink{}, 0, xerrors.Errorf("failed to select household %s sharing link: %w", householdID, err)
		}
	}
	previousExpirationTime := link.ExpireAt
	link.ExpireAt = timestamp.CurrentTimestampCtx(ctx).Add(model.LinkDefaultTTL)
	if err := c.dbClient.StoreHouseholdSharingLink(ctx, link); err != nil {
		return model.HouseholdSharingLink{}, 0, xerrors.Errorf("failed to store household %s sharing link: %w", householdID, err)
	}
	return link, previousExpirationTime, nil
}

func (c *Controller) AcceptSharingLink(ctx context.Context, guest model.User, encryptedLinkID string) (model.HouseholdInvitation, error) {
	linkID, err := model.DecodeHouseholdSharingLinkID(encryptedLinkID)
	if err != nil {
		return model.HouseholdInvitation{}, xerrors.Errorf("failed to decrypt link id: %w", err)
	}
	link, err := c.dbClient.SelectHouseholdSharingLinkByID(ctx, linkID)
	if err != nil {
		return model.HouseholdInvitation{}, xerrors.Errorf("failed to select household sharing link %s: %w", linkID, err)
	}
	if link.SenderID == guest.ID {
		return model.HouseholdInvitation{}, xerrors.Errorf("failed to accept sharing link: guest is owner of household %s: %w", link.HouseholdID, &model.SharingLinkNeedlessAcceptanceError{})
	}
	sharingInfos, err := c.dbClient.SelectGuestSharingInfos(ctx, guest.ID)
	if err != nil {
		return model.HouseholdInvitation{}, xerrors.Errorf("failed to select guest sharing infos: %w", err)
	}
	if slices.Contains(sharingInfos.HouseholdIDs(), link.HouseholdID) {
		return model.HouseholdInvitation{}, xerrors.Errorf("failed to accept sharing link: household %s already shared to guest: %w", link.HouseholdID, &model.SharingLinkNeedlessAcceptanceError{})
	}
	// we need to store user because it creates default household for him beforehand
	if err := c.dbClient.StoreUser(ctx, guest); err != nil {
		return model.HouseholdInvitation{}, xerrors.Errorf("failed to store user: %w", err)
	}
	invitation := link.ToHouseholdInvitation(guest.ID)
	if err := c.dbClient.StoreHouseholdInvitation(ctx, invitation); err != nil {
		return model.HouseholdInvitation{}, xerrors.Errorf("failed to store household invitation: %w", err)
	}
	return invitation, nil
}

func (c *Controller) DeleteSharingLinks(ctx context.Context, owner model.User, householdID string) error {
	if err := c.dbClient.DeleteHouseholdSharingLinks(ctx, owner.ID, householdID); err != nil {
		return xerrors.Errorf("failed to delete household %s sharing links: %w", householdID, err)
	}
	return nil
}

func (c *Controller) AcceptHouseholdInvitation(ctx context.Context, guest model.User, invitationID string, householdName string) error {
	return c.dbClient.Transaction(ctx, "accept-household-invitation", func(ctx context.Context) error {
		invitation, err := c.dbClient.SelectHouseholdInvitationByID(ctx, invitationID)
		if err != nil {
			return xerrors.Errorf("failed to select household invitation: %w", err)
		}
		if err := c.validateHouseholdInvitationAcceptanceForGuest(ctx, guest.ID, invitation); err != nil {
			return xerrors.Errorf("failed to validate household invitation for guest: %w", err)
		}
		if err := c.dbClient.StoreSharedHousehold(ctx, guest.ID, invitation.ToSharingInfo(householdName)); err != nil {
			return xerrors.Errorf("failed to store shared household: %w", err)
		}
		if err := c.dbClient.DeleteHouseholdInvitation(ctx, invitation); err != nil {
			return xerrors.Errorf("failed to delete household invitation: %w", err)
		}
		return nil
	})
}

func (c *Controller) DeclineHouseholdInvitation(ctx context.Context, guest model.User, invitationID string) error {
	invitation, err := c.dbClient.SelectHouseholdInvitationByID(ctx, invitationID)
	if err != nil {
		return xerrors.Errorf("failed to select household invitation: %w", err)
	}
	if err := c.validateHouseholdInvitationOwnershipForGuest(ctx, guest.ID, invitation); err != nil {
		return xerrors.Errorf("failed to validate household invitation for guest: %w", err)
	}
	if err := c.dbClient.DeleteHouseholdInvitation(ctx, invitation); err != nil {
		return xerrors.Errorf("failed to delete household invitation: %w", err)
	}
	return nil
}

func (c *Controller) GetHouseholdInvitation(ctx context.Context, user model.User, invitationID string) (model.HouseholdInvitation, error) {
	invitation, err := c.dbClient.SelectHouseholdInvitationByID(ctx, invitationID)
	if err != nil {
		return model.HouseholdInvitation{}, xerrors.Errorf("failed to select household invitation: %w", err)
	}
	if err := c.validateHouseholdInvitationOwnershipForGuest(ctx, user.ID, invitation); err != nil {
		return model.HouseholdInvitation{}, xerrors.Errorf("failed to validate household invitation for guest: %w", err)
	}
	return invitation, nil
}

func (c *Controller) RevokeGuestsHouseholdInvitations(ctx context.Context, owner model.User, householdID string, guestIDs []uint64) error {
	return c.dbClient.Transaction(ctx, "revoke-guests-household-invitations", func(ctx context.Context) error {
		invitations, err := c.dbClient.SelectHouseholdInvitationsBySender(ctx, owner.ID)
		if err != nil {
			return xerrors.Errorf("failed to select household invitation: %w", err)
		}
		// FIXME: trivial approach for now
		for _, guestID := range guestIDs {
			invitation, exist := invitations.GetByUsersAndHouseholdID(owner.ID, guestID, householdID)
			if !exist {
				ctxlog.Warnf(ctx, c.logger, "invitation to household %s to guest %d does not exist, skip it", householdID, guestID)
			}
			if err := c.dbClient.DeleteHouseholdInvitation(ctx, invitation); err != nil {
				return xerrors.Errorf("failed to delete household %s invitation %s: %w", householdID, invitation.ID, err)
			}
		}
		return nil
	})
}

func (c *Controller) RenameSharedHousehold(ctx context.Context, guestID uint64, householdID string, householdName string) error {
	return c.dbClient.RenameSharedHousehold(ctx, guestID, householdID, householdName)
}

func (c *Controller) validateHouseholdInvitationOwnershipForGuest(ctx context.Context, guestID uint64, invitation model.HouseholdInvitation) error {
	if _, err := c.dbClient.SelectUserHousehold(ctx, invitation.SenderID, invitation.HouseholdID); err != nil {
		return xerrors.Errorf("no such household %s: %w", invitation.HouseholdID, err)
	}
	if guestID != invitation.GuestID {
		return xerrors.Errorf("invitation is not belong to that guest: %w", &model.SharingInvitationDoesNotOwnedByUserError{})
	}
	return nil
}

func (c *Controller) validateHouseholdInvitationAcceptanceForGuest(ctx context.Context, guestID uint64, invitation model.HouseholdInvitation) error {
	if err := c.validateHouseholdInvitationOwnershipForGuest(ctx, guestID, invitation); err != nil {
		return xerrors.Errorf("failed to validate household invitation for guest: %w", err)
	}
	devices, err := c.dbClient.SelectUserDevicesSimple(ctx, guestID)
	if err != nil {
		return xerrors.Errorf("failed to select user %d devices: %w", guestID, err)
	}
	newSharedHouseholdDevices, err := c.dbClient.SelectUserHouseholdDevicesSimple(ctx, invitation.SenderID, invitation.HouseholdID)
	if err != nil {
		return xerrors.Errorf("failed to select user household %s devices: %w", invitation.HouseholdID, err)
	}
	if uint64(len(devices)+len(newSharedHouseholdDevices)) >= model.ConstDeviceLimit {
		return xerrors.Errorf("failed to accept household invitation: %w", &model.SharingDevicesLimitReachedError{})
	}
	guestSharingInfos, err := c.dbClient.SelectGuestSharingInfos(ctx, guestID)
	if err != nil {
		return xerrors.Errorf("failed to select guest sharing infos: %w", err)
	}
	ownerMap := guestSharingInfos.ToOwnerMap()
	if len(ownerMap) == model.ConstSharingUsersLimit {
		return xerrors.Errorf("failed to accept household invitation: %w", &model.SharingUsersLimitReachedError{})
	}
	return nil
}

func (c *Controller) GetSharingUser(ctx context.Context, user model.User, sharingUserID uint64) (sharingmodel.User, error) {
	users, err := c.GetSharingUsers(ctx, user, []uint64{sharingUserID})
	if err != nil {
		return sharingmodel.User{}, xerrors.Errorf("failed to get users: %w", err)
	}
	if len(users) != 1 {
		return sharingmodel.User{}, xerrors.Errorf("failed to get users: len of users is not 1")
	}
	return users[0], nil
}

func notificatorErrorWrapper(err error) error {
	if xerrors.Is(err, notificator.DeviceOfflineError) {
		return &model.DeviceUnreachableError{}
	}
	return err
}
