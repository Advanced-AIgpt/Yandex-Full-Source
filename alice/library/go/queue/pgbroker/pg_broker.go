package pgbroker

import (
	"context"
	"database/sql"
	"fmt"
	"strings"
	"time"

	sq "github.com/Masterminds/squirrel"
	"golang.yandex/hasql"

	"a.yandex-team.ru/alice/library/go/pgclient"
	"a.yandex-team.ru/alice/library/go/queue"
	"a.yandex-team.ru/library/go/core/xerrors"
)

const runningTasksFetchTimeout = 5 * time.Minute

type Broker struct {
	dbClient *pgclient.PGClient
}

func NewBroker(pgClient *pgclient.PGClient) *Broker {
	return &Broker{
		dbClient: pgClient,
	}
}

func (b *Broker) SubmitTasks(ctx context.Context, tasks []queue.Task, mergePolicy queue.MergePolicy) error {
	if len(tasks) == 0 {
		return nil
	}

	return b.dbClient.ExecuteInTransaction(ctx, hasql.Primary, func(ctx context.Context, tx *sql.Tx) error {
		return b.submitTasks(ctx, tasks, mergePolicy, tx)
	})
}

func (b *Broker) submitTasks(ctx context.Context, tasks []queue.Task, mergePolicy queue.MergePolicy, tx *sql.Tx) error {
	if len(tasks) == 0 {
		return nil
	}

	query := sq.
		Insert("iot.tasks").
		Columns("group_key", "id", "name", "state", "payload", "created_time", "scheduled_time", "updated_time", "retry_left", "merge_key")

	conflictClause := "ON CONFLICT (name, group_key, merge_key) WHERE state='READY'"
	switch mergePolicy {
	case queue.Fail: // do nothing here, cause we have uniq index for (name, group_key, merge_key) WHERE state='READY'
	case queue.Ignore:
		query = query.Suffix(conflictClause + " DO NOTHING")
	case queue.Replace:
		query = query.Suffix(conflictClause + " DO UPDATE SET payload=EXCLUDED.payload, created_time=EXCLUDED.created_time, scheduled_time=EXCLUDED.scheduled_time, updated_time=EXCLUDED.updated_time, retry_left=EXCLUDED.retry_left")
	default:
		return xerrors.Errorf("pg_broker: unsupported merge policy: %d", mergePolicy)
	}

	for i := 0; i < len(tasks); i++ {
		task := tasks[i]
		if task.ID == "" {
			task.ID = queue.GenerateTaskID()
		}

		query = query.Values(task.GroupKey, task.ID, task.Name, task.State, string(task.Payload),
			task.CreatedTime.UTC(), task.ScheduledTime.UTC(), task.UpdatedTime.UTC(), task.RetryLeft, task.MergeKey)
	}

	stmt, args, err := query.PlaceholderFormat(sq.Dollar).ToSql()
	if err != nil {
		return err
	}

	if _, err = tx.ExecContext(ctx, stmt, args...); err != nil {
		return err
	}

	return nil
}

func (b *Broker) GetTasksAndUpdateState(ctx context.Context, taskName string, limit int) ([]queue.Task, error) {
	return b.getTasksAndUpdateState(ctx, taskName, limit, false)
}

func (b *Broker) GetLostTasksAndUpdateState(ctx context.Context, taskName string, limit int) ([]queue.Task, error) {
	return b.getTasksAndUpdateState(ctx, taskName, limit, true)
}

