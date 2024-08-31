package db

import (
	"bytes"
	"context"
	"encoding/json"
	"fmt"

	"a.yandex-team.ru/library/go/core/log/ctxlog"

	"a.yandex-team.ru/alice/iot/adapters/tuya_adapter/tuya"
	"a.yandex-team.ru/alice/iot/bulbasaur/model"
	"a.yandex-team.ru/alice/library/go/timestamp"
	"a.yandex-team.ru/alice/library/go/tools"
	"a.yandex-team.ru/kikimr/public/sdk/go/ydb"
	"a.yandex-team.ru/kikimr/public/sdk/go/ydb/table"
	"a.yandex-team.ru/library/go/core/xerrors"
)

type CustomControlQueryCriteria struct {
	UserID   string
	DeviceID string
}

func (db *DBClient) getCustomControls(ctx context.Context, s *table.Session, tc *table.TransactionControl, criteria CustomControlQueryCriteria) ([]tuya.IRCustomControl, error) {
	var queryBuffer bytes.Buffer
	if err := selectCustomControlsTemplate.Execute(&queryBuffer, criteria); err != nil {
		return []tuya.IRCustomControl{}, err
	}

	query := fmt.Sprintf(queryBuffer.String(), db.Prefix)
	params := table.NewQueryParameters(
		table.ValueParam("$huid", ydb.Uint64Value(tools.HuidifyString(criteria.UserID))),
		table.ValueParam("$id", ydb.StringValue([]byte(criteria.DeviceID))),
	)

	stmt, err := s.Prepare(ctx, query)
	if err != nil {
		return []tuya.IRCustomControl{}, err
	}

	_, res, err := stmt.Execute(ctx, tc, params)
	if err != nil {
		return []tuya.IRCustomControl{}, err
	}

	if !res.NextSet() {
		return []tuya.IRCustomControl{}, xerrors.New("result set not found")
	}

	controls := make([]tuya.IRCustomControl, 0)
	for res.NextRow() {
		var control tuya.IRCustomControl

		res.NextItem()
		control.ID = string(res.OString())

		res.NextItem()
		control.Name = string(res.OString())

		res.NextItem()
		control.DeviceType = model.DeviceType(res.OString())

		res.NextItem()
		control.Buttons = make([]tuya.IRCustomButton, 0)
		if err := json.Unmarshal([]byte(res.OJSON()), &control.Buttons); err != nil {
			ctxlog.Error(ctx, db.Logger, err.Error())
			return []tuya.IRCustomControl{}, xerrors.Errorf("failed to unmarshal custom control buttons field: %w", err)
		}
		if res.Err() != nil {
			err := xerrors.Errorf("failed to parse YDB response row: %w", res.Err())
			ctxlog.Error(ctx, db.Logger, err.Error())
			return []tuya.IRCustomControl{}, err
		}
		controls = append(controls, control)
	}

	return controls, nil
}

func (db *DBClient) SelectCustomControl(ctx context.Context, userID, deviceID string) (tuya.IRCustomControl, error) {
	controlsRes := make([]tuya.IRCustomControl, 0)
	criteria := CustomControlQueryCriteria{
		UserID:   userID,
		DeviceID: deviceID,
	}
	selectFunc := func(ctx context.Context, s *table.Session) (err error) {
		txControl := table.TxControl(table.BeginTx(table.WithOnlineReadOnly()), table.CommitTx())
		controlsRes, err = db.getCustomControls(ctx, s, txControl, criteria)
		if err != nil {
			return err
		}
		return nil
	}

	err := db.Retry(ctx, selectFunc)
	if err != nil {
		return tuya.IRCustomControl{}, xerrors.Errorf("cannot get custom control %s of user %s from database: %w", deviceID, userID, err)
	}
	if len(controlsRes) > 1 {
		return tuya.IRCustomControl{}, fmt.Errorf("found more than one custom control using criteria: %#v", criteria)
	}
	if len(controlsRes) == 0 {
		return tuya.IRCustomControl{}, xerrors.Errorf("cannot get custom control %s of user %s from database: %w", deviceID, userID, &tuya.ErrCustomControlNotFound{})
	}
	return controlsRes[0], nil
}

func (db *DBClient) SelectUserCustomControls(ctx context.Context, userID string) (tuya.IRCustomControls, error) {
	controlsRes := make(tuya.IRCustomControls, 0)
	criteria := CustomControlQueryCriteria{
		UserID: userID,
	}
	selectFunc := func(ctx context.Context, s *table.Session) (err error) {
		txControl := table.TxControl(table.BeginTx(table.WithOnlineReadOnly()), table.CommitTx())
		controlsRes, err = db.getCustomControls(ctx, s, txControl, criteria)
		if err != nil {
			return err
		}
		return nil
	}

	err := db.Retry(ctx, selectFunc)
	if err != nil {
		return controlsRes, xerrors.Errorf("cannot get custom controls of user %s from database: %w", userID, err)
	}
	return controlsRes, nil
}

