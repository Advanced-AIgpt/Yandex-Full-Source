package db

import (
	"bytes"
	"context"
	"encoding/json"
	"fmt"
	"strings"

	"github.com/gofrs/uuid"
	"github.com/google/go-cmp/cmp"
	"golang.org/x/exp/slices"

	"a.yandex-team.ru/alice/iot/bulbasaur/model"
	"a.yandex-team.ru/alice/library/go/tools"
	"a.yandex-team.ru/kikimr/public/sdk/go/ydb"
	"a.yandex-team.ru/kikimr/public/sdk/go/ydb/table"
	"a.yandex-team.ru/library/go/core/log/ctxlog"
	"a.yandex-team.ru/library/go/core/xerrors"
	yslices "a.yandex-team.ru/library/go/slices"
)

type GroupQueryCriteria struct {
	GroupID     string
	UserID      uint64
	HouseholdID string
	WithShared  bool // that flag allows to select groups that were shared with user
}

type DeviceGroupsQueryCriteria struct {
	UserID   uint64
	GroupID  string
	DeviceID string
}

type deviceGroupsItem struct {
	DeviceID string
	GroupID  string
}

type groupChangeSet struct {
	ID       string
	Name     string
	Type     string
	UserID   uint64
	Archived bool
}

func (g *groupChangeSet) toStructValue() ydb.Value {
	var groupType ydb.Value
	if g.Type == "" {
		groupType = ydb.NullValue(ydb.TypeString)
	} else {
		groupType = ydb.OptionalValue(ydb.StringValue([]byte(g.Type)))
	}

	return ydb.StructValue(
		ydb.StructFieldValue("huid", ydb.Uint64Value(tools.Huidify(g.UserID))),
		ydb.StructFieldValue("id", ydb.StringValue([]byte(g.ID))),
		ydb.StructFieldValue("name", ydb.StringValue([]byte(g.Name))),
		ydb.StructFieldValue("type", groupType),
		ydb.StructFieldValue("archived", ydb.BoolValue(g.Archived)),
	)
}

func (db *DBClient) SelectUserGroups(ctx context.Context, userID uint64) ([]model.Group, error) {
	criteria := GroupQueryCriteria{UserID: userID, WithShared: true}
	return db.selectUserGroups(ctx, criteria)
}

func (db *DBClient) SelectUserGroup(ctx context.Context, userID uint64, groupID string) (model.Group, error) {
	criteria := GroupQueryCriteria{UserID: userID, GroupID: groupID, WithShared: true}
	result, err := db.selectUserGroups(ctx, criteria)
	if err != nil {
		return model.Group{}, xerrors.Errorf("cannot get user group from database: %w", err)
	}
	if len(result) > 1 {
		return model.Group{}, fmt.Errorf("found more than one group using criteria: %#v", criteria)
	}

	if len(result) == 0 {
		return model.Group{}, &model.GroupNotFoundError{}
	}
	return result[0], nil
}

func (db *DBClient) SelectUserHouseholdGroups(ctx context.Context, userID uint64, householdID string) (model.Groups, error) {
	criteria := GroupQueryCriteria{UserID: userID, HouseholdID: householdID, WithShared: true}
	return db.selectUserGroups(ctx, criteria)
}

