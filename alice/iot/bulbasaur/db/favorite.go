package db

import (
	"context"
	"encoding/json"
	"fmt"

	"a.yandex-team.ru/alice/iot/bulbasaur/model"
	"a.yandex-team.ru/alice/library/go/tools"
	"a.yandex-team.ru/kikimr/public/sdk/go/ydb"
	"a.yandex-team.ru/kikimr/public/sdk/go/ydb/table"
	"a.yandex-team.ru/library/go/core/log/ctxlog"
	"a.yandex-team.ru/library/go/core/xerrors"
)

func (db *DBClient) SelectFavorites(ctx context.Context, user model.User) (model.Favorites, error) {
	userInfo, err := db.SelectUserInfo(ctx, user.ID)
	if err != nil {
		return model.Favorites{}, xerrors.Errorf("failed to select user info: %w", err)
	}
	return userInfo.Favorites(), nil
}

func (db *DBClient) StoreFavoriteScenarios(ctx context.Context, user model.User, scenarios model.Scenarios) error {
	rawFavorites := make(rawFavorites, 0, len(scenarios))
	for _, scenario := range scenarios {
		var rawFavorite rawFavorite
		rawFavorite.FromScenario(scenario)
		rawFavorites = append(rawFavorites, rawFavorite)
	}
	return db.upsertRawFavorites(ctx, user, rawFavorites)
}

func (db *DBClient) DeleteFavoriteScenarios(ctx context.Context, user model.User, scenarios model.Scenarios) error {
	rawFavorites := make(rawFavorites, 0, len(scenarios))
	for _, scenario := range scenarios {
		var rawFavorite rawFavorite
		rawFavorite.FromScenario(scenario)
		rawFavorites = append(rawFavorites, rawFavorite)
	}
	return db.deleteRawFavorites(ctx, user, rawFavorites)
}

func (db *DBClient) StoreFavoriteDevices(ctx context.Context, user model.User, devices model.Devices) error {
	rawFavorites := make(rawFavorites, 0, len(devices))
	for _, device := range devices {
		var rawFavorite rawFavorite
		rawFavorite.FromDevice(device)
		rawFavorites = append(rawFavorites, rawFavorite)
	}
	return db.upsertRawFavorites(ctx, user, rawFavorites)
}

func (db *DBClient) DeleteFavoriteDevices(ctx context.Context, user model.User, devices model.Devices) error {
	rawFavorites := make(rawFavorites, 0, len(devices))
	for _, device := range devices {
		var rawFavorite rawFavorite
		rawFavorite.FromDevice(device)
		rawFavorites = append(rawFavorites, rawFavorite)
	}
	return db.deleteRawFavorites(ctx, user, rawFavorites)
}

func (db *DBClient) StoreFavoriteGroups(ctx context.Context, user model.User, groups model.Groups) error {
	rawFavorites := make(rawFavorites, 0, len(groups))
	for _, group := range groups {
		var rawFavorite rawFavorite
		rawFavorite.FromGroup(group)
		rawFavorites = append(rawFavorites, rawFavorite)
	}
	return db.upsertRawFavorites(ctx, user, rawFavorites)
}

func (db *DBClient) DeleteFavoriteGroups(ctx context.Context, user model.User, groups model.Groups) error {
	rawFavorites := make(rawFavorites, 0, len(groups))
	for _, group := range groups {
		var rawFavorite rawFavorite
		rawFavorite.FromGroup(group)
		rawFavorites = append(rawFavorites, rawFavorite)
	}
	return db.deleteRawFavorites(ctx, user, rawFavorites)
}

func (db *DBClient) StoreFavoriteProperties(ctx context.Context, user model.User, properties model.FavoritesDeviceProperties) error {
	rawFavorites := make(rawFavorites, 0, len(properties))
	for _, property := range properties {
		var rawFavorite rawFavorite
		rawFavorite.FromFavoritesDeviceProperty(property)
		rawFavorites = append(rawFavorites, rawFavorite)
	}
	return db.upsertRawFavorites(ctx, user, rawFavorites)
}

func (db *DBClient) DeleteFavoriteProperties(ctx context.Context, user model.User, properties model.FavoritesDeviceProperties) error {
	rawFavorites := make(rawFavorites, 0, len(properties))
	for _, property := range properties {
		var rawFavorite rawFavorite
		rawFavorite.FromFavoritesDeviceProperty(property)
		rawFavorites = append(rawFavorites, rawFavorite)
	}
	return db.deleteRawFavorites(ctx, user, rawFavorites)
}

func (db *DBClient) ReplaceFavoriteScenarios(ctx context.Context, user model.User, scenarios model.Scenarios) error {
	replaceFunc := func(ctx context.Context, s *table.Session, txControl *table.TransactionControl) (*table.Transaction, error) {
		if err := db.deleteFavoritesByType(ctx, user, model.ScenarioFavoriteType); err != nil {
			return nil, xerrors.Errorf("failed to delete favorites by type %s: %w", model.ScenarioFavoriteType, err)
		}
		if err := db.StoreFavoriteScenarios(ctx, user, scenarios); err != nil {
			return nil, xerrors.Errorf("failed to store favorite scenarios: %w", err)
		}
		return nil, nil
	}
	return db.CallInTx(ctx, SerializableReadWrite, replaceFunc)
}

