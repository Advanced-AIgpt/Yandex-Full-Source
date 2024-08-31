package model

import (
	"context"
	"encoding/base64"
	"time"

	"a.yandex-team.ru/alice/iot/bulbasaur/protos"
	"a.yandex-team.ru/alice/library/go/timestamp"
	"a.yandex-team.ru/alice/protos/data/device"
	"a.yandex-team.ru/library/go/core/xerrors"
	"github.com/gofrs/uuid"
)

type SharingInfo struct {
	OwnerID       uint64
	HouseholdID   string
	HouseholdName string
}

func NewSharingInfo(ownerID uint64, householdID string, householdName string) *SharingInfo {
	return &SharingInfo{
		OwnerID:       ownerID,
		HouseholdID:   householdID,
		HouseholdName: householdName,
	}
}

func (si *SharingInfo) Clone() *SharingInfo {
	if si == nil {
		return nil
	}
	return &SharingInfo{
		OwnerID:       si.OwnerID,
		HouseholdID:   si.HouseholdID,
		HouseholdName: si.HouseholdName,
	}
}

func (si *SharingInfo) FromProto(protoSharing *protos.SharingInfo) {
	if protoSharing == nil {
		return
	}
	si.OwnerID = protoSharing.OwnerID
	si.HouseholdID = protoSharing.HouseholdID
}

func (si *SharingInfo) ToProto() *protos.SharingInfo {
	if si == nil {
		return nil
	}
	return &protos.SharingInfo{
		OwnerID:     si.OwnerID,
		HouseholdID: si.HouseholdID,
	}
}

func (si *SharingInfo) fromUserInfoProto(protoSharing *device.TUserSharingInfo) {
	if protoSharing == nil {
		return
	}
	si.OwnerID = protoSharing.GetOwnerID()
	si.HouseholdID = protoSharing.GetHouseholdID()
}

func (si *SharingInfo) ToUserInfoProto() *device.TUserSharingInfo {
	if si == nil {
		return nil
	}
	return &device.TUserSharingInfo{
		OwnerID:     si.OwnerID,
		HouseholdID: si.HouseholdID,
	}
}

type SharingInfos []SharingInfo

func (si SharingInfos) ToOwnerMap() map[uint64]SharingInfos {
	result := make(map[uint64]SharingInfos)
	for _, sharingInfo := range si {
		result[sharingInfo.OwnerID] = append(result[sharingInfo.OwnerID], sharingInfo)
	}
	return result
}

func (si SharingInfos) ToHouseholdMap() map[string]SharingInfo {
	result := make(map[string]SharingInfo)
	for _, sharingInfo := range si {
		result[sharingInfo.HouseholdID] = sharingInfo
	}
	return result
}

func (si SharingInfos) HouseholdIDs() []string {
	result := make([]string, 0, len(si))
	for _, sharingInfo := range si {
		result = append(result, sharingInfo.HouseholdID)
	}
	return result
}

type HouseholdRole string

const (
	OwnerHouseholdRole             HouseholdRole = "owner"
	GuestHouseholdRole             HouseholdRole = "guest"
	PendingInvitationHouseholdRole HouseholdRole = "pending_invitation"
)

type HouseholdResident struct {
	ID   uint64
	Role HouseholdRole
}

func NewHouseholdResident(userID uint64, role HouseholdRole) HouseholdResident {
	return HouseholdResident{
		userID,
		role,
	}
}

type HouseholdResidents []HouseholdResident

func (residents HouseholdResidents) IDs() []uint64 {
	result := make([]uint64, 0, len(residents))
	for _, resident := range residents {
		result = append(result, resident.ID)
	}
	return result
}

func (residents HouseholdResidents) GuestsOrPendingInvitations() HouseholdResidents {
	result := make(HouseholdResidents, 0, len(residents))
	for _, resident := range residents {
		if resident.Role == GuestHouseholdRole || resident.Role == PendingInvitationHouseholdRole {
			result = append(result, resident)
		}
	}
	return result
}

type HouseholdInvitation struct {
	ID          string
	SenderID    uint64
	HouseholdID string
	GuestID     uint64
}

func (invitation HouseholdInvitation) ToSharingInfo(householdName string) SharingInfo {
	sharingInfo := NewSharingInfo(invitation.SenderID, invitation.HouseholdID, householdName)
	return *sharingInfo
}

type HouseholdInvitations []HouseholdInvitation

func (invitations HouseholdInvitations) GetByUsersAndHouseholdID(senderID uint64, guestID uint64, householdID string) (HouseholdInvitation, bool) {
	for _, invitation := range invitations {
		if invitation.SenderID == senderID && invitation.HouseholdID == householdID && invitation.GuestID == guestID {
			return invitation, true
		}
	}
	return HouseholdInvitation{}, false
}

func (invitations HouseholdInvitations) SendersIDs() []uint64 {
	result := make([]uint64, 0, len(invitations))
	for _, invitation := range invitations {
		result = append(result, invitation.SenderID)
	}
	return result
}

const LinkDefaultTTL = 24 * time.Hour

type HouseholdSharingLink struct {
	SenderID    uint64
	HouseholdID string
	ID          string
	ExpireAt    timestamp.PastTimestamp
}

func NewHouseholdSharingLink(ctx context.Context, senderID uint64, householdID string) HouseholdSharingLink {
	currentTS := timestamp.CurrentTimestampCtx(ctx)
	return HouseholdSharingLink{
		SenderID:    senderID,
		HouseholdID: householdID,
		ID:          uuid.Must(uuid.NewV4()).String(),
		ExpireAt:    currentTS.Add(LinkDefaultTTL),
	}
}

func (l HouseholdSharingLink) ConstructSharingLink() string {
	return "https://3944830.redirect.appmetrica.yandex.com/?url=https%3A%2F%2Fyandex.ru%2Fiot%2Fexternal%2Fsharing-invite%3Ftoken%3D" +
		EncodeHouseholdSharingLinkID(l.ID) + "&appmetrica_tracking_id=1108645433053365876&token=" + EncodeHouseholdSharingLinkID(l.ID) + "&referrer=reattribution%3D1"
}

func EncodeHouseholdSharingLinkID(linkID string) string {
	// TODO: encrypt link id with something
	encodedLinkID := base64.URLEncoding.EncodeToString([]byte(linkID))
	return encodedLinkID
}

func DecodeHouseholdSharingLinkID(encryptedLinkID string) (string, error) {
	result := make([]byte, len(encryptedLinkID))
	_, err := base64.URLEncoding.Decode(result, []byte(encryptedLinkID))
	if err != nil {
		return "", xerrors.Errorf("failed to decode base64 link: %w", &SharingLinkDoesNotExistError{})
	}
	uuidV4Len := len(uuid.Must(uuid.NewV4()).String())
	if len(result) < uuidV4Len {
		return "", &SharingLinkDoesNotExistError{}
	}
	result = result[:uuidV4Len]
	return string(result), nil
}

func (l HouseholdSharingLink) ToHouseholdInvitation(guestID uint64) HouseholdInvitation {
	return HouseholdInvitation{
		ID:          uuid.Must(uuid.NewV4()).String(),
		SenderID:    l.SenderID,
		HouseholdID: l.HouseholdID,
		GuestID:     guestID,
	}
}