func (db *DBClient) CreateUserGroup(ctx context.Context, user model.User, group model.Group) (groupID string, err error) {
	createFunc := func(ctx context.Context, s *table.Session, txControl *table.TransactionControl) (*table.Transaction, error) {
		tx, currentHousehold, err := db.selectCurrentHouseholdTx(ctx, s, txControl, CurrentHouseholdQueryCriteria{UserID: user.ID})
		if err != nil {
			return tx, err
		}
		var groupHouseholdID string
		if group.HouseholdID != "" {
			groupHouseholdID = group.HouseholdID
		} else {
			groupHouseholdID = currentHousehold.ID
		}
		criteria := GroupQueryCriteria{UserID: user.ID, HouseholdID: groupHouseholdID}
		userGroups, err := db.getUserGroupsInTx(ctx, s, tx, criteria)
		if err != nil {
			return tx, err
		}
		householdExists, err := db.checkUserHouseholdExistInTx(ctx, s, tx, HouseholdQueryCriteria{UserID: user.ID, HouseholdID: groupHouseholdID})
		if err != nil {
			return tx, err
		}
		if !householdExists {
			return tx, &model.UserHouseholdNotFoundError{}
		}

		normalizedName := tools.StandardizeSpaces(group.Name)
		if normalizedName == "" {
			return tx, &model.InvalidValueError{}
		}

		for _, userGroup := range userGroups {
			otherAliases := make([]string, 0, len(userGroup.Aliases))
			for _, alias := range userGroup.Aliases {
				otherAliases = append(otherAliases, strings.ToLower(alias))
			}
			if tools.IsAlphanumericEqual(userGroup.Name, normalizedName) || slices.Contains(otherAliases, strings.ToLower(normalizedName)) {
				return tx, &model.NameIsAlreadyTakenError{}
			}
		}

		uid, err := uuid.NewV4()
		if err != nil {
			ctxlog.Warnf(ctx, db.Logger, "failed to generate UUID: %v", err)
		}
		groupID = uid.String()

		_, userDevices, err := db.getUserDevicesSimpleTx(ctx, s, table.TxControl(table.WithTx(tx)), DeviceQueryCriteria{UserID: user.ID, HouseholdID: groupHouseholdID})
		if err != nil {
			return tx, err
		}
		devicesMap := userDevices.ToMap()
		devicesToGroup := make(model.Devices, 0)
		for _, deviceID := range group.Devices {
			if d, exist := devicesMap[deviceID]; exist {
				devicesToGroup = append(devicesToGroup, d)
			}
		}
		if !group.CompatibleWithDevices(devicesToGroup) {
			return tx, &model.IncompatibleDeviceTypeError{}
		}
		var deviceGroupsInsertData []deviceGroupRaw
		for _, d := range devicesToGroup {
			deviceGroupsInsertData = append(deviceGroupsInsertData, deviceGroupRaw{
				huid:     tools.Huidify(user.ID),
				userID:   user.ID,
				deviceID: d.ID,
				groupID:  groupID,
			})
		}
		if err := db.updateDeviceGroupsInTx(ctx, s, tx, deviceGroupsInsertData); err != nil {
			return tx, err
		}

		aggregatedType := devicesToGroup.AggregatedDeviceType()
		var groupTypeValue ydb.Value
		if aggregatedType != "" {
			groupTypeValue = ydb.OptionalValue(ydb.StringValue([]byte(aggregatedType)))
		} else {
			groupTypeValue = ydb.NullValue(ydb.TypeString)
		}

		var groupHouseholdIDValue ydb.Value
		if groupHouseholdID != "" {
			groupHouseholdIDValue = ydb.OptionalValue(ydb.StringValue([]byte(groupHouseholdID)))
		} else {
			groupHouseholdIDValue = ydb.NullValue(ydb.TypeString)
		}

		insertQuery := fmt.Sprintf(`
			--!syntax_v1
			PRAGMA TablePathPrefix("%s");
			DECLARE $uid AS Uint64;
			DECLARE $huid AS Uint64;
			DECLARE $group_id AS String;
			DECLARE $group_name AS String;
			DECLARE $timestamp AS Timestamp;
			DECLARE $household_id AS Optional<String>;
			DECLARE $group_type AS Optional<String>;

			UPSERT INTO
				Groups (id, huid, name, household_id, type, user_id, archived, created)
			VALUES
				($group_id, $huid, $group_name, $household_id, $group_type, $uid, false, $timestamp);`, db.Prefix)
		params := table.NewQueryParameters(
			table.ValueParam("$uid", ydb.Uint64Value(user.ID)),
			table.ValueParam("$huid", ydb.Uint64Value(tools.Huidify(user.ID))),
			table.ValueParam("$group_name", ydb.StringValue([]byte(normalizedName))),
			table.ValueParam("$group_id", ydb.StringValue([]byte(groupID))),
			table.ValueParam("$household_id", groupHouseholdIDValue),
			table.ValueParam("$group_type", groupTypeValue),
			table.ValueParam("$timestamp", ydb.TimestampValue(db.CurrentTimestamp().YdbTimestamp())),
		)
		insertQueryStmt, err := s.Prepare(ctx, insertQuery)
		if err != nil {
			return tx, err
		}
		_, err = tx.ExecuteStatement(ctx, insertQueryStmt, params)
		return tx, err
	}

	err = db.CallInTx(ctx, SerializableReadWrite, createFunc)
	if err != nil {
		return "", xerrors.Errorf("database request has failed: %w", err)
	}

	return groupID, nil
}

func (db *DBClient) DeleteUserGroup(ctx context.Context, userID uint64, groupID string) error {
	createFunc := func(ctx context.Context, s *table.Session, txControl *table.TransactionControl) (*table.Transaction, error) {
		deleteQuery := fmt.Sprintf(`
			--!syntax_v1
			PRAGMA TablePathPrefix("%s");
			DECLARE $group_id AS String;
			DECLARE $huid AS Uint64;

			UPDATE
				Groups
			SET
				archived = true
			WHERE
				huid == $huid AND
				id == $group_id;`, db.Prefix)
		params := table.NewQueryParameters(
			table.ValueParam("$group_id", ydb.StringValue([]byte(groupID))),
			table.ValueParam("$huid", ydb.Uint64Value(tools.Huidify(userID))),
		)

		stmt, err := s.Prepare(ctx, deleteQuery)
		if err != nil {
			return nil, err
		}

		tx, _, err := stmt.Execute(ctx, txControl, params)
		if err != nil {
			return tx, err
		}

		deleteDeviceGroupsQuery := fmt.Sprintf(`
			--!syntax_v1
			PRAGMA TablePathPrefix("%s");
			DECLARE $group_id AS String;
			DECLARE $huid AS Uint64;

			DELETE FROM
				DeviceGroups
			WHERE
				huid == $huid AND
				group_id == $group_id;
		`, db.Prefix)
		deleteDeviceGroupsQueryStmt, err := s.Prepare(ctx, deleteDeviceGroupsQuery)
		if err != nil {
			return tx, err
		}
		_, err = tx.ExecuteStatement(ctx, deleteDeviceGroupsQueryStmt, params)
		return tx, err
	}

	err := db.CallInTx(ctx, SerializableReadWrite, createFunc)
	if err != nil {
		return xerrors.Errorf("database request has failed: %w", err)
	}

	return nil
}

