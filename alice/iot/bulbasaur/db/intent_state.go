package db

import (
	"context"
	"fmt"

	"a.yandex-team.ru/alice/iot/bulbasaur/model"
	"a.yandex-team.ru/alice/library/go/tools"
	"a.yandex-team.ru/kikimr/public/sdk/go/ydb"
	"a.yandex-team.ru/kikimr/public/sdk/go/ydb/table"
	"a.yandex-team.ru/library/go/core/xerrors"
)

func (db *DBClient) SelectUserIntentState(ctx context.Context, userID uint64, intentStateKey model.IntentStateKey) (model.IntentState, error) {
	query := fmt.Sprintf(`
		--!syntax_v1
		PRAGMA TablePathPrefix("%s");
		DECLARE $huid AS Uint64;
		DECLARE $device_id AS String;
		DECLARE $session_id AS String;
		DECLARE $intent AS String;
		SELECT
			state
		FROM
			IntentStates
		WHERE
			huid == $huid AND
			device_id == $device_id AND
			session_id == $session_id AND
			intent == $intent`, db.Prefix)

	params := table.NewQueryParameters(
		table.ValueParam("$huid", ydb.Uint64Value(tools.Huidify(userID))),
		table.ValueParam("$device_id", ydb.StringValue([]byte(intentStateKey.SpeakerID))),
		table.ValueParam("$session_id", ydb.StringValue([]byte(intentStateKey.SessionID))),
		table.ValueParam("$intent", ydb.StringValue([]byte(intentStateKey.Intent))),
	)

	res, err := db.Read(ctx, query, params)
	if err != nil {
		return nil, err
	}

	var stateRaw []byte

	if !res.NextResultSet(ctx, "state") {
		return nil, xerrors.Errorf("failed to get result set for user: %w", err)
	}
	if !res.NextRow() {
		return nil, xerrors.Errorf("failed to get user row: empty select result: %w", &model.IntentStateNotFoundError{})
	}
	if err := res.ScanWithDefaults(&stateRaw); err != nil {
		return nil, xerrors.Errorf("failed to scan result for intent state: %w", err)
	}

	if res.Err() != nil {
		return nil, xerrors.Errorf("failed to scan row from query: %w", res.Err())
	}

	if len(stateRaw) > 0 {
		return stateRaw, nil
	}

	return nil, &model.IntentStateNotFoundError{}
}

func (db *DBClient) StoreUserIntentState(ctx context.Context, userID uint64, intentStateKey model.IntentStateKey, intentState model.IntentState) error {
	createFunc := func(ctx context.Context, s *table.Session, txControl *table.TransactionControl) (*table.Transaction, error) {
		stateValue := ydb.NullValue(ydb.TypeJSON)
		if intentState != nil {
			stateValue = ydb.OptionalValue(ydb.JSONValue(string(intentState)))
		}
		insertQuery := fmt.Sprintf(`
			--!syntax_v1
			PRAGMA TablePathPrefix("%s");
			DECLARE $huid AS Uint64;
			DECLARE $user_id AS Uint64;
			DECLARE $device_id AS String;
			DECLARE $session_id AS String;
			DECLARE $intent AS String;
			DECLARE $state AS Optional<Json>;
			DECLARE $timestamp AS Timestamp;

			UPSERT INTO
				IntentStates (huid, user_id, device_id, session_id, intent, state, ts)
			VALUES
				($huid, $user_id, $device_id, $session_id, $intent, $state, $timestamp);`, db.Prefix)
		params := table.NewQueryParameters(
			table.ValueParam("$huid", ydb.Uint64Value(tools.Huidify(userID))),
			table.ValueParam("$user_id", ydb.Uint64Value(userID)),
			table.ValueParam("$device_id", ydb.StringValue([]byte(intentStateKey.SpeakerID))),
			table.ValueParam("$session_id", ydb.StringValue([]byte(intentStateKey.SessionID))),
			table.ValueParam("$intent", ydb.StringValue([]byte(intentStateKey.Intent))),
			table.ValueParam("$state", stateValue),
			table.ValueParam("$timestamp", ydb.TimestampValue(db.CurrentTimestamp().YdbTimestamp())),
		)
		insertQueryStmt, err := s.Prepare(ctx, insertQuery)
		if err != nil {
			return nil, err
		}
		tx, _, err := insertQueryStmt.Execute(ctx, txControl, params)
		defer func() {
			if err != nil {
				db.RollbackTransaction(ctx, tx, err)
			}
		}()

		return tx, nil
	}

	if err := db.CallInTx(ctx, SerializableReadWrite, createFunc); err != nil {
		return xerrors.Errorf("database request has failed: %w", err)
	}
	return nil
}
