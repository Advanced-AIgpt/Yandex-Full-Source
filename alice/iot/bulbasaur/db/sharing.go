package db

import (
	"bytes"
	"context"
	"time"

	"a.yandex-team.ru/alice/library/go/timestamp"
	"golang.org/x/exp/slices"

	"a.yandex-team.ru/alice/iot/bulbasaur/model"
	"a.yandex-team.ru/alice/library/go/tools"
	"a.yandex-team.ru/kikimr/public/sdk/go/ydb"
	"a.yandex-team.ru/kikimr/public/sdk/go/ydb/table"
	"a.yandex-team.ru/library/go/core/xerrors"
)

func (db *DBClient) SelectGuestSharingInfos(ctx context.Context, guestID uint64) (model.SharingInfos, error) {
	res, err := db.Read(ctx, db.PragmaPrefix(`
		DECLARE $guest_huid AS UInt64;
		SELECT
			owner_id, household_id, household_name
		FROM
			SharedHouseholds
		WHERE
			guest_huid == $guest_huid;
	`), table.NewQueryParameters(table.ValueParam("$guest_huid", ydb.Uint64Value(tools.Huidify(guestID)))))
	if err != nil {
		return nil, xerrors.Errorf("failed to select user %d sharing info: %w", guestID, err)
	}
	result := make(model.SharingInfos, 0)
	for res.NextResultSet(ctx, "owner_id", "household_id", "household_name") {
		for res.NextRow() {
			var sharingInfo model.SharingInfo
			if err := res.ScanWithDefaults(&sharingInfo.OwnerID, &sharingInfo.HouseholdID, &sharingInfo.HouseholdName); err != nil {
				return nil, xerrors.Errorf("failed to scan result for user: %w", err)
			}
			result = append(result, sharingInfo)
		}
	}
	return result, nil
}

func (db *DBClient) DeleteSharedHousehold(ctx context.Context, ownerID uint64, guestID uint64, householdID string) error {
	query := db.PragmaPrefix(`
			DECLARE $guest_huid AS Uint64;
			DECLARE $owner_huid AS Uint64;
			DECLARE $household_id AS String;
			DELETE FROM
				SharedHouseholds
			WHERE
				guest_huid == $guest_huid and
				owner_huid == $owner_huid and
				household_id == $household_id`)
	params := table.NewQueryParameters(
		table.ValueParam("$guest_huid", ydb.Uint64Value(tools.Huidify(guestID))),
		table.ValueParam("$owner_huid", ydb.Uint64Value(tools.Huidify(ownerID))),
		table.ValueParam("$household_id", ydb.StringValue([]byte(householdID))),
	)
	return db.Write(ctx, query, params)
}

func (db *DBClient) StoreSharedHousehold(ctx context.Context, guestID uint64, sharingInfo model.SharingInfo) error {
	upsertFunc := func(ctx context.Context, s *table.Session, txControl *table.TransactionControl) (*table.Transaction, error) {
		tx, exist, err := db.checkUserHouseholdExistTx(ctx, s, txControl, HouseholdQueryCriteria{UserID: sharingInfo.OwnerID, HouseholdID: sharingInfo.HouseholdID})
		if err != nil {
			return tx, xerrors.Errorf("failed to get user %d household %s: %w", sharingInfo.OwnerID, sharingInfo.HouseholdID, err)
		}
		if !exist {
			return nil, &model.UserHouseholdNotFoundError{}
		}
		allHouseholds, err := db.getUserHouseholdsInTx(ctx, s, tx, HouseholdQueryCriteria{UserID: guestID, WithShared: true})
		if err != nil {
			return tx, xerrors.Errorf("failed to select user households: %w", err)
		}
		testHousehold := model.Household{
			Name: sharingInfo.HouseholdName,
		}
		if err := testHousehold.ValidateName(allHouseholds); err != nil {
			return nil, xerrors.Errorf("failed to validate shared household name: %w", err)
		}
		query := db.PragmaPrefix(`
			DECLARE $guest_huid AS Uint64;
			DECLARE $guest_id AS Uint64;
			DECLARE $owner_huid AS Uint64;
			DECLARE $owner_id AS Uint64;
			DECLARE $household_id AS String;
			DECLARE $household_name AS String;
			UPSERT INTO
				SharedHouseholds (guest_huid, guest_id, owner_huid, owner_id, household_id, household_name)
			VALUES
				($guest_huid, $guest_id, $owner_huid, $owner_id, $household_id, $household_name);`)
		params := table.NewQueryParameters(
			table.ValueParam("$guest_huid", ydb.Uint64Value(tools.Huidify(guestID))),
			table.ValueParam("$guest_id", ydb.Uint64Value(guestID)),
			table.ValueParam("$owner_huid", ydb.Uint64Value(tools.Huidify(sharingInfo.OwnerID))),
			table.ValueParam("$owner_id", ydb.Uint64Value(sharingInfo.OwnerID)),
			table.ValueParam("$household_id", ydb.StringValue([]byte(sharingInfo.HouseholdID))),
			table.ValueParam("$household_name", ydb.StringValue([]byte(sharingInfo.HouseholdName))),
		)
		upsertStmt, err := s.Prepare(ctx, query)
		if err != nil {
			return nil, xerrors.Errorf("failed to prepare statement: %w", err)
		}
		if _, err := tx.ExecuteStatement(ctx, upsertStmt, params); err != nil {
			return tx, xerrors.Errorf("failed to share household %s to guest %d: %w", sharingInfo.HouseholdID, guestID, err)
		}
		return tx, nil
	}
	return db.CallInTx(ctx, SerializableReadWrite, upsertFunc)
}