type deviceGroupRaw struct {
	huid     uint64
	userID   uint64
	deviceID string
	groupID  string
}

func (dgr deviceGroupRaw) ToYDBValue() ydb.Value {
	return ydb.StructValue(
		ydb.StructFieldValue("huid", ydb.Uint64Value(dgr.huid)),
		ydb.StructFieldValue("user_id", ydb.Uint64Value(dgr.userID)),
		ydb.StructFieldValue("device_id", ydb.StringValue([]byte(dgr.deviceID))),
		ydb.StructFieldValue("group_id", ydb.StringValue([]byte(dgr.groupID))),
	)
}

func (db *DBClient) UpdateUserDeviceGroups(ctx context.Context, user model.User, deviceID string, groupsIDs []string) error {
	updateFunc := func(ctx context.Context, s *table.Session, txControl *table.TransactionControl) (*table.Transaction, error) {
		// Get user device and start update transaction
		deviceCriteria := DeviceQueryCriteria{UserID: user.ID, DeviceID: deviceID}
		tx, userDevices, err := db.getUserDevicesTx(ctx, s, txControl, deviceCriteria)
		if err != nil {
			return tx, err
		}
		if len(userDevices) == 0 {
			return tx, &model.DeviceNotFoundError{}
		}
		if len(userDevices) > 1 {
			return tx, fmt.Errorf("found more than one device using deviceId: %s", deviceID)
		}
		device := userDevices[0]

		groupCriteria := GroupQueryCriteria{UserID: user.ID, HouseholdID: device.HouseholdID}
		userGroups, err := db.getUserGroupsInTx(ctx, s, tx, groupCriteria)
		if err != nil {
			return tx, err
		}

		// If user has no groups but wants to put device into some group
		if len(userGroups) == 0 && len(groupsIDs) != 0 {
			return tx, &model.GroupNotFoundError{}
		}

		userGroupsMap := make(map[string]model.Group)
		for _, userGroup := range userGroups {
			userGroupsMap[userGroup.ID] = userGroup
		}

		// Check that user has rights to use groups from request and prepare data for insert
		deviceGroupsInsertData := make([]deviceGroupRaw, 0, len(groupsIDs))
		groupsTypesChangeSet := make([]groupChangeSet, 0, len(groupsIDs))
		for _, groupID := range groupsIDs {
			// Prevent to put device into foreign group
			group, exists := userGroupsMap[groupID]
			if !exists {
				return tx, &model.GroupNotFoundError{}
			}

			if !group.CompatibleWithDeviceType(device.Type) {
				return tx, &model.IncompatibleDeviceTypeError{}
			}

			// Create data to insert into DevicesGroups
			deviceGroupsInsertRaw := deviceGroupRaw{
				huid:     tools.Huidify(user.ID),
				userID:   user.ID,
				deviceID: deviceID,
				groupID:  groupID,
			}
			deviceGroupsInsertData = append(deviceGroupsInsertData, deviceGroupsInsertRaw)

			// Create data to update group type
			// If group has no devices before change its type to target group type
			if len(group.Devices) == 0 {
				targetGroupType := device.Type
				if device.Type.IsSmartSpeaker() {
					if model.MultiroomSpeakers[device.Type] {
						targetGroupType = model.SmartSpeakerDeviceType
					} else {
						// non-multiroom speakers can't be placed in groups
						return tx, &model.IncompatibleDeviceTypeError{}
					}
				}
				groupsTypesChangeSet = append(groupsTypesChangeSet, groupChangeSet{ID: group.ID, Name: group.Name, Type: targetGroupType.String(), UserID: user.ID, Archived: false})
			}
		}

		// Starting update
		// Delete device from all groups
		if err := db.removeDeviceFromAllGroupsInTx(ctx, s, tx, user.ID, deviceID); err != nil {
			return tx, err
		}

		// If some groups is empty now - set type to null via groupsTypesChangeSet
		for _, group := range userGroups {
			if (cmp.Equal(group.Devices, []string{deviceID}) || len(group.Devices) == 0) && !tools.Contains(group.ID, groupsIDs) {
				groupsTypesChangeSet = append(groupsTypesChangeSet, groupChangeSet{ID: group.ID, Name: group.Name, Type: "", UserID: user.ID, Archived: false})
			}
		}

		// Add new mapping device:groups if new deviceGroups are added
		if err := db.updateDeviceGroupsInTx(ctx, s, tx, deviceGroupsInsertData); err != nil {
			return tx, err
		}

		// Update groups types
		if err := db.updateGroupsInTx(ctx, s, tx, groupsTypesChangeSet); err != nil {
			return tx, xerrors.Errorf("failed to update groups types: %w", err)
		}

		return tx, nil
	}

	err := db.CallInTx(ctx, SerializableReadWrite, updateFunc)
	if err != nil {
		return xerrors.Errorf("database request has failed: %w", err)
	}

	return nil
}

