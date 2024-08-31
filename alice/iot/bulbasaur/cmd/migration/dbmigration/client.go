package dbmigration

import (
	"context"
	"fmt"
	"os"
	"strings"
	"time"

	"golang.org/x/exp/slices"

	model_db "a.yandex-team.ru/alice/iot/bulbasaur/db"
	"a.yandex-team.ru/alice/library/go/ydbclient"
	"a.yandex-team.ru/kikimr/public/sdk/go/ydb"
	"a.yandex-team.ru/kikimr/public/sdk/go/ydb/table"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/log/ctxlog"
	"a.yandex-team.ru/library/go/core/xerrors"
)

const (
	ydbRowCountLimit = 1000
)

type Client struct {
	endpoint string
	logger   log.Logger
	*model_db.DBClient
}

func (mdb *Client) Description() string {
	return fmt.Sprintf("endpoint: %q, prefix: %q", mdb.endpoint, mdb.Prefix)
}

func NewClient(logger log.Logger, endpoint, ydbPrefix, ydbToken string, trace bool, opts ...ydbclient.Options) *Client {
	ctx, cancel := context.WithTimeout(context.Background(), time.Second*10)
	defer cancel()

	dbClient, err := model_db.NewClient(ctx, logger, endpoint, ydbPrefix, ydb.AuthTokenCredentials{AuthToken: ydbToken}, trace, opts...)
	if err != nil {
		panic(err.Error())
	}

	return &Client{
		endpoint: endpoint,
		logger:   logger,
		DBClient: dbClient,
	}
}

func NewClientFromEnv(logger log.Logger) *Client {
	endpoint := os.Getenv("YDB_ENDPOINT")
	if len(endpoint) == 0 {
		panic("YDB_ENDPOINT env is not set")
	}

	prefix := os.Getenv("YDB_PREFIX")
	if len(prefix) == 0 {
		panic("YDB_PREFIX env is not set")
	}

	token := os.Getenv("YDB_TOKEN")
	if len(token) == 0 {
		panic("YDB_TOKEN env is not set")
	}

	_, trace := os.LookupEnv("YDB_DEBUG")

	return NewClient(logger, endpoint, prefix, token, trace)
}

// ReadTable read full table
func (mdb Client) streamReadQuery(ctx context.Context, query string, createItem func() ydbclient.YDBRow, readStream chan<- ydbclient.YDBRow) error {
	ctx = ydb.WithOperationTimeout(ctx, time.Second*120)
	session, err := mdb.TableClient.CreateSession(ctx)
	if err != nil {
		return xerrors.Errorf("failed to create ydb session: %w", err)
	}

	var opts = []table.ReadTableOption{table.ReadOrdered()}

	fieldNames := createItem().YDBFields()
	for _, fieldName := range fieldNames {
		opts = append(opts, table.ReadColumn(fieldName))
	}

	res, err := session.StreamExecuteScanQuery(ctx, query, nil)
	if err != nil {
		return xerrors.Errorf("failed to start stream read query %q: %w", query, err)
	}

	for res.NextResultSet(ctx, fieldNames...) {
		for res.NextRow() {
			row := createItem()
			if err := row.YDBParseResult(res); err != nil {
				return xerrors.Errorf("failed to scan row from query %q: %w", query, err)
			}
			readStream <- row
		}
	}

	if err := res.Err(); err != nil {
		return xerrors.Errorf("failed to read result from query %q: %w", query, err)
	}
	return nil
}