func (db *DBClient) RenameSharedHousehold(ctx context.Context, guestID uint64, householdID string, householdName string) error {
	upsertFunc := func(ctx context.Context, s *table.Session, txControl *table.TransactionControl) (*table.Transaction, error) {
		tx, allHouseholds, err := db.getUserHouseholdsTx(ctx, s, txControl, HouseholdQueryCriteria{UserID: guestID, WithShared: true})
		if err != nil {
			return tx, xerrors.Errorf("failed to select user households: %w", err)
		}
		testHousehold := model.Household{
			ID:   householdID,
			Name: householdName,
		}
		if err := testHousehold.ValidateName(allHouseholds); err != nil {
			return nil, xerrors.Errorf("failed to validate shared household name: %w", err)
		}
		sharingInfos, err := db.selectGuestSharingInfosInTx(ctx, s, tx, guestID)
		if err != nil {
			return nil, xerrors.Errorf("failed to select guest sharing infos: %w", err)
		}
		sharingInfoByHouseholdMap := sharingInfos.ToHouseholdMap()
		householdSharingInfo, ok := sharingInfoByHouseholdMap[householdID]
		if !ok {
			return nil, xerrors.Errorf("failed to rename shared household %s: guest do not have access to household: %w", householdID, &model.UserHouseholdNotFoundError{})
		}
		query := db.PragmaPrefix(`
			DECLARE $guest_huid AS Uint64;
			DECLARE $guest_id AS Uint64;
			DECLARE $owner_huid AS Uint64;
			DECLARE $owner_id AS Uint64;
			DECLARE $household_id AS String;
			DECLARE $household_name AS String;
			UPSERT INTO
				SharedHouseholds (guest_huid, guest_id, owner_huid, owner_id, household_id, household_name)
			VALUES
				($guest_huid, $guest_id, $owner_huid, $owner_id, $household_id, $household_name);`)
		params := table.NewQueryParameters(
			table.ValueParam("$guest_huid", ydb.Uint64Value(tools.Huidify(guestID))),
			table.ValueParam("$guest_id", ydb.Uint64Value(guestID)),
			table.ValueParam("$owner_huid", ydb.Uint64Value(tools.Huidify(householdSharingInfo.OwnerID))),
			table.ValueParam("$owner_id", ydb.Uint64Value(householdSharingInfo.OwnerID)),
			table.ValueParam("$household_id", ydb.StringValue([]byte(householdID))),
			table.ValueParam("$household_name", ydb.StringValue([]byte(householdName))),
		)
		upsertStmt, err := s.Prepare(ctx, query)
		if err != nil {
			return nil, xerrors.Errorf("failed to prepare statement: %w", err)
		}
		if _, err := tx.ExecuteStatement(ctx, upsertStmt, params); err != nil {
			return tx, xerrors.Errorf("failed to rename shared household %s for guest %d: %w", householdID, guestID, err)
		}
		return tx, nil
	}
	return db.CallInTx(ctx, SerializableReadWrite, upsertFunc)
}

