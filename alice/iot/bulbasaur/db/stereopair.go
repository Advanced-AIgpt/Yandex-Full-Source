package db

import (
	"context"
	"encoding/json"
	"fmt"

	"a.yandex-team.ru/alice/iot/bulbasaur/model"
	"a.yandex-team.ru/alice/library/go/timestamp"
	"a.yandex-team.ru/alice/library/go/tools"
	"a.yandex-team.ru/kikimr/public/sdk/go/ydb"
	"a.yandex-team.ru/kikimr/public/sdk/go/ydb/table"
	"a.yandex-team.ru/library/go/core/xerrors"
)

type stereopairsRow struct {
	HUID       uint64
	UserID     uint64
	ID         string
	Name       string
	Created    timestamp.PastTimestamp
	Config     json.RawMessage
	Archived   bool
	ArchivedAt timestamp.PastTimestamp
}

func (r *stereopairsRow) FromStereopair(userID uint64, sp model.Stereopair) error {
	r.HUID = tools.Huidify(userID)
	r.UserID = userID
	r.ID = sp.ID
	r.Name = sp.Name
	r.Created = sp.Created

	var err error
	r.Config, err = json.Marshal(sp.Config)
	if err != nil {
		return xerrors.Errorf("failed to marshal stereopair config: %w", err)
	}

	r.Archived = false
	r.ArchivedAt = 0
	return nil
}

func (r *stereopairsRow) FromYDBResult(res *table.Result) error {
	if !res.SeekItem("huid") {
		return xerrors.Errorf(`can't find field "huid"`)
	}
	r.HUID = res.OUint64()

	if !res.SeekItem("user_id") {
		return xerrors.Errorf(`can't find field "user_id"`)
	}
	r.UserID = res.OUint64()

	if !res.SeekItem("id") {
		return xerrors.New(`can't find field "id"'`)
	}
	r.ID = string(res.OString())

	if !res.SeekItem("name") {
		return xerrors.New(`can't find field "name"'`)
	}
	r.Name = string(res.OString())

	if !res.SeekItem("created") {
		return xerrors.New(`can't find field "created"'`)
	}
	r.Created = timestamp.FromMicro(res.OTimestamp())

	if !res.SeekItem("config") {
		return xerrors.New(`can't find field "config"`)
	}
	r.Config = json.RawMessage(res.OJSON())

	if !res.SeekItem("archived") {
		return xerrors.New(`can't find field "archived"'`)
	}
	r.Archived = res.OBool()

	if !res.SeekItem("archived_at") {
		return xerrors.New(`can't find field "archived_at"'`)
	}
	if res.IsNull() {
		r.ArchivedAt = 0
	} else {
		r.ArchivedAt = timestamp.FromMicro(res.OTimestamp())
	}

	return nil
}

func (r *stereopairsRow) ToStereopair() (model.Stereopair, error) {
	var cfg model.StereopairConfig
	err := json.Unmarshal(r.Config, &cfg)
	if err != nil {
		return model.Stereopair{}, xerrors.Errorf("failed to unmarshal stereopair config: %w", err)
	}

	sp := model.Stereopair{
		ID:      r.ID,
		Name:    r.Name,
		Config:  cfg,
		Created: r.Created,
	}
	return sp, nil
}

func (r stereopairsRow) YDBStructDeclare() string {
	return `Struct<
		huid: Uint64,
		user_id: Uint64,
		id: String,
		name: String,
		created: Timestamp,
		config: Json,
		archived: Bool,
		archived_at: Timestamp?
	>`
}