func (db *DBClient) UpdateUserGroupNameAndDevices(ctx context.Context, user model.User, group model.Group) error {
	updateFunc := func(ctx context.Context, s *table.Session, txControl *table.TransactionControl) (*table.Transaction, error) {
		// Get user groups and start update transaction
		tx, currentHousehold, err := db.selectCurrentHouseholdTx(ctx, s, txControl, CurrentHouseholdQueryCriteria{UserID: user.ID})
		if err != nil {
			return tx, err
		}
		var groupHouseholdID string
		if group.HouseholdID != "" {
			groupHouseholdID = group.HouseholdID
		} else {
			groupHouseholdID = currentHousehold.ID
		}
		criteria := GroupQueryCriteria{UserID: user.ID, HouseholdID: groupHouseholdID}
		userGroups, err := db.getUserGroupsInTx(ctx, s, tx, criteria)
		if err != nil {
			return tx, err
		}

		normalizedName := tools.StandardizeSpaces(group.Name)
		if normalizedName == "" {
			return tx, &model.InvalidValueError{}
		}

		var currentGroup *model.Group
		for i, userGroup := range userGroups {
			if userGroup.ID == group.ID {
				currentGroup = &userGroups[i]
				continue
			}
			otherAliases := make([]string, 0, len(userGroup.Aliases))
			for _, alias := range userGroup.Aliases {
				otherAliases = append(otherAliases, strings.ToLower(alias))
			}
			if tools.IsAlphanumericEqual(userGroup.Name, normalizedName) || slices.Contains(otherAliases, strings.ToLower(normalizedName)) {
				return tx, &model.NameIsAlreadyTakenError{}
			}
		}

		if currentGroup == nil {
			return tx, &model.GroupNotFoundError{}
		}
		_, devices, err := db.getUserDevicesSimpleTx(ctx, s, table.TxControl(table.WithTx(tx)), DeviceQueryCriteria{UserID: user.ID, HouseholdID: groupHouseholdID})
		if err != nil {
			return tx, err
		}
		devicesMap := devices.ToMap()
		devicesToGroup := make(model.Devices, 0)
		for _, deviceID := range group.Devices {
			if d, exist := devicesMap[deviceID]; exist {
				devicesToGroup = append(devicesToGroup, d)
			}
		}
		aggregatedType := devicesToGroup.AggregatedDeviceType()
		if len(devicesToGroup) != 0 && aggregatedType == "" {
			return tx, &model.IncompatibleDeviceTypeError{}
		}

		deviceGroupsInsertData := make([]deviceGroupRaw, 0, len(devicesToGroup))
		groupTypeChangeSet := groupChangeSet{
			ID:     group.ID,
			Name:   group.Name,
			UserID: user.ID,
			Type:   string(aggregatedType),
		}
		for _, device := range devicesToGroup {
			// Create data to insert into DevicesGroups
			deviceGroupsInsertRaw := deviceGroupRaw{
				huid:     tools.Huidify(user.ID),
				userID:   user.ID,
				deviceID: device.ID,
				groupID:  currentGroup.ID,
			}
			deviceGroupsInsertData = append(deviceGroupsInsertData, deviceGroupsInsertRaw)
		}

		// Starting update
		// remove all devices from that group
		if err := db.removeAllDevicesFromGroupInTx(ctx, s, tx, user.ID, group.ID); err != nil {
			return tx, xerrors.Errorf("failed to remove all devices from group: %w", err)
		}

		// Add new mapping device:groups
		if err := db.updateDeviceGroupsInTx(ctx, s, tx, deviceGroupsInsertData); err != nil {
			return tx, err
		}

		// Update groups types
		if err := db.updateGroupsInTx(ctx, s, tx, []groupChangeSet{groupTypeChangeSet}); err != nil {
			return tx, xerrors.Errorf("failed to update group %s type: %w", currentGroup.ID, err)
		}

		return tx, nil
	}

	err := db.CallInTx(ctx, SerializableReadWrite, updateFunc)
	if err != nil {
		return xerrors.Errorf("database request has failed: %w", err)
	}

	return nil
}

func (db *DBClient) UpdateUserGroupName(ctx context.Context, user model.User, groupID string, name string) error {
	updateFunc := func(ctx context.Context, s *table.Session, txControl *table.TransactionControl) (*table.Transaction, error) {
		tx, currentHousehold, err := db.selectCurrentHouseholdTx(ctx, s, txControl, CurrentHouseholdQueryCriteria{UserID: user.ID})
		if err != nil {
			return tx, err
		}
		criteria := GroupQueryCriteria{UserID: user.ID, HouseholdID: currentHousehold.ID}
		userGroups, err := db.getUserGroupsInTx(ctx, s, tx, criteria)
		if err != nil {
			return tx, err
		}

		normalizedName := tools.StandardizeSpaces(name)
		if normalizedName == "" {
			return tx, &model.InvalidValueError{}
		}

		groupsIDs := make(map[string]bool)
		for _, userGroup := range userGroups {
			otherAliases := make([]string, 0, len(userGroup.Aliases))
			for _, alias := range userGroup.Aliases {
				otherAliases = append(otherAliases, strings.ToLower(alias))
			}
			if tools.IsAlphanumericEqual(userGroup.Name, normalizedName) || slices.Contains(otherAliases, strings.ToLower(normalizedName)) {
				if userGroup.ID == groupID {
					return tx, nil
				} else {
					return tx, &model.NameIsAlreadyTakenError{}
				}
			}

			groupsIDs[userGroup.ID] = true
		}
		if _, ok := groupsIDs[groupID]; !ok {
			return tx, &model.GroupNotFoundError{}
		}

		updateQuery := fmt.Sprintf(`
			--!syntax_v1
			PRAGMA TablePathPrefix("%s");
			DECLARE $huid AS Uint64;
			DECLARE $group_id AS String;
			DECLARE $group_name AS String;

			UPDATE
				Groups
			SET
				name = $group_name
			WHERE
				huid == $huid AND
				id == $group_id AND
				archived == false;`, db.Prefix)
		params := table.NewQueryParameters(
			table.ValueParam("$huid", ydb.Uint64Value(tools.Huidify(user.ID))),
			table.ValueParam("$group_name", ydb.StringValue([]byte(normalizedName))),
			table.ValueParam("$group_id", ydb.StringValue([]byte(groupID))),
		)

		updateQueryStmt, err := s.Prepare(ctx, updateQuery)
		if err != nil {
			return tx, err
		}
		_, err = tx.ExecuteStatement(ctx, updateQueryStmt, params)
		if err != nil {
			return tx, err
		}

		return tx, nil
	}

	err := db.CallInTx(ctx, SerializableReadWrite, updateFunc)
	if err != nil {
		return xerrors.Errorf("database request has failed: %w", err)
	}

	return nil
}