func (db *DBClient) SelectHouseholdResidents(ctx context.Context, userID uint64, household model.Household) (model.HouseholdResidents, error) {
	ownerID := userID
	if household.SharingInfo != nil {
		ownerID = household.SharingInfo.OwnerID
	}
	res, err := db.Read(ctx, db.PragmaPrefix(`
		--!syntax_v1
		DECLARE $owner_huid AS Uint64;
		DECLARE $household_id AS String;

		SELECT
			guest_id
		FROM
			SharedHouseholds VIEW shared_households_owner_huid
		WHERE
			owner_huid == $owner_huid AND
			household_id == $household_id;

		SELECT
			guest_id
		FROM
			SharedHouseholdsInvitations VIEW shared_households_invitations_sender_huid
		WHERE
			sender_huid == $owner_huid AND
			household_id == $household_id;
	`), table.NewQueryParameters(
		table.ValueParam("$owner_huid", ydb.Uint64Value(tools.Huidify(ownerID))),
		table.ValueParam("$household_id", ydb.StringValue([]byte(household.ID)))),
	)
	if err != nil {
		return nil, xerrors.Errorf("failed to select shared household %s guests: %w", household.ID, err)
	}
	result := make(model.HouseholdResidents, 0)
	if !res.NextResultSet(ctx, "guest_id") {
		return nil, xerrors.Errorf("failed to select household %s residents: empty result set for guests", household.ID)
	}
	for res.NextRow() {
		var guestID uint64
		if err := res.ScanWithDefaults(&guestID); err != nil {
			return nil, xerrors.Errorf("failed to scan result for user: %w", err)
		}
		result = append(result, model.NewHouseholdResident(guestID, model.GuestHouseholdRole))
	}

	if !res.NextResultSet(ctx, "guest_id") {
		return nil, xerrors.Errorf("failed to select household %s residents: empty result set for pending invitations", household.ID)
	}
	for res.NextRow() {
		var guestID uint64
		if err := res.ScanWithDefaults(&guestID); err != nil {
			return nil, xerrors.Errorf("failed to scan result for user: %w", err)
		}
		result = append(result, model.NewHouseholdResident(guestID, model.PendingInvitationHouseholdRole))
	}
	result = append(result, model.NewHouseholdResident(ownerID, model.OwnerHouseholdRole))
	return result, nil
}

func (db *DBClient) StoreHouseholdSharingLink(ctx context.Context, link model.HouseholdSharingLink) error {
	upsertFunc := func(ctx context.Context, s *table.Session, txControl *table.TransactionControl) (*table.Transaction, error) {
		tx, exist, err := db.checkUserHouseholdExistTx(ctx, s, txControl, HouseholdQueryCriteria{UserID: link.SenderID, HouseholdID: link.HouseholdID})
		if err != nil {
			return tx, xerrors.Errorf("failed to get user %d household %s: %w", link.SenderID, link.HouseholdID, err)
		}
		if !exist {
			return nil, &model.UserHouseholdNotFoundError{}
		}
		query := db.PragmaPrefix(`
			DECLARE $sender_huid AS Uint64;
			DECLARE $sender_id AS Uint64;
			DECLARE $id AS String;
			DECLARE $household_id AS String;
			DECLARE $expire_at AS Timestamp;
			UPSERT INTO
				SharedHouseholdsLinks (sender_huid, sender_id, id, household_id, expire_at)
			VALUES
				($sender_huid, $sender_id, $id, $household_id, $expire_at);`)
		params := table.NewQueryParameters(
			table.ValueParam("$sender_huid", ydb.Uint64Value(tools.Huidify(link.SenderID))),
			table.ValueParam("$sender_id", ydb.Uint64Value(link.SenderID)),
			table.ValueParam("$id", ydb.StringValue([]byte(link.ID))),
			table.ValueParam("$household_id", ydb.StringValue([]byte(link.HouseholdID))),
			table.ValueParam("$expire_at", ydb.TimestampValue(link.ExpireAt.YdbTimestamp())),
		)
		upsertStmt, err := s.Prepare(ctx, query)
		if err != nil {
			return nil, xerrors.Errorf("failed to prepare statement: %w", err)
		}
		if _, err := tx.ExecuteStatement(ctx, upsertStmt, params); err != nil {
			return tx, xerrors.Errorf("failed to store household %s link from sender %d: %w", link.HouseholdID, link.SenderID, err)
		}
		return tx, nil
	}
	return db.CallInTx(ctx, SerializableReadWrite, upsertFunc)
}

