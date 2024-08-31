package dbmigration

import (
	"a.yandex-team.ru/alice/iot/bulbasaur/db/dao"
	"a.yandex-team.ru/alice/library/go/ydbclient"
)

type Chunker interface {
	CreateEmptyRowItem() ydbclient.YDBRow

	// AddRow call for every row from ChunkTable as sql query order (usually unordered).
	// https://a.yandex-team.ru/arc_vcs/kikimr/public/sdk/go/ydb/internal/result/result.go?rev=r8636044#L954
	// return true - row accepted to the chunk
	// return false - row not accepted to the chunk. Chunk must be reset after AddRow return false.
	// then the row will send to AddRow again
	AddRow(row ydbclient.YDBRow) bool

	// Query query, used as rows source for chunker.
	// It must be simple query - SELECT * FROM Table or SELECT * FROM Table WHERE ...
	// Use ORDER BY, GROUP BY, etc is danger for large amount of data.
	Query() string

	// Reset internal state of chunker
	// called for start new chunk after AddRow return false.
	// Chunker must accept any first row after reset
	Reset()
}

type CounterChunker struct {
	MaxRowCount int

	emptyItemFunc func() ydbclient.YDBRow
	sourceQuery   string
	counter       int
}

func (c *CounterChunker) CreateEmptyRowItem() ydbclient.YDBRow {
	return c.emptyItemFunc()
}

func (c *CounterChunker) Query() string {
	return c.sourceQuery
}

func (c *CounterChunker) AddRow(_ ydbclient.YDBRow) bool {
	if c.counter >= c.MaxRowCount {
		return false
	}

	c.counter++
	return true
}

func (c *CounterChunker) Reset() {
	c.counter = 0
}

func NewCounterChunker(sourceQuery string, emptyItemFunc func() ydbclient.YDBRow) *CounterChunker {
	return &CounterChunker{
		emptyItemFunc: emptyItemFunc,
		sourceQuery:   sourceQuery,
		MaxRowCount:   ydbRowCountLimit,
		counter:       0,
	}
}

type HuidChunker struct {
	MaxHuids int

	emptyItemFunc func() dao.HuidRow
	sourceQuery   string
	huids         map[uint64]struct{}
}

func NewHuidChunker(sourceQuery string, emptyItemFunc func() dao.HuidRow) *HuidChunker {
	return &HuidChunker{
		emptyItemFunc: emptyItemFunc,
		sourceQuery:   sourceQuery,
		MaxHuids:      100,
		huids:         make(map[uint64]struct{}),
	}
}

func (c *HuidChunker) CreateEmptyRowItem() ydbclient.YDBRow {
	return c.emptyItemFunc()
}

func (c *HuidChunker) AddRow(row ydbclient.YDBRow) bool {
	huid := row.(dao.HuidRow).GetHuid()
	if _, exist := c.huids[huid]; exist {
		return true
	}

	if len(c.huids) >= c.MaxHuids {
		return false
	}

	c.huids[huid] = struct{}{}
	return true
}

func (c *HuidChunker) Query() string {
	return c.sourceQuery
}

func (c *HuidChunker) Reset() {
	c.huids = make(map[uint64]struct{})
}