func (db *DBClient) UpdateUserGroupNameAndAliases(ctx context.Context, user model.User, groupID string, name string, aliases []string) error {
	updateFunc := func(ctx context.Context, s *table.Session, txControl *table.TransactionControl) (*table.Transaction, error) {
		tx, currentHousehold, err := db.selectCurrentHouseholdTx(ctx, s, txControl, CurrentHouseholdQueryCriteria{UserID: user.ID})
		if err != nil {
			return tx, err
		}
		criteria := GroupQueryCriteria{UserID: user.ID, HouseholdID: currentHousehold.ID}
		userGroups, err := db.getUserGroupsInTx(ctx, s, tx, criteria)
		if err != nil {
			return tx, err
		}

		normalizedName := tools.StandardizeSpaces(name)
		if normalizedName == "" {
			return tx, &model.InvalidValueError{}
		}

		if len(aliases) > model.GroupNameAliasesLimit {
			return tx, &model.GroupAliasesLimitReachedError{}
		}

		normalizedAliases := make([]string, 0, len(aliases))
		loweredNormalizedAliases := make([]string, 0, len(aliases))
		for _, a := range aliases {
			alias := tools.StandardizeSpaces(a)
			if alias == "" {
				return tx, &model.InvalidValueError{}
			}
			normalizedAliases = append(normalizedAliases, alias)
			loweredNormalizedAliases = append(loweredNormalizedAliases, strings.ToLower(alias))
		}

		groupsIDs := make(map[string]bool)
		for _, userGroup := range userGroups {
			otherAliases := make([]string, 0, len(userGroup.Aliases))
			for _, alias := range userGroup.Aliases {
				otherAliases = append(otherAliases, strings.ToLower(alias))
			}
			if tools.IsAlphanumericEqual(userGroup.Name, normalizedName) ||
				yslices.ContainsAny(otherAliases, loweredNormalizedAliases) ||
				slices.Contains(otherAliases, strings.ToLower(normalizedName)) {
				if userGroup.ID != groupID {
					return tx, &model.NameIsAlreadyTakenError{}
				}
			}

			groupsIDs[userGroup.ID] = true
		}
		if _, ok := groupsIDs[groupID]; !ok {
			return tx, &model.GroupNotFoundError{}
		}

		marshalledAliases, err := json.Marshal(normalizedAliases)
		if err != nil {
			return tx, err
		}

		updateQuery := fmt.Sprintf(`
			--!syntax_v1
			PRAGMA TablePathPrefix("%s");
			DECLARE $huid AS Uint64;
			DECLARE $group_id AS String;
			DECLARE $group_name AS String;
			DECLARE $group_aliases AS Json;

			UPDATE
				Groups
			SET
				name = $group_name,
				aliases = $group_aliases
			WHERE
				huid == $huid AND
				id == $group_id AND
				archived == false;`, db.Prefix)
		params := table.NewQueryParameters(
			table.ValueParam("$huid", ydb.Uint64Value(tools.Huidify(user.ID))),
			table.ValueParam("$group_id", ydb.StringValue([]byte(groupID))),
			table.ValueParam("$group_name", ydb.StringValue([]byte(normalizedName))),
			table.ValueParam("$group_aliases", ydb.JSONValue(string(marshalledAliases))),
		)

		updateQueryStmt, err := s.Prepare(ctx, updateQuery)
		if err != nil {
			return tx, err
		}
		_, err = tx.ExecuteStatement(ctx, updateQueryStmt, params)
		if err != nil {
			return tx, err
		}

		return tx, nil
	}

	err := db.CallInTx(ctx, SerializableReadWrite, updateFunc)
	if err != nil {
		return xerrors.Errorf("database request has failed: %w", err)
	}

	return nil
}

