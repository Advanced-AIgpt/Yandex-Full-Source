package db

import (
	"bytes"
	"context"
	"fmt"

	"a.yandex-team.ru/library/go/core/log/ctxlog"

	"a.yandex-team.ru/alice/iot/bulbasaur/model"
	"a.yandex-team.ru/alice/library/go/timestamp"
	"a.yandex-team.ru/alice/library/go/tools"
	"a.yandex-team.ru/kikimr/public/sdk/go/ydb"
	"a.yandex-team.ru/kikimr/public/sdk/go/ydb/table"
	"a.yandex-team.ru/library/go/core/xerrors"
)

type NetworkQueryCriteria struct {
	UserID uint64
	SSID   string
}

func (db *DBClient) selectUserNetworksSimple(ctx context.Context, criteria NetworkQueryCriteria) ([]model.Network, error) {
	var networks []model.Network
	selectFunc := func(ctx context.Context, s *table.Session, txControl *table.TransactionControl) (*table.Transaction, error) {
		var queryBuffer bytes.Buffer
		if err := SelectUserNetworksTemplate.Execute(&queryBuffer, criteria); err != nil {
			return nil, err
		}

		query := fmt.Sprintf(queryBuffer.String(), db.Prefix)
		params := table.NewQueryParameters(
			table.ValueParam("$huid", ydb.Uint64Value(tools.Huidify(criteria.UserID))),
			table.ValueParam("$ssid", ydb.StringValue([]byte(criteria.SSID))),
		)

		res, err := db.Read(ctx, query, params)
		if err != nil {
			return nil, err
		}
		if !res.NextSet() {
			return nil, xerrors.New("result set not found")
		}

		networks = make([]model.Network, 0, res.RowCount())
		for res.NextRow() {
			var network model.Network

			res.NextItem()
			network.SSID = string(res.OString())

			res.NextItem()
			network.Password = string(res.OString())

			res.NextItem()
			updated := res.OTimestamp()
			if updated != 0 {
				network.Updated = timestamp.FromMicro(updated)
			}

			if res.Err() != nil {
				ctxlog.Errorf(ctx, db.Logger, "failed to parse YDB response row: %s", res.Err())
				return nil, xerrors.Errorf("failed to parse YDB response row: %w", res.Err())
			}
			networks = append(networks, network)
		}
		return nil, nil
	}

	err := db.CallInTx(ctx, OnlineReadOnly, selectFunc)
	if err != nil {
		return networks, xerrors.Errorf("database request has failed: %w", err)
	}

	return networks, nil
}

func (db *DBClient) SelectUserNetworks(ctx context.Context, userID uint64) (model.Networks, error) {
	criteria := NetworkQueryCriteria{UserID: userID}
	networks, err := db.selectUserNetworksSimple(ctx, criteria)
	return networks, err
}

func (db *DBClient) SelectUserNetwork(ctx context.Context, userID uint64, SSID string) (model.Network, error) {
	criteria := NetworkQueryCriteria{UserID: userID, SSID: SSID}
	networks, err := db.selectUserNetworksSimple(ctx, criteria)
	if err != nil {
		return model.Network{}, err
	}
	if len(networks) == 0 {
		return model.Network{}, &model.UserNetworkNotFoundError{}
	}
	if len(networks) > 1 {
		return model.Network{}, fmt.Errorf("found more than one user network using criteria: %#v", criteria)
	}
	return networks[0], err
}