// ReadRows - draft
// don't use channel to return rows because transactions doesn't work in parallel goroutines
// query - string query with range templates
// query = `
// DECLARE $huid AS UInt64;
// SELECT
//    huid, id, value
// FROM
//    table_name
// WHERE
//   huid = $huid
// `
//
// params = ydb.N
// rows, err := ReadRows(ctx, query, []string{"huid", "id", "value"}, []string{"id"}, params)
//
// the query MUST have WHERE section, contains at least one condition and WHERE must be last section.
// it converted internal to
// PRAGMA TablePathPrefix("<db prefix>");
// DECLARE $orderField1 AS OrderFieldType1;
// DECLARE $orderField2 AS OrderFieldType2;
//
// -- original query part
// DECLARE $huid AS UInt64;
// SELECT
//    huid, id, value
// FROM
//    table_name
// WHERE
//   huid = $huid
// --end original query
//
// AND $orderField1 >= $orderField1LastValue AND $orderField2 >= $orderField2LastValue
// AND NOT ($orderField1 = $orderField1LastValue AND $orderField2 = $orderField2LastValue)
// ORDER BY orderField1, orderField2
func (mdb Client) ReadRows(ctx context.Context, query string, orderFields []string, params *table.QueryParameters, itemFabric func() ydbclient.YDBRow) (resRows []ydbclient.YDBRow, err error) {
	originalQuery := query
	var rangeKeys []ydb.Value

	if len(orderFields) == 0 {
		return nil, xerrors.New("orderFields must contain one or more fields")
	}

	selectFields := itemFabric().YDBFields()
	for _, fieldName := range orderFields {
		if !slices.Contains(selectFields, fieldName) {
			return nil, xerrors.Errorf("struct ydb fields doesn't contains field: %q", fieldName)
		}
	}

	for {
		query, queryParams, err := buildQuery(orderFields, rangeKeys, params, originalQuery)
		if err != nil {
			return nil, xerrors.Errorf("failed to build read rows query: %w", err)
		}
		query = mdb.PragmaPrefix(query)
		ctxlog.Infof(ctx, mdb.logger, "query: %v (%v)", query, queryParams)

		var truncatedResponse bool
		err = mdb.CallInTx(ctx, model_db.SerializableReadWrite, func(ctx context.Context, s *table.Session, txControl *table.TransactionControl) (*table.Transaction, error) {
			tx, res, err := s.Execute(ctx, txControl, query, queryParams, table.WithQueryCachePolicy(table.WithQueryCachePolicyKeepInCache()))
			if err != nil {
				err = xerrors.Errorf("failed to read rows page: %w", err)
				return tx, err
			}

			if !res.NextResultSet(ctx, itemFabric().YDBFields()...) {
				return tx, xerrors.New("failed to get readset")
			}
			truncatedResponse = res.Truncated()

			for res.NextRow() {
				row := itemFabric()
				err := row.YDBParseResult(res)
				if err != nil {
					return tx, err
				}
				resRows = append(resRows, row)

				if !res.HasNextRow() {
					rangeKeys = make([]ydb.Value, 0, len(orderFields))
					for _, fieldName := range orderFields {
						if !res.SeekItem(fieldName) {
							return tx, xerrors.Errorf("failed to seek order field field: %q", fieldName)
						}
						rangeKeys = append(rangeKeys, res.Value())
					}
				}
			}
			if res.Err() != nil {
				return tx, xerrors.Errorf("failed to parse result: %w", err)
			}
			return tx, nil
		})

		if err != nil {
			return nil, xerrors.Errorf("failed to read rows page from %+v: %w", rangeKeys, err)
		}

		if !truncatedResponse {
			break
		}
	}
	return resRows, nil
}

func buildQuery(orderFields []string, prevOrderValues []ydb.Value, originalParams *table.QueryParameters, originalQuery string) (string, *table.QueryParameters, error) {
	var pagerDeclare, pagerWhere, pagerOrder string
	pagerOrder = strings.Join(orderFields, ", ")
	var queryParams *table.QueryParameters
	if len(prevOrderValues) == 0 {
		pagerDeclare = ""
		pagerWhere = ""
		queryParams = originalParams
	} else {
		fields, err := toYdbFields(orderFields, prevOrderValues)
		if err != nil {
			return "", nil, xerrors.Errorf("failed to create ydb fields: %w", err)
		}
		pagerDeclare, err = buildDeclareFromFields(fields)
		if err != nil {
			return "", nil, xerrors.Errorf("failed to build declare section for pagination request: %w", err)
		}
		pagerWhere = " AND " + buildRangeWhereFromFields(orderFields)
		queryParams, err = buildQueryParams(originalParams, fields)
		if err != nil {
			return "", nil, xerrors.Errorf("failed to build ydb query params: %w", err)
		}
	}

	query := fmt.Sprintf(`--!syntax_v1
%s
%s
%s
ORDER BY %v;
`, pagerDeclare, originalQuery, pagerWhere, pagerOrder)
	return query, queryParams, nil
}