func (r *stereopairsRow) ToYDBStruct() ydb.Value {
	var archivedAtValue = ydb.NullValue(ydb.TypeTimestamp)
	if r.Archived {
		archivedAtValue = ydb.OptionalValue(ydb.TimestampValue(r.ArchivedAt.YdbTimestamp()))
	}
	res := ydb.StructValue(
		ydb.StructFieldValue("huid", ydb.Uint64Value(r.HUID)),
		ydb.StructFieldValue("user_id", ydb.Uint64Value(r.UserID)),
		ydb.StructFieldValue("id", ydb.StringValue([]byte(r.ID))),
		ydb.StructFieldValue("name", ydb.StringValue([]byte(r.Name))),
		ydb.StructFieldValue("created", ydb.TimestampValue(r.Created.YdbTimestamp())),
		ydb.StructFieldValue("config", ydb.JSONValue(string(r.Config))),
		ydb.StructFieldValue("archived", ydb.BoolValue(r.Archived)),
		ydb.StructFieldValue("archived_at", archivedAtValue),
	)
	return res
}

func (db *DBClient) UpdateStereopairName(ctx context.Context, userID uint64, stereopairID, oldName, newName string) error {
	return db.CallInTx(ctx, SerializableReadWrite, func(ctx context.Context, _ *table.Session, _ *table.TransactionControl) (*table.Transaction, error) {
		stereopair, err := db.SelectStereopair(ctx, userID, stereopairID)
		if err != nil {
			return nil, xerrors.Errorf("failed to select stereopair: %w", err)
		}
		if stereopair.Name != oldName {
			return nil, xerrors.Errorf("failed to rename stereopair: stereopair name not equal to oldName name: %q != %q", stereopair.Name, oldName)
		}
		stereopair.Name = newName
		return nil, db.StoreStereopair(ctx, userID, stereopair)
	})
}

func (db *DBClient) StoreStereopair(ctx context.Context, userID uint64, stereopair model.Stereopair) error {
	return db.storeStereopairs(ctx, userID, model.Stereopairs{stereopair})
}

func (db *DBClient) storeStereopairs(ctx context.Context, userID uint64, stereopairs model.Stereopairs) error {
	if len(stereopairs) == 0 {
		return nil
	}

	query := fmt.Sprintf(`--!syntax_v1
	PRAGMA TablePathPrefix("%s");

	DECLARE $values AS List<%s>;

	UPSERT INTO Stereopairs
	SELECT
		stereopairs.*
	FROM AS_TABLE($values) as stereopairs
`, db.Prefix, stereopairsRow{}.YDBStructDeclare())

	values := make([]ydb.Value, 0, len(stereopairs))
	for _, pair := range stereopairs {
		var row stereopairsRow
		err := row.FromStereopair(userID, pair)
		if err != nil {
			return xerrors.Errorf("failed create row struct for stereopair with id %q: %w", pair.ID, err)
		}
		values = append(values, row.ToYDBStruct())
	}

	return db.Write(ctx, query, table.NewQueryParameters(table.ValueParam("$values", ydb.ListValue(values...))))
}

func (db *DBClient) SelectStereopair(ctx context.Context, userID uint64, stereopairID string) (model.Stereopair, error) {
	stereopairs, err := db.SelectStereopairs(ctx, userID)
	if err != nil {
		return model.Stereopair{}, err
	}

	stereopair, ok := stereopairs.GetByID(stereopairID)
	if !ok {
		return model.Stereopair{}, xerrors.Errorf("stereopair not found in database: %q", stereopairID)
	}
	return stereopair, nil
}

func (db *DBClient) SelectStereopairs(ctx context.Context, userID uint64) (model.Stereopairs, error) {
	stereopairs, err := db.SelectStereopairsSimple(ctx, userID)
	if err != nil {
		return nil, xerrors.Errorf("failed to select stereopairs: %w", err)
	}

	if len(stereopairs) == 0 {
		return stereopairs, nil
	}

	devices, err := db.SelectUserDevices(ctx, userID)
	if err != nil {
		return nil, xerrors.Errorf("failed to select devices: %w", err)
	}

	devicesMap := devices.ToMap()
	err = fillStereopairsFromDevices(stereopairs, devicesMap)
	if err != nil {
		return nil, xerrors.Errorf("failed to fill stereopair from devices map: %w", err)
	}

	return stereopairs, nil
}