func (db *DBClient) removeDevicesFromAllGroupsInTx(ctx context.Context, s *table.Session, tx *table.Transaction, userID uint64, deviceIDs []string) error {
	if len(deviceIDs) == 0 {
		return nil
	}
	deleteQuery := fmt.Sprintf(`
			--!syntax_v1
			PRAGMA TablePathPrefix("%s");
			DECLARE $huid AS Uint64;
			DECLARE $device_ids AS List<String>;

			DELETE FROM
				DeviceGroups
			WHERE
				huid == $huid AND
				device_id in $device_ids;`, db.Prefix)
	deviceIDsValues := make([]ydb.Value, 0, len(deviceIDs))
	for _, deviceID := range deviceIDs {
		deviceIDsValues = append(deviceIDsValues, ydb.StringValue([]byte(deviceID)))
	}

	params := table.NewQueryParameters(
		table.ValueParam("$huid", ydb.Uint64Value(tools.Huidify(userID))),
		table.ValueParam("$device_ids", ydb.ListValue(deviceIDsValues...)),
	)
	deleteQueryStmt, err := s.Prepare(ctx, deleteQuery)
	if err != nil {
		return err
	}
	_, err = tx.ExecuteStatement(ctx, deleteQueryStmt, params)
	if err != nil {
		return err
	}

	return nil
}

func (db *DBClient) removeDeviceFromAllGroupsInTx(ctx context.Context, s *table.Session, tx *table.Transaction, userID uint64, deviceID string) error {
	return db.removeDevicesFromAllGroupsInTx(ctx, s, tx, userID, []string{deviceID})
}

func (db *DBClient) removeAllDevicesFromGroupInTx(ctx context.Context, s *table.Session, tx *table.Transaction, userID uint64, groupID string) error {
	deleteQuery := fmt.Sprintf(`
			--!syntax_v1
			PRAGMA TablePathPrefix("%s");
			DECLARE $huid AS Uint64;
			DECLARE $group_id AS String;

			DELETE FROM
				DeviceGroups
			WHERE
				huid == $huid AND
				group_id == $group_id;`, db.Prefix)
	params := table.NewQueryParameters(
		table.ValueParam("$huid", ydb.Uint64Value(tools.Huidify(userID))),
		table.ValueParam("$group_id", ydb.StringValue([]byte(groupID))),
	)
	deleteQueryStmt, err := s.Prepare(ctx, deleteQuery)
	if err != nil {
		return err
	}
	_, err = tx.ExecuteStatement(ctx, deleteQueryStmt, params)
	if err != nil {
		return err
	}

	return nil
}

type deviceGroupsDeleteChange struct {
	UserID   uint64
	DeviceID string
	GroupID  string
}

func (d *deviceGroupsDeleteChange) toStructValue(userID uint64) ydb.Value {
	return ydb.StructValue(
		ydb.StructFieldValue("huid", ydb.Uint64Value(tools.Huidify(userID))),
		ydb.StructFieldValue("device_id", ydb.StringValue([]byte(d.DeviceID))),
		ydb.StructFieldValue("group_id", ydb.StringValue([]byte(d.GroupID))),
	)
}

func (db *DBClient) removeDevicesFromGroupsInTx(ctx context.Context, s *table.Session, tx *table.Transaction, userID uint64, deleteSet []deviceGroupsDeleteChange) error {
	if len(deleteSet) == 0 {
		return nil
	}

	deleteDeviceGroupsQuery := fmt.Sprintf(`
			--!syntax_v1
			PRAGMA TablePathPrefix("%s");
			DECLARE $device_groups AS List<Struct<
				huid: Uint64,
				device_id: String,
				group_id: String
			>>;

			DELETE FROM DeviceGroups ON SELECT huid, device_id, group_id FROM AS_TABLE($device_groups)
		`, db.Prefix)

	deleteSetParams := make([]ydb.Value, 0, len(deleteSet))
	for _, entry := range deleteSet {
		deleteSetParams = append(deleteSetParams, entry.toStructValue(userID))
	}

	deleteParams := table.NewQueryParameters(
		table.ValueParam("$device_groups", ydb.ListValue(deleteSetParams...)),
	)
	deleteDeviceGroupsQueryStmt, err := s.Prepare(ctx, deleteDeviceGroupsQuery)
	if err != nil {
		return err
	}
	_, err = tx.ExecuteStatement(ctx, deleteDeviceGroupsQueryStmt, deleteParams)
	if err != nil {
		return err
	}

	return nil
}

func (db *DBClient) updateGroupsInTx(ctx context.Context, s *table.Session, tx *table.Transaction, changeSet []groupChangeSet) error {
	if len(changeSet) == 0 {
		return nil
	}

	updateQuery := fmt.Sprintf(`
			--!syntax_v1
			PRAGMA TablePathPrefix("%s");
			DECLARE $changeset AS List<Struct<
				huid: Uint64,
				id: String,
				name: String,
				type: String?,
				archived: Bool
			>>;

			UPDATE Groups ON
			SELECT * FROM AS_TABLE($changeset);`, db.Prefix)

	queryChangeSetParams := make([]ydb.Value, 0, len(changeSet))
	for _, change := range changeSet {
		queryChangeSetParams = append(queryChangeSetParams, change.toStructValue())
	}

	params := table.NewQueryParameters(
		table.ValueParam("$changeset", ydb.ListValue(queryChangeSetParams...)),
	)
	updateQueryStmt, err := s.Prepare(ctx, updateQuery)
	if err != nil {
		return err
	}
	_, err = tx.ExecuteStatement(ctx, updateQueryStmt, params)
	if err != nil {
		return err
	}

	return nil
}