func (db *DBClient) SelectHouseholdSharingLinkByID(ctx context.Context, linkID string) (model.HouseholdSharingLink, error) {
	res, err := db.Read(ctx, db.PragmaPrefix(`
		--!syntax_v1
		DECLARE $id AS String;
		SELECT
			sender_id, id, household_id, expire_at
		FROM
			SharedHouseholdsLinks VIEW shared_households_links_id
		WHERE
			id == $id;
	`),
		table.NewQueryParameters(
			table.ValueParam("$id", ydb.StringValue([]byte(linkID))),
		),
	)
	if err != nil {
		return model.HouseholdSharingLink{}, xerrors.Errorf("failed to select link %s: %w", linkID, err)
	}
	result := make([]model.HouseholdSharingLink, 0)
	for res.NextResultSet(ctx, "id", "sender_id", "household_id", "expire_at") {
		for res.NextRow() {
			var invitation model.HouseholdSharingLink
			var expireAt time.Time
			if err := res.ScanWithDefaults(&invitation.ID, &invitation.SenderID, &invitation.HouseholdID, &expireAt); err != nil {
				return model.HouseholdSharingLink{}, xerrors.Errorf("failed to scan result for user: %w", err)
			}
			invitation.ExpireAt = timestamp.FromTime(expireAt)
			result = append(result, invitation)
		}
	}
	if len(result) != 1 {
		return model.HouseholdSharingLink{}, &model.SharingLinkDoesNotExistError{}
	}
	return result[0], nil
}

func (db *DBClient) SelectHouseholdSharingLink(ctx context.Context, senderID uint64, householdID string) (model.HouseholdSharingLink, error) {
	res, err := db.Read(ctx, db.PragmaPrefix(`
		--!syntax_v1
		DECLARE $sender_huid AS Uint64;
		DECLARE $household_id AS String;
		SELECT
			sender_id, id, household_id, expire_at
		FROM
			SharedHouseholdsLinks
		WHERE
			sender_huid == $sender_huid AND
			household_id == $household_id;
	`),
		table.NewQueryParameters(
			table.ValueParam("$sender_huid", ydb.Uint64Value(tools.Huidify(senderID))),
			table.ValueParam("$household_id", ydb.StringValue([]byte(householdID))),
		),
	)
	if err != nil {
		return model.HouseholdSharingLink{}, xerrors.Errorf("failed to select link of household %s: %w", householdID, err)
	}
	result := make([]model.HouseholdSharingLink, 0)
	for res.NextResultSet(ctx, "id", "sender_id", "household_id", "expire_at") {
		for res.NextRow() {
			var invitation model.HouseholdSharingLink
			var expireAt time.Time
			if err := res.ScanWithDefaults(&invitation.ID, &invitation.SenderID, &invitation.HouseholdID, &expireAt); err != nil {
				return model.HouseholdSharingLink{}, xerrors.Errorf("failed to scan result for user: %w", err)
			}
			invitation.ExpireAt = timestamp.FromTime(expireAt)
			result = append(result, invitation)
		}
	}
	if len(result) != 1 {
		return model.HouseholdSharingLink{}, &model.SharingLinkDoesNotExistError{}
	}
	return result[0], nil
}

func (db *DBClient) DeleteHouseholdSharingLinks(ctx context.Context, senderID uint64, householdID string) error {
	query := db.PragmaPrefix(`
			--!syntax_v1
			DECLARE $sender_huid AS Uint64;
			DECLARE $household_id AS String;
			DELETE FROM
				SharedHouseholdsLinks
			WHERE
				sender_huid == $sender_huid and
				household_id == $household_id`)
	params := table.NewQueryParameters(
		table.ValueParam("$sender_huid", ydb.Uint64Value(tools.Huidify(senderID))),
		table.ValueParam("$household_id", ydb.StringValue([]byte(householdID))),
	)
	return db.Write(ctx, query, params)
}