func fillStereopairsFromDevices(stereopairs model.Stereopairs, devicesMap model.DevicesMapByID) error {
	for i := range stereopairs {
		stereopairs[i].Devices = make(model.Devices, 0, len(stereopairs[i].Config.Devices))
		for _, deviceConfig := range stereopairs[i].Config.Devices {
			device, ok := devicesMap[deviceConfig.ID]
			if ok {
				stereopairs[i].Devices = append(stereopairs[i].Devices, device)
			} else {
				return xerrors.Errorf("failed to find device for stereopair: stereopair_id %q, device_id: %q",
					stereopairs[i].ID, deviceConfig.ID)
			}
		}
	}
	return nil
}

func (db *DBClient) SelectStereopairsSimple(ctx context.Context, userID uint64) (model.Stereopairs, error) {
	query := fmt.Sprintf(`--!syntax_v1
			PRAGMA TablePathPrefix("%s");

			DECLARE $huid AS Uint64;

			SELECT
				*
			FROM
				Stereopairs
			WHERE
				huid = $huid AND
				archived == false;
		`, db.Prefix)

	res, err := db.Read(ctx, query, table.NewQueryParameters(
		table.ValueParam("$huid", ydb.Uint64Value(tools.Huidify(userID))),
	))

	if err != nil {
		return nil, xerrors.Errorf("failed to select stereopairs: %w", err)
	}

	if !res.NextSet() {
		return nil, xerrors.Errorf("no result sets found")
	}

	stereopairs, err := readStereopairsResultSet(res)
	if err != nil {
		return nil, xerrors.Errorf("failed to read stereopair result set: %w", err)
	}
	return stereopairs, nil
}

func (db *DBClient) DeleteStereopair(ctx context.Context, userID uint64, id string) error {
	return db.deleteStereopairs(ctx, userID, []string{id})
}
func (db *DBClient) deleteStereopairs(ctx context.Context, userID uint64, ids []string) error {
	if len(ids) == 0 {
		return nil
	}
	query := fmt.Sprintf(`--!syntax_v1
	PRAGMA TablePathPrefix("%s");

	DECLARE $huid AS Uint64;
	DECLARE $now AS Timestamp;
	DECLARE $ids AS List<Struct<id: String>>;

	UPDATE
		Stereopairs
	SET
		archived = true,
		archived_at = $now
	WHERE
		huid = $huid AND
		archived == false AND
		id IN (SELECT id FROM AS_TABLE($ids))
`, db.Prefix)
	ydbIds := make([]ydb.Value, 0, len(ids))
	for _, id := range ids {
		ydbIds = append(ydbIds, ydb.StructValue(
			ydb.StructFieldValue("id", ydb.StringValue([]byte(id))),
		),
		)
	}
	now := timestamp.CurrentTimestampCtx(ctx).YdbTimestamp()
	return db.Write(ctx, query, table.NewQueryParameters(
		table.ValueParam("$huid", ydb.Uint64Value(tools.Huidify(userID))),
		table.ValueParam("$now", ydb.TimestampValue(now)),
		table.ValueParam("$ids", ydb.ListValue(ydbIds...)),
	))
}

func readStereopairsResultSet(tableRes *table.Result) (model.Stereopairs, error) {
	var result = make(model.Stereopairs, 0, tableRes.RowCount())

	for tableRes.NextRow() {
		var spr stereopairsRow
		err := spr.FromYDBResult(tableRes)
		if err != nil {
			return nil, xerrors.Errorf("failed to read stereopair from db: %w", err)
		}

		sp, err := spr.ToStereopair()
		if err != nil {
			return nil, xerrors.Errorf("failed to create stereopair from db row: %w", err)
		}

		result = append(result, sp)
	}
	return result, nil
}