func (db *DBClient) selectUserGroupsSimple(ctx context.Context, criteria GroupQueryCriteria) ([]model.Group, error) {
	var queryB bytes.Buffer
	if err := SelectUserGroupsSimpleTemplate.Execute(&queryB, criteria); err != nil {
		return nil, err
	}

	query := fmt.Sprintf(queryB.String(), db.Prefix)
	params := table.NewQueryParameters(
		table.ValueParam("$huid", ydb.Uint64Value(tools.Huidify(criteria.UserID))),
		table.ValueParam("$group_id", ydb.StringValue([]byte(criteria.GroupID))),
		table.ValueParam("$household_id", ydb.StringValue([]byte(criteria.HouseholdID))),
	)

	res, err := db.Read(ctx, query, params)
	if err != nil {
		return nil, xerrors.Errorf("failed to get groups from DB: %w", err)
	}

	if !res.NextSet() {
		return nil, xerrors.New("result set not found")
	}

	groups := make([]model.Group, 0, res.SetRowCount())
	for res.NextRow() {
		group := model.Group{Devices: []string{}}

		res.NextItem()
		group.ID = string(res.OString())

		res.NextItem()
		group.Name = string(res.OString())

		res.NextItem()
		rawJSONAliases := res.OJSON()
		group.Aliases = make([]string, 0)
		if len(rawJSONAliases) != 0 {
			var aliases []string
			if err := json.Unmarshal([]byte(rawJSONAliases), &aliases); err != nil {
				return nil, err
			}
			group.Aliases = aliases
		}

		res.NextItem()
		group.Type = model.DeviceType(res.OString())

		res.NextItem()
		group.HouseholdID = string(res.OString())

		if res.Err() != nil {
			err := xerrors.Errorf("failed to parse YDB response row: %w", res.Err())
			ctxlog.Error(ctx, db.Logger, err.Error())
			return nil, err
		}

		groups = append(groups, group)
	}

	return groups, nil
}

func (db *DBClient) selectUserGroups(ctx context.Context, criteria GroupQueryCriteria) ([]model.Group, error) {
	var groups []model.Group
	selectFunc := func(ctx context.Context, s *table.Session, txControl *table.TransactionControl) (tx *table.Transaction, err error) {
		tx, groups, err = db.getUserGroupsTx(ctx, s, txControl, criteria)
		return tx, err
	}

	err := db.CallInTx(ctx, SerializableReadWrite, selectFunc)
	if err != nil {
		return groups, xerrors.Errorf("database request has failed: %w", err)
	}

	return groups, nil
}

func (db *DBClient) getUserGroupsTx(ctx context.Context, s *table.Session, tc *table.TransactionControl, criteria GroupQueryCriteria) (tx *table.Transaction, groups model.Groups, err error) {
	var queryB bytes.Buffer
	if err := SelectUserGroupsTemplate.Execute(&queryB, criteria); err != nil {
		return nil, groups, err
	}

	query := fmt.Sprintf(queryB.String(), db.Prefix)
	params := table.NewQueryParameters(
		table.ValueParam("$huid", ydb.Uint64Value(tools.Huidify(criteria.UserID))),
		table.ValueParam("$group_id", ydb.StringValue([]byte(criteria.GroupID))),
		table.ValueParam("$household_id", ydb.StringValue([]byte(criteria.HouseholdID))),
	)

	stmt, err := s.Prepare(ctx, query)
	if err != nil {
		return nil, nil, err
	}

	tx, res, err := stmt.Execute(ctx, tc, params)
	defer func() {
		if err != nil {
			db.RollbackTransaction(ctx, tx, err)
		}
	}()
	if err != nil {
		return nil, nil, err
	}

	groups = make([]model.Group, 0, res.RowCount())
	for res.NextSet() {
		for res.NextRow() {
			var group model.Group
			res.SeekItem("id")
			group.ID = string(res.OString())
			res.NextItem()
			group.Name = string(res.OString())

			res.NextItem()
			rawJSONAliases := res.OJSON()
			group.Aliases = make([]string, 0)
			if len(rawJSONAliases) != 0 {
				var aliases []string
				if err := json.Unmarshal([]byte(rawJSONAliases), &aliases); err != nil {
					return tx, nil, err
				}
				group.Aliases = aliases
			}

			res.NextItem()
			group.Type = model.DeviceType(res.OString())

			// devices
			res.NextItem()
			devices := make([]string, 0)
			for i, n := 0, res.ListIn(); i < n; i++ {
				res.ListItem(i)
				devices = append(devices, string(res.String()))
			}
			res.ListOut()
			group.Devices = devices

			// household_id
			res.NextItem()
			group.HouseholdID = string(res.OString())

			// favourite
			res.NextItem()
			inFavoritesID := string(res.OString())
			group.Favorite = len(inFavoritesID) > 0

			if res.Err() != nil {
				err := xerrors.Errorf("failed to parse YDB response row: %w", res.Err())
				ctxlog.Error(ctx, db.Logger, err.Error())
				return nil, nil, err
			}

			groups = append(groups, group)
		}
	}

	if criteria.WithShared {
		sharedGroups, err := db.selectSharedGroupsInTx(ctx, s, tx, criteria)
		if err != nil {
			return tx, nil, xerrors.Errorf("failed to select shared groups in tx: %w", err)
		}
		groups = append(groups, sharedGroups...)
	}

	return tx, groups, nil
}