func (db *DBClient) ReplaceFavoriteDevices(ctx context.Context, user model.User, devices model.Devices) error {
	replaceFunc := func(ctx context.Context, s *table.Session, txControl *table.TransactionControl) (*table.Transaction, error) {
		if err := db.deleteFavoritesByType(ctx, user, model.DeviceFavoriteType); err != nil {
			return nil, xerrors.Errorf("failed to delete favorites by type %s: %w", model.DeviceFavoriteType, err)
		}
		if err := db.StoreFavoriteDevices(ctx, user, devices); err != nil {
			return nil, xerrors.Errorf("failed to store favorite devices: %w", err)
		}
		return nil, nil
	}
	return db.CallInTx(ctx, SerializableReadWrite, replaceFunc)
}

func (db *DBClient) ReplaceFavoriteProperties(ctx context.Context, user model.User, properties model.FavoritesDeviceProperties) error {
	replaceFunc := func(ctx context.Context, s *table.Session, txControl *table.TransactionControl) (*table.Transaction, error) {
		if err := db.deleteFavoritesByType(ctx, user, model.DevicePropertyFavoriteType); err != nil {
			return nil, xerrors.Errorf("failed to delete favorites by type %s: %w", model.DevicePropertyFavoriteType, err)
		}
		if err := db.StoreFavoriteProperties(ctx, user, properties); err != nil {
			return nil, xerrors.Errorf("failed to store favorite device properties: %w", err)
		}
		return nil, nil
	}
	return db.CallInTx(ctx, SerializableReadWrite, replaceFunc)

}

func (db *DBClient) ReplaceFavoriteGroups(ctx context.Context, user model.User, groups model.Groups) error {
	replaceFunc := func(ctx context.Context, s *table.Session, txControl *table.TransactionControl) (*table.Transaction, error) {
		if err := db.deleteFavoritesByType(ctx, user, model.GroupFavoriteType); err != nil {
			return nil, xerrors.Errorf("failed to delete favorites by type %s: %w", model.GroupFavoriteType, err)
		}
		if err := db.StoreFavoriteGroups(ctx, user, groups); err != nil {
			return nil, xerrors.Errorf("failed to store favorite group: %w", err)
		}
		return nil, nil
	}
	return db.CallInTx(ctx, SerializableReadWrite, replaceFunc)
}

func (db *DBClient) deleteFavoritesByType(ctx context.Context, user model.User, favoriteType model.FavoriteType) error {
	query := fmt.Sprintf(`
			--!syntax_v1
			PRAGMA TablePathPrefix("%s");
			DECLARE $huid AS Uint64;
			DECLARE $type AS String;

			DELETE FROM
				Favorites
			WHERE huid == $huid AND type == $type;`, db.Prefix)
	params := table.NewQueryParameters(
		table.ValueParam("$huid", ydb.Uint64Value(tools.Huidify(user.ID))),
		table.ValueParam("$type", ydb.StringValue([]byte(favoriteType))),
	)
	return db.Write(ctx, query, params)
}

func (db *DBClient) upsertRawFavorites(ctx context.Context, user model.User, rawFavorites rawFavorites) error {
	if len(rawFavorites) == 0 {
		return nil
	}
	query := fmt.Sprintf(`
			--!syntax_v1
			PRAGMA TablePathPrefix("%s");
			DECLARE $values AS List<Struct<
				huid: Uint64,
				target_id: String,
				user_id: Uint64,
				type: String,
				key: String,
				parameters: Json?
			>>;

			UPSERT INTO
				Favorites (huid, target_id, user_id, type, key, parameters)
			SELECT
				huid, target_id, user_id, type, key, parameters
			FROM AS_TABLE($values);`, db.Prefix)

	queryChangeSetParams := make([]ydb.Value, 0, len(rawFavorites))
	for _, rawF := range rawFavorites {
		var storeChangeset rawFavoritesStoreChangeset
		storeChangeset.FromRawFavorite(user.ID, rawF)
		ydbValue, err := storeChangeset.ToStructValue()
		if err != nil {
			return xerrors.Errorf("failed to store raw favorites: %w", err)
		}
		queryChangeSetParams = append(queryChangeSetParams, ydbValue)
	}
	params := table.NewQueryParameters(
		table.ValueParam("$values", ydb.ListValue(queryChangeSetParams...)),
	)
	return db.Write(ctx, query, params)
}

