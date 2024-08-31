package db

import (
	"context"
	"fmt"
	"regexp"
	"strings"

	"a.yandex-team.ru/kikimr/public/sdk/go/ydb"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/xerrors"

	"a.yandex-team.ru/alice/library/go/timestamp"
	"a.yandex-team.ru/alice/library/go/ydbclient"
	"a.yandex-team.ru/kikimr/public/sdk/go/ydb/table"
)

type DBClient struct {
	timestamper timestamp.ITimestamper

	StaleReadTransactionType TransactionType
	*ydbclient.YDBClient
}

var (
	pragmaPrefixRegexp = regexp.MustCompile(`PRAGMA\s+TablePathPrefix\s*\(`)
)

func NewClient(ctx context.Context, logger log.Logger, endpoint, prefix string, credentials ydb.Credentials, trace bool, options ...ydbclient.Options) (*DBClient, error) {
	ydbClient, err := ydbclient.NewYDBClient(ctx, logger, endpoint, prefix, credentials, trace, options...)
	if err != nil {
		return nil, xerrors.Errorf("unable to create ydbClient: %w", err)
	}
	return NewClientWithYDBClient(ydbClient), nil
}

func NewClientWithYDBClient(ydbClient *ydbclient.YDBClient) *DBClient {
	return &DBClient{
		timestamper:              timestamp.NewTimestamper(),
		StaleReadTransactionType: StaleReadOnly,
		YDBClient:                ydbClient,
	}
}

func (db *DBClient) CurrentTimestamp() timestamp.PastTimestamp {
	return db.timestamper.CurrentTimestamp()
}

// for testing purposes only
func (db *DBClient) SetTimestamper(timestamper timestamp.ITimestamper) {
	db.timestamper = timestamper
}

func (db *DBClient) PragmaPrefix(query string) string {
	if pragmaPrefixRegexp.MatchString(query) {
		panic("failed to add second pragma prefix")
	}
	var prefix string
	if strings.HasPrefix(strings.TrimSpace(query), "--!syntax_v1") {
		query = strings.TrimSpace(query)
		query = strings.TrimSpace(strings.TrimPrefix(query, "--!syntax_v1"))
		prefix = "--!syntax_v1"
	}
	res := fmt.Sprintf(
		`%s
		PRAGMA TablePathPrefix("%s");
		%s`,
		prefix, db.Prefix, query)
	return res
}

func (db *DBClient) StoreYDBRows(ctx context.Context, tableName string, rows []ydbclient.YDBRow) (err error) {
	if len(rows) == 0 {
		return nil
	}

	query := db.PragmaPrefix(fmt.Sprintf(`
		--!syntax_v1
		DECLARE $values AS List<%s>;

		UPSERT INTO
			%s
		SELECT
			*
		FROM AS_TABLE($values);
`, rows[0].YDBDeclareStruct(), tableName))

	values := make([]ydb.Value, len(rows))
	for i, row := range rows {
		values[i], err = row.YDBStruct()
		if err != nil {
			return xerrors.Errorf("failed to convert row to ydb struct: %w", err)
		}
	}

	params := table.NewQueryParameters(
		table.ValueParam("$values", ydb.ListValue(values...)),
	)

	return db.Write(ctx, query, params)
}

func optionalYdbTimestampValue(timestamp timestamp.PastTimestamp) ydb.Value {
	if timestamp > 0 {
		return ydb.OptionalValue(ydb.TimestampValue(timestamp.YdbTimestamp()))
	} else {
		return ydb.NullValue(ydb.TypeTimestamp)
	}

}