func (db *DBClient) SelectHouseholdInvitationsByGuest(ctx context.Context, guestID uint64) (model.HouseholdInvitations, error) {
	return db.selectHouseholdInvitations(ctx, HouseholdInvitationCriteria{GuestID: guestID})
}

type HouseholdInvitationCriteria struct {
	GuestID     uint64
	HouseholdID string
}

func (db *DBClient) SelectHouseholdInvitationsBySender(ctx context.Context, senderID uint64) (model.HouseholdInvitations, error) {
	res, err := db.Read(ctx, db.PragmaPrefix(`
		--!syntax_v1
		DECLARE $sender_huid AS UInt64;
		DECLARE $sender_id AS Uint64;
		SELECT
			id, $sender_id AS sender_id, household_id, guest_id
		FROM
			SharedHouseholdsInvitations VIEW shared_households_invitations_sender_huid
		WHERE
			sender_huid == $sender_huid;
	`),
		table.NewQueryParameters(
			table.ValueParam("$sender_huid", ydb.Uint64Value(tools.Huidify(senderID))),
			table.ValueParam("$sender_id", ydb.Uint64Value(senderID)),
		),
	)
	if err != nil {
		return nil, xerrors.Errorf("failed to select invitations by sender %d: %w", senderID, err)
	}
	result := make(model.HouseholdInvitations, 0)
	for res.NextResultSet(ctx, "id", "sender_id", "household_id", "guest_id") {
		for res.NextRow() {
			var invitation model.HouseholdInvitation
			if err := res.ScanWithDefaults(&invitation.ID, &invitation.SenderID, &invitation.HouseholdID, &invitation.GuestID); err != nil {
				return nil, xerrors.Errorf("failed to scan result for user: %w", err)
			}
			result = append(result, invitation)
		}
	}
	return result, nil
}

func (db *DBClient) SelectHouseholdInvitationByID(ctx context.Context, ID string) (model.HouseholdInvitation, error) {
	res, err := db.Read(ctx, db.PragmaPrefix(`
		--!syntax_v1
		DECLARE $id AS String;
		SELECT
			id, sender_id, household_id, guest_id
		FROM
			SharedHouseholdsInvitations VIEW shared_households_invitations_id
		WHERE
			id == $id;
	`),
		table.NewQueryParameters(
			table.ValueParam("$id", ydb.StringValue([]byte(ID))),
		),
	)
	if err != nil {
		return model.HouseholdInvitation{}, xerrors.Errorf("failed to select invitations by id %s: %w", ID, err)
	}
	result := make(model.HouseholdInvitations, 0)
	for res.NextResultSet(ctx, "id", "sender_id", "household_id", "guest_id") {
		for res.NextRow() {
			var invitation model.HouseholdInvitation
			if err := res.ScanWithDefaults(&invitation.ID, &invitation.SenderID, &invitation.HouseholdID, &invitation.GuestID); err != nil {
				return model.HouseholdInvitation{}, xerrors.Errorf("failed to scan result for user: %w", err)
			}
			result = append(result, invitation)
		}
	}
	if len(result) != 1 {
		return model.HouseholdInvitation{}, &model.SharingInvitationDoesNotExistError{}
	}
	return result[0], nil
}

func (db *DBClient) DeleteHouseholdInvitation(ctx context.Context, invitation model.HouseholdInvitation) error {
	query := db.PragmaPrefix(`
			DECLARE $guest_huid AS Uint64;
			DECLARE $sender_huid AS Uint64;
			DECLARE $household_id AS String;
			DELETE FROM
				SharedHouseholdsInvitations
			WHERE
				guest_huid == $guest_huid and
				household_id == $household_id and
				sender_huid == $sender_huid;`)
	params := table.NewQueryParameters(
		table.ValueParam("$guest_huid", ydb.Uint64Value(tools.Huidify(invitation.GuestID))),
		table.ValueParam("$sender_huid", ydb.Uint64Value(tools.Huidify(invitation.SenderID))),
		table.ValueParam("$household_id", ydb.StringValue([]byte(invitation.HouseholdID))),
	)
	return db.Write(ctx, query, params)
}