func (db *DBClient) getUserGroupsInTx(ctx context.Context, s *table.Session, tx *table.Transaction, criteria GroupQueryCriteria) (model.Groups, error) {
	var groups model.Groups
	var queryB bytes.Buffer
	if err := SelectUserGroupsTemplate.Execute(&queryB, criteria); err != nil {
		return groups, err
	}

	query := fmt.Sprintf(queryB.String(), db.Prefix)
	params := table.NewQueryParameters(
		table.ValueParam("$huid", ydb.Uint64Value(tools.Huidify(criteria.UserID))),
		table.ValueParam("$group_id", ydb.StringValue([]byte(criteria.GroupID))),
		table.ValueParam("$household_id", ydb.StringValue([]byte(criteria.HouseholdID))),
	)

	stmt, err := s.Prepare(ctx, query)
	if err != nil {
		return nil, err
	}

	res, err := tx.ExecuteStatement(ctx, stmt, params)
	if err != nil {
		return nil, err
	}

	groups = make([]model.Group, 0, res.RowCount())
	for res.NextSet() {
		for res.NextRow() {
			var group model.Group
			res.SeekItem("id")
			group.ID = string(res.OString())
			res.NextItem()
			group.Name = string(res.OString())

			res.NextItem()
			rawJSONAliases := res.OJSON()
			group.Aliases = make([]string, 0)
			if len(rawJSONAliases) != 0 {
				var aliases []string
				if err := json.Unmarshal([]byte(rawJSONAliases), &aliases); err != nil {
					return nil, err
				}
				group.Aliases = aliases
			}

			res.NextItem()
			group.Type = model.DeviceType(res.OString())

			// devices
			res.NextItem()
			devices := make([]string, 0)
			for i, n := 0, res.ListIn(); i < n; i++ {
				res.ListItem(i)
				devices = append(devices, string(res.String()))
			}
			res.ListOut()
			group.Devices = devices

			// household_id
			res.NextItem()
			group.HouseholdID = string(res.OString())

			// favourite
			res.NextItem()
			inFavoritesID := string(res.OString())
			group.Favorite = len(inFavoritesID) > 0

			if res.Err() != nil {
				err := xerrors.Errorf("failed to parse YDB response row: %w", res.Err())
				ctxlog.Error(ctx, db.Logger, err.Error())
				return nil, err
			}

			groups = append(groups, group)
		}
	}

	if criteria.WithShared {
		sharedGroups, err := db.selectSharedGroupsInTx(ctx, s, tx, criteria)
		if err != nil {
			return nil, xerrors.Errorf("failed to select shared groups in tx: %w", err)
		}
		groups = append(groups, sharedGroups...)
	}

	return groups, nil
}

func (db *DBClient) selectDeviceGroups(ctx context.Context, criteria DeviceGroupsQueryCriteria) ([]deviceGroupsItem, error) {
	var queryB bytes.Buffer
	if err := SelectUserDeviceGroupsTemplate.Execute(&queryB, criteria); err != nil {
		return nil, err
	}

	query := fmt.Sprintf(queryB.String(), db.Prefix)
	params := table.NewQueryParameters(
		table.ValueParam("$huid", ydb.Uint64Value(tools.Huidify(criteria.UserID))),
		table.ValueParam("$device_id", ydb.StringValue([]byte(criteria.DeviceID))),
		table.ValueParam("$group_id", ydb.StringValue([]byte(criteria.GroupID))),
	)

	res, err := db.Read(ctx, query, params)
	if err != nil {
		return nil, xerrors.Errorf("failed to get deviceGroups from DB: %w", err)
	}

	if !res.NextSet() {
		return nil, xerrors.New("result set not found")
	}

	deviceGroups := make([]deviceGroupsItem, 0, res.SetRowCount())
	for res.NextRow() {
		var item deviceGroupsItem

		res.NextItem()
		item.DeviceID = string(res.OString())
		res.NextItem()
		item.GroupID = string(res.OString())

		if res.Err() != nil {
			err := xerrors.Errorf("failed to parse YDB response row: %w", res.Err())
			ctxlog.Error(ctx, db.Logger, err.Error())
			return nil, err
		}

		deviceGroups = append(deviceGroups, item)
	}

	return deviceGroups, nil
}

func (db *DBClient) updateDeviceGroupsInTx(ctx context.Context, s *table.Session, tx *table.Transaction, changeSet []deviceGroupRaw) error {
	if len(changeSet) > 0 {
		upsertQuery := fmt.Sprintf(`
			--!syntax_v1
			PRAGMA TablePathPrefix("%s");

			DECLARE $values AS List<Struct<
				huid: Uint64,
				user_id: Uint64,
				device_id: String,
				group_id: String
			>>;

			UPSERT INTO
				DeviceGroups ( huid, user_id, device_id, group_id )
			SELECT
				huid, user_id, device_id, group_id
			FROM AS_TABLE($values);
		`, db.Prefix)
		values := make([]ydb.Value, 0, len(changeSet))
		for _, deviceGroup := range changeSet {
			values = append(values, deviceGroup.ToYDBValue())
		}
		upsertParams := table.NewQueryParameters(
			table.ValueParam("$values", ydb.ListValue(values...)),
		)
		upsertQueryStmt, err := s.Prepare(ctx, upsertQuery)
		if err != nil {
			return err
		}
		_, err = tx.ExecuteStatement(ctx, upsertQueryStmt, upsertParams)
		if err != nil {
			return err
		}
	}
	return nil
}
