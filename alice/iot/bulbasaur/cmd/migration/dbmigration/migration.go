package dbmigration

import (
	"context"
	"time"

	"a.yandex-team.ru/alice/library/go/ydbclient"

	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/log/ctxlog"

	model_db "a.yandex-team.ru/alice/iot/bulbasaur/db"
	"a.yandex-team.ru/kikimr/public/sdk/go/ydb/table"
	"a.yandex-team.ru/library/go/core/xerrors"
)

const (
	chunksReadAhead                    = 2
	keysReadAhead                      = 1000
	printMigrationStatusIntervalChunks = 10
	migrationRetryCount                = 10
)

type Chunk struct {
	// Rows contain keys fields from chunker
	// Rows ordered by primary key from chunked table
	Rows []ydbclient.YDBRow
}

func splitChunks(chunks chan<- Chunk, rows <-chan ydbclient.YDBRow, chunker Chunker) {
	chunker.Reset()
	var chunkRows []ydbclient.YDBRow

	sendChunk := func() {
		if len(chunkRows) > 0 {
			chunks <- Chunk{chunkRows}
		}
	}

	for row := range rows {
		if !chunker.AddRow(row) {
			sendChunk()
			chunkRows = []ydbclient.YDBRow{}

			chunker.Reset()
			_ = chunker.AddRow(row) // empty chunk must accept first row
		}
		chunkRows = append(chunkRows, row)
	}

	// send last chunk
	sendChunk()
}

// MigrateChunkFunc must be stateless migration func
// it may be called more then once per chunk (for repeat transactions or repeat source table scan)
// it can be called in parallels for different chunks.
// all db operations must be go through CallInTx (mdb.Read, mdb.Write - ok).
// chunk contains one or more rows
type MigrateChunkFunc func(transactionCtx context.Context, mdb *Client, chunk Chunk) error

func MigrateDatabase(ctx context.Context, logger log.Logger, mdb *Client, chunker Chunker, migrateChunk MigrateChunkFunc) error {
	ctx, cancel := context.WithCancel(ctx)
	defer cancel()

	keyRows := make(chan ydbclient.YDBRow, keysReadAhead)
	chunks := make(chan Chunk, chunksReadAhead)

	var readTableError error

	go func() {
		defer close(keyRows)
		readTableError = mdb.streamReadQuery(ctx, chunker.Query(), chunker.CreateEmptyRowItem, keyRows)
	}()

	go func() {
		defer close(chunks)
		splitChunks(chunks, keyRows, chunker)
	}()

	chunkNum := 0
	rowsNum := 0
	for chunk := range chunks {
		// retry chunk few times - for work with flaps in middle of migration
		chunkRetries := 0
		for {
			err := mdb.RetryTransaction(ctx, model_db.SerializableReadWrite, func(ctx context.Context, s *table.Session, txControl *table.TransactionControl) (*table.Transaction, error) {
				return nil, migrateChunk(ctx, mdb, chunk)
			})
			if err == nil {
				// stop retry, continue work
				break
			}
			if chunkRetries >= migrationRetryCount {
				return xerrors.Errorf("failed to migrate chunk %v: %w", chunk.Rows[0], err)
			}

			chunkRetries++
			ctxlog.Infof(ctx, logger, "chunk migration error, wait and retry : %+v", err)
			time.Sleep(time.Second * 5)
			ctxlog.Debugf(ctx, logger, "wait completed, retry once more (attempt: %v)", chunkRetries)
		}
		chunkNum++
		rowsNum += len(chunk.Rows)
		if chunkNum%printMigrationStatusIntervalChunks == 0 {
			ctxlog.Infof(ctx, logger, "Completed chunks: %v, rows: %v\n", chunkNum, rowsNum)
			ctxlog.Infof(ctx, logger, "Last migrated row from chunk table: %+v\n", chunk.Rows[len(chunk.Rows)-1])
		}
	}

	if readTableError != nil {
		return xerrors.Errorf("failed to read query %q: %w", chunker.Query(), readTableError)
	}
	return nil
}