func (db *DBClient) StoreHouseholdInvitation(ctx context.Context, invitation model.HouseholdInvitation) error {
	upsertFunc := func(ctx context.Context, s *table.Session, txControl *table.TransactionControl) (*table.Transaction, error) {
		tx, exist, err := db.checkUserHouseholdExistTx(ctx, s, txControl, HouseholdQueryCriteria{UserID: invitation.SenderID, HouseholdID: invitation.HouseholdID})
		if err != nil {
			return tx, xerrors.Errorf("failed to get user %d household %s: %w", invitation.SenderID, invitation.HouseholdID, err)
		}
		if !exist {
			return nil, &model.UserHouseholdNotFoundError{}
		}
		query := db.PragmaPrefix(`
			DECLARE $sender_huid AS Uint64;
			DECLARE $sender_id AS Uint64;
			DECLARE $id AS String;
			DECLARE $guest_huid AS Uint64;
			DECLARE $guest_id AS Uint64;
			DECLARE $household_id AS String;
			DECLARE $created AS Timestamp;
			UPSERT INTO
				SharedHouseholdsInvitations (sender_huid, sender_id, id, guest_huid, guest_id, household_id, created)
			VALUES
				($sender_huid, $sender_id, $id, $guest_huid, $guest_id, $household_id, $created);`)
		params := table.NewQueryParameters(
			table.ValueParam("$sender_huid", ydb.Uint64Value(tools.Huidify(invitation.SenderID))),
			table.ValueParam("$sender_id", ydb.Uint64Value(invitation.SenderID)),
			table.ValueParam("$id", ydb.StringValue([]byte(invitation.ID))),
			table.ValueParam("$guest_huid", ydb.Uint64Value(tools.Huidify(invitation.GuestID))),
			table.ValueParam("$guest_id", ydb.Uint64Value(invitation.GuestID)),
			table.ValueParam("$household_id", ydb.StringValue([]byte(invitation.HouseholdID))),
			table.ValueParam("$created", ydb.TimestampValue(db.CurrentTimestamp().YdbTimestamp())),
		)
		upsertStmt, err := s.Prepare(ctx, query)
		if err != nil {
			return nil, xerrors.Errorf("failed to prepare statement: %w", err)
		}
		if _, err := tx.ExecuteStatement(ctx, upsertStmt, params); err != nil {
			return tx, xerrors.Errorf("failed to store household %s invitation from sender %d: %w", invitation.HouseholdID, invitation.SenderID, err)
		}
		return tx, nil
	}
	return db.CallInTx(ctx, SerializableReadWrite, upsertFunc)
}

func (db *DBClient) selectHouseholdInvitations(ctx context.Context, criteria HouseholdInvitationCriteria) (model.HouseholdInvitations, error) {
	var queryB bytes.Buffer
	if err := SelectSharedHouseholdsInvitationsTemplate.Execute(&queryB, criteria); err != nil {
		return nil, err
	}
	res, err := db.Read(ctx, db.PragmaPrefix(queryB.String()),
		table.NewQueryParameters(
			table.ValueParam("$guest_huid", ydb.Uint64Value(tools.Huidify(criteria.GuestID))),
			table.ValueParam("$guest_id", ydb.Uint64Value(criteria.GuestID)),
			table.ValueParam("$household_id", ydb.StringValue([]byte(criteria.HouseholdID))),
		),
	)
	if err != nil {
		return nil, xerrors.Errorf("failed to select shared household invitations: %w", err)
	}
	result := make(model.HouseholdInvitations, 0)
	for res.NextResultSet(ctx, "id", "sender_id", "household_id", "guest_id") {
		for res.NextRow() {
			var invitation model.HouseholdInvitation
			if err := res.ScanWithDefaults(&invitation.ID, &invitation.SenderID, &invitation.HouseholdID, &invitation.GuestID); err != nil {
				return nil, xerrors.Errorf("failed to scan result for user: %w", err)
			}
			result = append(result, invitation)
		}
	}
	return result, nil
}