func (b *Broker) getTasksAndUpdateState(ctx context.Context, taskName string, limit int, isLostTasks bool) ([]queue.Task, error) {
	whereCondition := sq.And{
		sq.Eq{"name": taskName},
		sq.Eq{"state": queue.Ready},
		sq.LtOrEq{"scheduled_time": time.Now().UTC()},
	}
	if isLostTasks {
		whereCondition = sq.And{
			sq.Eq{"name": taskName},
			sq.Eq{"state": queue.Running},
			sq.LtOrEq{"scheduled_time": time.Now().UTC()},
			sq.LtOrEq{"updated_time": time.Now().Add(-runningTasksFetchTimeout).UTC()},
		}
	}

	taskIDLockQuery := sq.
		Select("group_key", "id").
		From("iot.tasks").
		Where(whereCondition).
		OrderBy("scheduled_time").
		Limit(uint64(limit)).
		Suffix("FOR UPDATE SKIP LOCKED")

	taskIDLockStmt, args, err := taskIDLockQuery.PlaceholderFormat(sq.Dollar).ToSql()
	if err != nil {
		return []queue.Task{}, err
	}

	queryTemplate := `
		UPDATE
			iot.tasks AS tasks
		SET
			state = 'RUNNING',
			updated_time = $%d
		FROM
			(%s) AS found
		WHERE
			tasks.group_key = found.group_key AND
			tasks.id = found.id
		RETURNING
			tasks.group_key AS group_key, tasks.id AS id, name, state, payload, created_time, scheduled_time, updated_time, retry_left, last_error, merge_key
	`
	query := fmt.Sprintf(queryTemplate, len(args)+1, taskIDLockStmt)
	args = append(args, time.Now().UTC()) // updated_time param

	db, err := b.dbClient.GetMaster()
	if err != nil {
		return []queue.Task{}, err
	}

	rows, err := db.QueryContext(ctx, query, args...)
	if err != nil {
		return []queue.Task{}, err
	}
	defer func() {
		_ = rows.Close()
	}()

	tasks := make([]queue.Task, 0, limit)
	for rows.Next() {
		t := queue.Task{}
		err := rows.Scan(&t.GroupKey, &t.ID, &t.Name, &t.State, &t.Payload, &t.CreatedTime, &t.ScheduledTime, &t.UpdatedTime, &t.RetryLeft, &t.LastError, &t.MergeKey)
		if err != nil {
			return []queue.Task{}, err
		}
		tasks = append(tasks, t)
	}

	return tasks, nil
}

func (b *Broker) SetTasksFinishedAndSubmit(ctx context.Context, finishedTasks []queue.Task, newTasks []queue.Task, mergePolicy queue.MergePolicy) error {
	if len(finishedTasks) == 0 && len(newTasks) == 0 {
		return nil
	}

	return b.dbClient.ExecuteInTransaction(ctx, hasql.Primary, func(ctx context.Context, tx *sql.Tx) error {
		if len(finishedTasks) > 0 {
			err := b.setTasksFinished(ctx, finishedTasks, tx)
			if err != nil {
				return err
			}
		}
		if len(newTasks) > 0 {
			err := b.submitTasks(ctx, newTasks, mergePolicy, tx)
			if err != nil {
				return err
			}
		}
		return nil
	})
}

func (b *Broker) SetTasksDone(ctx context.Context, tasks []queue.Task) error {
	if len(tasks) == 0 {
		return nil
	}

	values := make([]string, 0, len(tasks))
	args := make([]interface{}, 0, len(tasks)*2)
	for _, t := range tasks {
		taskVals, taskArgs := getValuesPlaceholders(len(args)+1, t.GroupKey, t.ID)
		values = append(values, taskVals)
		args = append(args, taskArgs...)
	}

	queryTemplate := `
		UPDATE
			iot.tasks AS t
		SET
			state = 'DONE',
			last_error = NULL
		FROM
			(VALUES %s) AS c(group_key, id)
		WHERE
			t.group_key = c.group_key AND
			t.id = c.id::uuid;
	`
	query := fmt.Sprintf(queryTemplate, strings.Join(values, ", "))

	db, err := b.dbClient.GetMaster()
	if err != nil {
		return err
	}

	if _, err := db.ExecContext(ctx, query, args...); err != nil {
		return err
	}

	return nil
}

func (b *Broker) SetTasksFailed(ctx context.Context, tasks []queue.Task) error {
	if len(tasks) == 0 {
		return nil
	}

	values := make([]string, 0, len(tasks))
	args := make([]interface{}, 0, len(tasks)*6)
	for _, t := range tasks {
		taskVals, taskArgs := getValuesPlaceholders(len(args)+1, t.GroupKey, t.ID, t.State, t.LastError, t.RetryLeft, t.ScheduledTime)
		values = append(values, taskVals)
		args = append(args, taskArgs...)
	}

	queryTemplate := `
		UPDATE
			iot.tasks AS t
		SET
			state = c.state::iot.task_state,
			last_error = c.last_error,
			retry_left = c.retry_left,
			scheduled_time = c.scheduled_time::timestamp
		FROM
			(VALUES %s) AS c(group_key, id, state, last_error, retry_left, scheduled_time)
		WHERE
			t.group_key = c.group_key AND
			t.id = c.id::uuid;
	`
	query := fmt.Sprintf(queryTemplate, strings.Join(values, ", "))

	db, err := b.dbClient.GetMaster()
	if err != nil {
		return err
	}

	if _, err := db.ExecContext(ctx, query, args...); err != nil {
		return err
	}

	return nil
}