func (db *DBClient) deleteRawFavorites(ctx context.Context, user model.User, rawFavorites rawFavorites) error {
	if len(rawFavorites) == 0 {
		return nil
	}
	query := fmt.Sprintf(`
			--!syntax_v1
			PRAGMA TablePathPrefix("%s");
			DECLARE $values AS List<Struct<
				huid: Uint64,
				target_id: String,
				type: String,
				key: String
			>>;

			DELETE FROM
				Favorites
			ON
				SELECT huid, target_id, type, key FROM AS_TABLE($values);`, db.Prefix)

	queryChangeSetParams := make([]ydb.Value, 0, len(rawFavorites))
	for _, rawF := range rawFavorites {
		var deleteChangeset rawFavoritesDeleteChangeSet
		deleteChangeset.FromRawFavorite(user.ID, rawF)
		ydbValue := deleteChangeset.ToStructValue()
		queryChangeSetParams = append(queryChangeSetParams, ydbValue)
	}
	params := table.NewQueryParameters(
		table.ValueParam("$values", ydb.ListValue(queryChangeSetParams...)),
	)
	return db.Write(ctx, query, params)
}

type rawFavoritesStoreChangeset struct {
	TargetID   string
	UserID     uint64
	Type       model.FavoriteType
	Key        string
	Parameters iRawFavoriteParameters
}

func (c *rawFavoritesStoreChangeset) FromRawFavorite(userID uint64, rawFavorite rawFavorite) {
	c.TargetID = rawFavorite.TargetID
	c.UserID = userID
	c.Type = rawFavorite.Type
	c.Key = rawFavorite.Key
	c.Parameters = rawFavorite.Parameters
}

func (c rawFavoritesStoreChangeset) ToStructValue() (ydb.Value, error) {
	paramsValue := ydb.NullValue(ydb.TypeJSON)
	if c.Parameters != nil {
		rawParams, err := json.Marshal(c.Parameters)
		if err != nil {
			return nil, xerrors.Errorf("failed to marshal raw favorite parameters: %w", err)
		}
		paramsValue = ydb.OptionalValue(ydb.JSONValue(string(rawParams)))
	}

	return ydb.StructValue(
		ydb.StructFieldValue("huid", ydb.Uint64Value(tools.Huidify(c.UserID))),
		ydb.StructFieldValue("user_id", ydb.Uint64Value(c.UserID)),
		ydb.StructFieldValue("target_id", ydb.StringValue([]byte(c.TargetID))),
		ydb.StructFieldValue("type", ydb.StringValue([]byte(c.Type))),
		ydb.StructFieldValue("key", ydb.StringValue([]byte(c.Key))),
		ydb.StructFieldValue("parameters", paramsValue),
	), nil
}

type rawFavoritesDeleteChangeSet struct {
	TargetID string
	UserID   uint64
	Type     model.FavoriteType
	Key      string
}

func (c *rawFavoritesDeleteChangeSet) FromRawFavorite(userID uint64, rawFavorite rawFavorite) {
	c.TargetID = rawFavorite.TargetID
	c.UserID = userID
	c.Type = rawFavorite.Type
	c.Key = rawFavorite.Key
}

func (c rawFavoritesDeleteChangeSet) ToStructValue() ydb.Value {
	return ydb.StructValue(
		ydb.StructFieldValue("huid", ydb.Uint64Value(tools.Huidify(c.UserID))),
		ydb.StructFieldValue("target_id", ydb.StringValue([]byte(c.TargetID))),
		ydb.StructFieldValue("type", ydb.StringValue([]byte(c.Type))),
		ydb.StructFieldValue("key", ydb.StringValue([]byte(c.Key))),
	)
}

func (db *DBClient) selectRawFavorites(ctx context.Context, userID uint64) (rawFavorites, error) {
	query := fmt.Sprintf(`
		--!syntax_v1
		PRAGMA TablePathPrefix("%s");
		DECLARE $huid AS Uint64;

		SELECT
			type,
			target_id,
			key,
			parameters
		FROM Favorites
		WHERE
			huid == $huid;`, db.Prefix)

	params := table.NewQueryParameters(
		table.ValueParam("$huid", ydb.Uint64Value(tools.Huidify(userID))),
	)

	res, err := db.Read(ctx, query, params)
	if err != nil {
		return nil, err
	}

	if !res.NextSet() {
		return nil, xerrors.New("result set not found")
	}

	var rawFavorites rawFavorites

	for res.NextRow() {
		rawFavorite, err := db.parseRawFavorite(ctx, res)
		if err != nil {
			return nil, err
		}

		if res.Err() != nil {
			err := xerrors.Errorf("failed to parse YDB response row: %w", res.Err())
			ctxlog.Error(ctx, db.Logger, err.Error())
			return nil, err
		}

		rawFavorites = append(rawFavorites, rawFavorite)
	}
	return rawFavorites, nil
}

func (db *DBClient) parseRawFavorite(ctx context.Context, res *table.Result) (rawFavorite, error) {
	var rawFavorite rawFavorite

	// type
	res.SeekItem("type")
	rawFavorite.Type = model.FavoriteType(res.OString())
	// target_id
	res.NextItem()
	rawFavorite.TargetID = string(res.OString())
	// key
	res.NextItem()
	rawFavorite.Key = string(res.OString())
	// parameters
	res.NextItem()
	byteParameters := res.OJSON()
	if len(byteParameters) != 0 {
		parameters, err := JSONUnmarshalRawFavoriteParameters(rawFavorite.Type, []byte(byteParameters))
		if err != nil {
			return rawFavorite, err
		}
		rawFavorite.Parameters = parameters
	}

	return rawFavorite, nil
}