func (db *DBClient) selectGuestSharingInfosInTx(ctx context.Context, s *table.Session, tx *table.Transaction, guestID uint64) (model.SharingInfos, error) {
	query := db.PragmaPrefix(`
		DECLARE $guest_huid AS UInt64;
		SELECT
			owner_id, household_id, household_name
		FROM
			SharedHouseholds
		WHERE
			guest_huid == $guest_huid
	`)
	params := table.NewQueryParameters(
		table.ValueParam("$guest_huid", ydb.Uint64Value(tools.Huidify(guestID))),
	)
	stmt, err := s.Prepare(ctx, query)
	if err != nil {
		return nil, xerrors.Errorf("failed to prepare statement: %w", err)
	}
	res, err := tx.ExecuteStatement(ctx, stmt, params)
	if err != nil {
		return nil, xerrors.Errorf("failed to execute statement: %w", err)
	}
	result := make(model.SharingInfos, 0)

	for res.NextResultSet(ctx, "owner_id", "household_id", "household_name") {
		for res.NextRow() {
			var sharingInfo model.SharingInfo
			if err := res.ScanWithDefaults(&sharingInfo.OwnerID, &sharingInfo.HouseholdID, &sharingInfo.HouseholdName); err != nil {
				return nil, xerrors.Errorf("failed to scan result for user: %w", err)
			}
			result = append(result, sharingInfo)
		}
	}
	return result, nil
}

func (db *DBClient) selectSharedRoomsInTx(ctx context.Context, s *table.Session, tx *table.Transaction, criteria RoomQueryCriteria) (model.Rooms, error) {
	guestSharingInfos, err := db.selectGuestSharingInfosInTx(ctx, s, tx, criteria.UserID)
	if err != nil {
		return nil, xerrors.Errorf("failed to select guest sharing infos: %w", err)
	}
	if len(guestSharingInfos) == 0 {
		return nil, nil
	}
	result := make(model.Rooms, 0)
	ownerSharingInfoMap := guestSharingInfos.ToOwnerMap()
	for ownerID, sharingInfos := range ownerSharingInfoMap {
		householdIDs := sharingInfos.HouseholdIDs()
		if criteria.HouseholdID != "" && !slices.Contains(householdIDs, criteria.HouseholdID) {
			continue
		}
		ownerCriteria := criteria
		ownerCriteria.UserID = ownerID
		ownerCriteria.WithShared = false
		ownerRooms, err := db.getUserRoomsInTx(ctx, s, tx, ownerCriteria)
		if err != nil {
			return nil, xerrors.Errorf("failed to select shared rooms of user %d: %w", ownerID, err)
		}
		ownerRooms = ownerRooms.FilterByHouseholdIDs(householdIDs)
		ownerRooms.SetSharingInfo(sharingInfos)
		result = append(result, ownerRooms...)
	}
	return result, nil
}

func (db *DBClient) selectSharedGroupsInTx(ctx context.Context, s *table.Session, tx *table.Transaction, criteria GroupQueryCriteria) (model.Groups, error) {
	guestSharingInfos, err := db.selectGuestSharingInfosInTx(ctx, s, tx, criteria.UserID)
	if err != nil {
		return nil, xerrors.Errorf("failed to select guest sharing infos: %w", err)
	}
	if len(guestSharingInfos) == 0 {
		return nil, nil
	}
	result := make(model.Groups, 0)
	ownerSharingInfoMap := guestSharingInfos.ToOwnerMap()
	for ownerID, sharingInfos := range ownerSharingInfoMap {
		householdIDs := sharingInfos.HouseholdIDs()
		if criteria.HouseholdID != "" && !slices.Contains(householdIDs, criteria.HouseholdID) {
			continue
		}
		ownerCriteria := criteria
		ownerCriteria.UserID = ownerID
		ownerCriteria.WithShared = false
		ownerGroups, err := db.getUserGroupsInTx(ctx, s, tx, ownerCriteria)
		if err != nil {
			return nil, xerrors.Errorf("failed to select shared groups of user %d: %w", ownerID, err)
		}
		ownerGroups = ownerGroups.FilterByHouseholdIDs(householdIDs)
		ownerGroups.SetSharingInfo(sharingInfos)
		ownerGroups.SetFavorite(false)
		result = append(result, ownerGroups...)
	}
	return result, nil
}

