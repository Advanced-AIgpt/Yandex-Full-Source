package dbmigration

import (
	"context"
	"errors"
	"os"
	"sort"
	"sync"
	"testing"
	"time"

	"github.com/stretchr/testify/assert"
	"go.uber.org/zap/zaptest"

	"a.yandex-team.ru/alice/iot/bulbasaur/db/schema"
	zaplog "a.yandex-team.ru/library/go/core/log/zap"

	"a.yandex-team.ru/alice/library/go/ydbclient"

	"a.yandex-team.ru/alice/library/go/tools"

	"a.yandex-team.ru/kikimr/public/sdk/go/ydb"
	"a.yandex-team.ru/kikimr/public/sdk/go/ydb/table"

	"math/rand"

	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/xerrors"
)

var (
	createTableOnce sync.Once
)

type testEnv struct {
	db     *Client
	logger log.Logger
	ctx    context.Context
	at     *assert.Assertions
}

func newEnv(t *testing.T) *testEnv {
	// https://a.yandex-team.ru/arc/trunk/arcadia/kikimr/public/tools/ydb_recipe/README.md
	endpoint, ok := os.LookupEnv("YDB_ENDPOINT")
	if !ok {
		panic(errors.New("can not read YDB_ENDPOINT envvar"))
	}

	prefix, ok := os.LookupEnv("YDB_DATABASE")
	if !ok {
		panic(errors.New("can not read YDB_DATABASE envvar"))
	}

	token, ok := os.LookupEnv("YDB_TOKEN")
	if !ok {
		token = "anyNotEmptyString"
	}

	ctx, cancel := context.WithCancel(context.Background())
	t.Cleanup(cancel)
	logger := &zaplog.Logger{L: zaptest.NewLogger(t)}

	db := NewClient(logger, endpoint, prefix, token, false)

	createTableOnce.Do(func() {
		logger.Info("creating tables")
		err := schema.CreateTables(ctx, db.SessionPool, prefix, "")
		if err != nil {
			t.Fatalf("failed to create tables: %+v", err)
		}
		logger.Info("create tables completed")
	})

	return &testEnv{
		db:     db,
		logger: logger,
		ctx:    ctx,
		at:     assert.New(t),
	}
}

type userTestDAO struct {
	Huid uint64
	ID   uint64
}

func (u userTestDAO) GetHuid() uint64 {
	return u.Huid
}

func (u userTestDAO) YDBFields() []string {
	return []string{"hid", "id"}
}

func (u *userTestDAO) YDBParseResult(res *table.Result) error {
	return res.ScanWithDefaults(&u.Huid, &u.ID)
}

func (u userTestDAO) YDBDeclareStruct() string {
	return `Struct<
      hid: Uint64,
      id: Uint64,
>`
}

func (u userTestDAO) YDBStruct() (ydb.Value, error) {
	u.Huid = tools.Huidify(u.ID)

	return ydb.StructValue(
		ydb.StructFieldValue("hid", ydb.Uint64Value(u.Huid)),
		ydb.StructFieldValue("id", ydb.Uint64Value(u.ID)),
	), nil
}

func TestClientReadTable(t *testing.T) {
	const recordCount = ydbRowCountLimit * 3
	env := newEnv(t)
	ctx := env.ctx
	t.Log("fill users started")
	err := fillUsersChecked(ctx, env.db, recordCount)
	t.Log("fill users finished")
	if err != nil {
		// it can be ok for continue
		// for example record counter for large database is slow
		t.Logf("error with fill table: %+v", err)
	}

	ch := make(chan ydbclient.YDBRow)
	counter := 0

	var wg sync.WaitGroup
	wg.Add(1)
	go func() {
		defer wg.Done()
		counter++
		for range ch {
			counter++
		}
	}()

	err = env.db.streamReadQuery(ctx, env.db.PragmaPrefix("SELECT * FROM Users"), func() ydbclient.YDBRow {
		return &userTestDAO{}
	}, ch)
	env.at.NoError(err)
	close(ch)

	wg.Wait()
	env.at.GreaterOrEqual(counter, recordCount)
}

func TestReadRows(t *testing.T) {
	const recordCount = ydbRowCountLimit * 3
	env := newEnv(t)
	ctx := env.ctx
	t.Log("fill users started")
	err := fillUsersChecked(ctx, env.db, recordCount)
	t.Log("fill users finished")
	if err != nil {
		// it can be ok for continue
		// for example record counter for large database is slow
		t.Logf("error with fill table: %+v", err)
	}

	// order for test differenct from primary key - it is specially for check sort order
	// two fields special too - for check create conditions
	params := table.NewQueryParameters(table.ValueParam("$minimum_hid", ydb.Int64Value(0)))
	createItem := func() ydbclient.YDBRow {
		return &userTestDAO{}
	}
	rows, err := env.db.ReadRows(ctx, `
		DECLARE $minimum_hid AS Int64;
		SELECT
			*
		FROM
			Users
		WHERE
			hid >= $minimum_hid
`, []string{"id", "hid"}, params, createItem)

	env.at.NoError(err)

	env.at.True(sort.SliceIsSorted(rows, func(i, j int) bool {
		vI := rows[i].(*userTestDAO)
		vJ := rows[j].(*userTestDAO)
		return vI.ID < vJ.ID
	}))

	env.at.GreaterOrEqual(len(rows), recordCount)
}

func fillUsersChecked(ctx context.Context, db *Client, targetCount int) error {
	res, err := db.Read(ctx, db.PragmaPrefix(`
		DECLARE $max_count AS Uint64;
		SELECT
			COUNT(*) AS cnt
		FROM (
			SELECT
				id
			FROM
				Users
			LIMIT $max_count
		)
`), table.NewQueryParameters(table.ValueParam("$max_count", ydb.Uint64Value(uint64(targetCount)))))
	if err != nil {
		return xerrors.Errorf("failed to get ScenarioLaunches count: %w", err)
	}

	res.NextResultSet(ctx, "cnt")
	res.NextRow()

	var dbCount uint64
	err = res.Scan(&dbCount)
	if err != nil {
		return xerrors.Errorf("failed to scan ScenarioLaunches count: %w", err)
	}

	if dbCount >= uint64(targetCount) {
		return nil
	}

	additionalCount := targetCount - int(dbCount)
	launches := make([]ydbclient.YDBRow, additionalCount)
	for i := range launches {
		launches[i] = &userTestDAO{
			ID: rand.Uint64(),
		}
	}

	start := time.Now()
	chunkSize := ydbRowCountLimit

	for len(launches) > 0 {
		if err = db.StoreYDBRows(ctx, "Users", launches[:chunkSize]); err != nil {
			return xerrors.Errorf("failed to write users (time: %v): %w", time.Since(start), err)
		}
		launches = launches[chunkSize:]
	}
	return nil
}