// TODO: refactor this and StoreUserDevice methods on ydb changesets
func (db *DBClient) StoreUserNetwork(ctx context.Context, userID uint64, network model.Network) error {
	networkStoreFunc := func(ctx context.Context, s *table.Session, txControl *table.TransactionControl) (*table.Transaction, error) {
		//check userNetwork already exist
		query := fmt.Sprintf(`
			--!syntax_v1
			PRAGMA TablePathPrefix("%s");
			DECLARE $huid AS Uint64;
			DECLARE $ssid AS String;

			SELECT
				COUNT(*) AS cnt
			FROM
				UserNetworks
			WHERE
				UserNetworks.huid == $huid AND
				UserNetworks.archived == false;

			SELECT
				created
			FROM
				UserNetworks
			WHERE
				huid == $huid AND
				ssid == $ssid AND
				archived == false;`, db.Prefix)
		params := table.NewQueryParameters(
			table.ValueParam("$huid", ydb.Uint64Value(tools.Huidify(userID))),
			table.ValueParam("$ssid", ydb.StringValue([]byte(network.SSID))),
		)

		stmt, err := s.Prepare(ctx, query)
		if err != nil {
			return nil, err
		}

		tx, res, err := stmt.Execute(ctx, txControl, params)
		if err != nil {
			return tx, err
		}

		// read current user networks count
		if !res.NextSet() {
			return tx, xerrors.New("can't count unarchived user networks: result set does not exist")
		}
		if !res.NextRow() {
			return tx, xerrors.New("can't read count of unarchived user networks: result set has no rows")
		}
		res.SeekItem("cnt")
		userNetworksCount := res.Uint64()
		if err := res.Err(); err != nil {
			return tx, xerrors.Errorf("can't read count of unarchived networks: %w", err)
		}

		createdValue := timestamp.Now().YdbTimestamp()
		// read current network 'created' field
		if !res.NextSet() {
			return tx, xerrors.New("can't read unarchived user networks: result set does not exist")
		}

		if res.SetRowCount() > 1 {
			return tx, fmt.Errorf("database contains non-unique primary key: <%d, %s>", tools.Huidify(userID), network.SSID)
		}

		if res.NextRow() {
			res.SeekItem("created")
			createdValue = res.OTimestamp()
		} else {
			if userNetworksCount >= model.ConstUserNetworksLimit {
				return tx, &model.UserNetworksLimitReachedError{}
			}
		}

		upsertNetworkQuery := fmt.Sprintf(`
				--!syntax_v1
				PRAGMA TablePathPrefix("%s");
				DECLARE $huid AS Uint64;
				DECLARE $user_id AS Uint64;
				DECLARE $ssid AS String;
				DECLARE $password AS String;
				DECLARE $updated AS Timestamp;
				DECLARE $created AS Timestamp;
				UPSERT INTO
					UserNetworks (huid, user_id, ssid, password, archived, updated, created)
				VALUES
					($huid, $user_id, $ssid, $password, false, $updated, $created)`, db.Prefix)
		params = table.NewQueryParameters(
			table.ValueParam("$huid", ydb.Uint64Value(tools.Huidify(userID))),
			table.ValueParam("$user_id", ydb.Uint64Value(userID)),
			table.ValueParam("$ssid", ydb.StringValue([]byte(network.SSID))),
			table.ValueParam("$password", ydb.StringValue([]byte(network.Password))),
			table.ValueParam("$updated", ydb.TimestampValue(network.Updated.YdbTimestamp())),
			table.ValueParam("$created", ydb.TimestampValue(createdValue)),
		)
		upsertNetworkQueryStmt, err := s.Prepare(ctx, upsertNetworkQuery)
		if err != nil {
			return tx, err
		}
		_, err = tx.ExecuteStatement(ctx, upsertNetworkQueryStmt, params)
		if err != nil {
			return tx, err
		}
		ctxlog.Infof(ctx, db.Logger, "Inserted new network %s for user %d", network.SSID, userID)
		return tx, nil
	}
	err := db.CallInTx(ctx, SerializableReadWrite, networkStoreFunc)
	if err != nil {
		return xerrors.Errorf("database request has failed: %w", err)
	}
	return nil
}

func (db *DBClient) DeleteUserNetwork(ctx context.Context, userID uint64, SSID string) error {
	deleteNetworkQuery := fmt.Sprintf(`
		--!syntax_v1
		PRAGMA TablePathPrefix("%s");
		DECLARE $huid AS Uint64;
		DECLARE $ssid AS String;

		UPDATE
			UserNetworks
		SET
			archived = true
		WHERE
			huid == $huid AND
			ssid == $ssid`, db.Prefix)
	params := table.NewQueryParameters(
		table.ValueParam("$huid", ydb.Uint64Value(tools.Huidify(userID))),
		table.ValueParam("$ssid", ydb.StringValue([]byte(SSID))),
	)

	if err := db.Write(ctx, deleteNetworkQuery, params); err != nil {
		return xerrors.Errorf("failed to delete user network %s of user %d: %w", SSID, userID, err)
	}

	return nil
}