func (db *DBClient) selectSharedDevicesInTx(ctx context.Context, s *table.Session, tx *table.Transaction, criteria DeviceQueryCriteria) (model.Devices, error) {
	guestSharingInfos, err := db.selectGuestSharingInfosInTx(ctx, s, tx, criteria.UserID)
	if err != nil {
		return nil, xerrors.Errorf("failed to select guest sharing infos: %w", err)
	}
	if len(guestSharingInfos) == 0 {
		return nil, nil
	}
	result := make(model.Devices, 0)
	ownerSharingInfoMap := guestSharingInfos.ToOwnerMap()
	for ownerID, sharingInfos := range ownerSharingInfoMap {
		householdIDs := sharingInfos.HouseholdIDs()
		if criteria.HouseholdID != "" && !slices.Contains(householdIDs, criteria.HouseholdID) {
			continue
		}
		ownerCriteria := criteria
		ownerCriteria.UserID = ownerID
		ownerCriteria.WithShared = false
		ownerDevices, err := db.getUserDevicesInTx(ctx, s, tx, ownerCriteria)
		if err != nil {
			return nil, xerrors.Errorf("failed to select shared devices of user %d: %w", ownerID, err)
		}
		ownerDevices = ownerDevices.FilterByHouseholdIDs(householdIDs)
		ownerDevices.SetSharingInfo(sharingInfos)
		ownerDevices.SetFavorite(false)
		result = append(result, ownerDevices...)
	}
	return result, nil
}

func (db *DBClient) selectSharedDevicesSimpleInTx(ctx context.Context, s *table.Session, tx *table.Transaction, criteria DeviceQueryCriteria) (model.Devices, error) {
	guestSharingInfos, err := db.selectGuestSharingInfosInTx(ctx, s, tx, criteria.UserID)
	if err != nil {
		return nil, xerrors.Errorf("failed to select guest sharing infos: %w", err)
	}
	if len(guestSharingInfos) == 0 {
		return nil, nil
	}
	result := make(model.Devices, 0)
	ownerSharingInfoMap := guestSharingInfos.ToOwnerMap()
	for ownerID, sharingInfos := range ownerSharingInfoMap {
		householdIDs := sharingInfos.HouseholdIDs()
		if criteria.HouseholdID != "" && !slices.Contains(householdIDs, criteria.HouseholdID) {
			continue
		}
		ownerCriteria := criteria
		ownerCriteria.UserID = ownerID
		ownerCriteria.WithShared = false
		ownerDevices, err := db.getUserDevicesSimpleInTx(ctx, s, tx, ownerCriteria)
		if err != nil {
			return nil, xerrors.Errorf("failed to select shared devices of user %d: %w", ownerID, err)
		}
		ownerDevices = ownerDevices.FilterByHouseholdIDs(householdIDs)
		ownerDevices.SetSharingInfo(sharingInfos)
		ownerDevices.SetFavorite(false)
		result = append(result, ownerDevices...)
	}
	return result, nil
}

func (db *DBClient) selectSharedHouseholdsInTx(ctx context.Context, s *table.Session, tx *table.Transaction, criteria HouseholdQueryCriteria) (model.Households, error) {
	guestSharingInfos, err := db.selectGuestSharingInfosInTx(ctx, s, tx, criteria.UserID)
	if err != nil {
		return nil, xerrors.Errorf("failed to select guest sharing infos: %w", err)
	}
	if len(guestSharingInfos) == 0 {
		return nil, nil
	}
	result := make(model.Households, 0)
	ownerSharingInfoMap := guestSharingInfos.ToOwnerMap()
	for ownerID, sharingInfos := range ownerSharingInfoMap {
		householdIDs := sharingInfos.HouseholdIDs()
		if criteria.HouseholdID != "" && !slices.Contains(householdIDs, criteria.HouseholdID) {
			continue
		}
		ownerCriteria := criteria
		ownerCriteria.UserID = ownerID
		ownerCriteria.WithShared = false
		ownerHouseholds, err := db.getUserHouseholdsInTx(ctx, s, tx, ownerCriteria)
		if err != nil {
			return nil, xerrors.Errorf("failed to select shared households of user %d: %w", ownerID, err)
		}
		ownerHouseholds = ownerHouseholds.FilterByIDs(householdIDs)
		ownerHouseholds.SetSharingInfoAndName(sharingInfos)
		result = append(result, ownerHouseholds...)
	}
	return result, nil
}