func (b *Broker) setTasksFinished(ctx context.Context, tasks []queue.Task, tx *sql.Tx) error {
	if len(tasks) == 0 {
		return nil
	}

	values := make([]string, 0, len(tasks))
	args := make([]interface{}, 0, len(tasks)*4)
	for _, t := range tasks {
		taskVals, taskArgs := getValuesPlaceholders(len(args)+1, t.GroupKey, t.ID, t.State, t.LastError)
		values = append(values, taskVals)
		args = append(args, taskArgs...)
	}

	queryTemplate := `
		UPDATE
			iot.tasks AS t
		SET
			state = c.state::iot.task_state,
			last_error = c.last_error
		FROM
			(VALUES %s) AS c(group_key, id, state, last_error)
		WHERE
			t.group_key = c.group_key AND
			t.id = c.id::uuid;
	`
	query := fmt.Sprintf(queryTemplate, strings.Join(values, ", "))

	if _, err := tx.ExecContext(ctx, query, args...); err != nil {
		return err
	}

	return nil
}

func getValuesPlaceholders(startIndex int, args ...interface{}) (string, []interface{}) {
	placeholders := make([]string, 0, len(args))
	for i := startIndex; i < startIndex+len(args); i++ {
		placeholders = append(placeholders, fmt.Sprintf("$%d", i))
	}
	return fmt.Sprintf("(%s)", strings.Join(placeholders, ", ")), args
}

func (b *Broker) SetTasksReady(ctx context.Context, tasks []queue.Task) error {
	if len(tasks) == 0 {
		return nil
	}

	values := make([]string, 0, len(tasks))
	args := make([]interface{}, 0, len(tasks)*2)
	for _, t := range tasks {
		taskVals, taskArgs := getValuesPlaceholders(len(args)+1, t.GroupKey, t.ID)
		values = append(values, taskVals)
		args = append(args, taskArgs...)
	}

	queryTemplate := `
		UPDATE
			iot.tasks AS t
		SET
			state = 'READY'
		FROM
			(VALUES %s) AS c(group_key, id)
		WHERE
			t.group_key = c.group_key AND
			t.id = c.id::uuid;
	`
	query := fmt.Sprintf(queryTemplate, strings.Join(values, ", "))

	db, err := b.dbClient.GetMaster()
	if err != nil {
		return err
	}

	if _, err = db.ExecContext(ctx, query, args...); err != nil {
		return err
	}

	return nil
}

func (b *Broker) RemoveOldFinishedTasks(ctx context.Context, taskName string, threshold time.Duration, limit int) (int, error) {
	selectSubquery := sq.
		Select("group_key", "id").
		From("iot.tasks").
		Where(sq.And{
			sq.Eq{"name": taskName},
			sq.Eq{"state": []queue.State{queue.Done, queue.Failed}},
			sq.LtOrEq{"scheduled_time": time.Now().Add(-threshold).UTC()},
		}).
		OrderBy("scheduled_time").
		Limit(uint64(limit))

	selectStmt, args, err := selectSubquery.PlaceholderFormat(sq.Dollar).ToSql()
	if err != nil {
		return 0, err
	}
	deleteQuery := fmt.Sprintf(`DELETE FROM iot.tasks WHERE (group_key, id) IN (%s)`, selectStmt)

	db, err := b.dbClient.GetMaster()
	if err != nil {
		return 0, err
	}

	res, err := db.ExecContext(ctx, deleteQuery, args...)
	if err != nil {
		return 0, err
	}
	removedRowsNum, err := res.RowsAffected()
	if err != nil {
		return 0, err
	}

	return int(removedRowsNum), nil
}

func (b *Broker) Heartbeat(ctx context.Context) {
}

func (b *Broker) Cleanup(ctx context.Context) {
}