func (db *DBClient) StoreCustomControl(ctx context.Context, userID string, cp tuya.IRCustomControl) error {
	controlStoreFunc := func(ctx context.Context, s *table.Session) (err error) {
		//check device already exist
		query := fmt.Sprintf(`
			--!syntax_v1
			PRAGMA TablePathPrefix("%s");
			DECLARE $huid AS Uint64;
			DECLARE $id AS String;

			SELECT
				created
			FROM
				CustomControls
			WHERE
				huid == $huid AND
				id == $id AND
				archived == false;`, db.Prefix)
		params := table.NewQueryParameters(
			table.ValueParam("$huid", ydb.Uint64Value(tools.HuidifyString(userID))),
			table.ValueParam("$id", ydb.StringValue([]byte(cp.ID))),
		)

		stmt, err := s.Prepare(ctx, query)
		if err != nil {
			return err
		}

		txControl := table.TxControl(table.BeginTx(table.WithSerializableReadWrite()))
		tx, res, err := stmt.Execute(ctx, txControl, params)
		defer func() {
			if tx != nil {
				err = db.closeTransaction(ctx, tx, err)
			}
		}()
		if err != nil {
			return err
		}

		if res.RowCount() > 1 {
			return xerrors.Errorf("failed to select custom control: table contains non-unique primary key <%d, %s>", tools.HuidifyString(userID), cp.ID)
		}
		createdValue := timestamp.Now().YdbTimestamp()
		// read current custom preset created
		if !res.NextSet() {
			if !res.NextRow() {
				return xerrors.New("can't read 'created' field of unarchived custom controls: result set has no rows")
			}
			if !res.SeekItem("created") {
				// new preset
				createdValue = res.Uint64()
			}
		}
		cp.Buttons.Normalize()
		customButtons, err := json.Marshal(cp.Buttons)
		if err != nil {
			return xerrors.Errorf("cannot marshal buttons: %w", err)
		}
		upsertControlQuery := fmt.Sprintf(`
				--!syntax_v1
				PRAGMA TablePathPrefix("%s");
				DECLARE $huid AS Uint64;
				DECLARE $id AS String;
				DECLARE $user_id AS String;
				DECLARE $name AS String;
				DECLARE $device_type AS String;
				DECLARE $buttons AS Json;
				DECLARE $created AS Timestamp;
				UPSERT INTO
					CustomControls (huid, id, user_id, name, device_type, buttons, archived, created)
				VALUES
					($huid, $id, $user_id, $name, $device_type, $buttons, false, $created)`, db.Prefix)
		params = table.NewQueryParameters(
			table.ValueParam("$huid", ydb.Uint64Value(tools.HuidifyString(userID))),
			table.ValueParam("$id", ydb.StringValue([]byte(cp.ID))),
			table.ValueParam("$user_id", ydb.StringValue([]byte(userID))),
			table.ValueParam("$name", ydb.StringValue([]byte(cp.Name))),
			table.ValueParam("$device_type", ydb.StringValue([]byte(cp.DeviceType))),
			table.ValueParam("$buttons", ydb.JSONValue(string(customButtons))),
			table.ValueParam("$created", ydb.TimestampValue(createdValue)),
		)
		_, err = tx.Execute(ctx, upsertControlQuery, params)
		if err != nil {
			return err
		}
		ctxlog.Infof(ctx, db.Logger, "Stored custom control %s for user %s", cp.ID, userID)
		return nil
	}
	err := db.Retry(ctx, controlStoreFunc)
	if err != nil {
		return xerrors.Errorf("database request has failed: %w", err)
	}
	return nil
}

func (db *DBClient) DeleteCustomControl(ctx context.Context, userID, controlID string) error {
	deleteControlQuery := fmt.Sprintf(`
		--!syntax_v1
		PRAGMA TablePathPrefix("%s");
		DECLARE $huid AS Uint64;
		DECLARE $id AS String;

		UPDATE
			CustomControls
		SET
			archived = true
		WHERE
			huid == $huid AND
			id == $id`, db.Prefix)
	params := table.NewQueryParameters(
		table.ValueParam("$huid", ydb.Uint64Value(tools.HuidifyString(userID))),
		table.ValueParam("$id", ydb.StringValue([]byte(controlID))),
	)

	if err := db.Write(ctx, deleteControlQuery, params); err != nil {
		return xerrors.Errorf("failed to delete custom control %s of user %s: %w", controlID, userID, err)
	}

	return nil
}
