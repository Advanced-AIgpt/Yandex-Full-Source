package db

import (
	"context"
	"encoding/json"
	"fmt"

	"a.yandex-team.ru/alice/iot/bulbasaur/controller/experiments"
	"a.yandex-team.ru/library/go/core/xerrors"

	"a.yandex-team.ru/kikimr/public/sdk/go/ydb/table"
)

func (db *DBClient) SelectAllExperiments(ctx context.Context) (experiments.Experiments, error) {
	var allExperiments experiments.Experiments
	selectExperiments := func(ctx context.Context, s *table.Session, txControl *table.TransactionControl) (*table.Transaction, error) {
		groupsQuery := fmt.Sprintf(`
			--!syntax_v1
			PRAGMA TablePathPrefix("%s");

			SELECT
				   group_id, user_ids
			FROM
				ExperimentsUserGroup`, db.Prefix)

		groupsStmt, err := s.Prepare(ctx, groupsQuery)
		if err != nil {
			return nil, xerrors.Errorf("prepare experiment groups query: %w", err)
		}

		tx, groupsRes, err := groupsStmt.Execute(ctx, txControl, nil)
		if err != nil {
			return tx, xerrors.Errorf("execute experiment user groups read: %w", err)
		}

		groupUsers := make(map[string][]uint64)

		groupsRes.NextSet()
		for groupsRes.NextRow() {
			groupsRes.NextItem()
			groupID := string(groupsRes.OString())

			groupsRes.NextItem()
			userIDsJSON := groupsRes.OJSON()
			var groupUserIds []uint64
			if userIDsJSON != "" {
				err = json.Unmarshal([]byte(userIDsJSON), &groupUserIds)
				if err != nil {
					return tx, xerrors.Errorf("unmarshal user ids in group %q: %w", groupID, err)
				}
			}
			groupUsers[groupID] = groupUserIds
		}

		expQuery := fmt.Sprintf(`
		--!syntax_v1
		PRAGMA TablePathPrefix("%s");

		SELECT
			name,is_enabled,user_ids,allow_staff,allow_all,group_ids
		FROM
			Experiments`, db.Prefix)

		expStmt, err := s.Prepare(ctx, expQuery)
		if err != nil {
			return tx, xerrors.Errorf("prepare experiments query: %w", err)
		}

		expRes, err := tx.ExecuteStatement(ctx, expStmt, nil)
		if err != nil {
			return tx, xerrors.Errorf("read experiments from db: %w", err)
		}

		if !expRes.NextSet() {
			return tx, nil
		}

		for expRes.NextRow() {
			expRes.NextItem()
			name := string(expRes.OString())

			expRes.NextItem()
			isEnabled := expRes.OBool()

			expRes.NextItem()
			rawUserIDs := expRes.OJSON()
			var userIDs []uint64
			err = json.Unmarshal([]byte(rawUserIDs), &userIDs)
			if err != nil {
				return tx, xerrors.Errorf("unmarshal user_ids in experiment %q: %w", name, err)
			}

			expRes.NextItem()
			allowStaffUsers := expRes.OBool()

			expRes.NextItem()
			allowAll := expRes.OBool()

			expRes.NextItem()

			var groupIDs []string
			rawGroupIDs := expRes.OJSON()
			if rawGroupIDs != "" {
				err = json.Unmarshal([]byte(rawGroupIDs), &groupIDs)
				if err != nil {
					return tx, xerrors.Errorf("unmarshal group_ids in experiment %q: %w", name, err)
				}
			}

			exp := experiments.NewExperiment(experiments.Name(name), isEnabled, userIDs, allowStaffUsers, allowAll)
			for _, groupID := range groupIDs {
				exp = exp.WithUserIDs(groupUsers[groupID]...)
			}

			if err = expRes.Err(); err != nil {
				return tx, xerrors.Errorf("result error: %w", err)
			}

			allExperiments = append(allExperiments, *exp)
		}

		return tx, nil
	}

	err := db.CallInTx(ctx, SerializableReadWrite, selectExperiments)
	if err != nil {
		return nil, xerrors.Errorf("select experiments from db: %w", err)
	}
	return allExperiments, nil
}
