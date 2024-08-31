package dbmigration

import (
	"testing"

	"a.yandex-team.ru/alice/iot/bulbasaur/db/dao"

	"a.yandex-team.ru/alice/library/go/ydbclient"

	"github.com/stretchr/testify/assert"
)

var (
	_ Chunker = &CounterChunker{}
)

func TestCounterChunker(t *testing.T) {
	t.Run("new-func", func(t *testing.T) {
		at := assert.New(t)

		tableName := "test-table"

		chunker := NewCounterChunker(tableName, func() ydbclient.YDBRow {
			return nil
		})
		at.Equal(tableName, chunker.Query())
		at.NotNil(chunker.emptyItemFunc)
		at.NotPanics(func() {
			chunker.CreateEmptyRowItem()
		})
	})

	t.Run("new-chunks", func(t *testing.T) {
		at := assert.New(t)
		const rowsPerChunk = 3

		chunker := NewCounterChunker("", nil)
		chunker.MaxRowCount = rowsPerChunk

		for i := 0; i <= 13; i++ {
			newChunk := chunker.AddRow(nil)

			switch i {
			case 0, 1, 2, 4, 5, 8, 10, 11, 13:
				at.True(newChunk, i)
			case 3, 6, 12:
				at.False(newChunk, i)
				chunker.Reset()
				chunker.AddRow(nil)
			case 7, 9:
				at.True(newChunk, i)
				chunker.Reset()
				chunker.AddRow(nil)
			default:
				at.FailNow("unexpected variant", i)
			}
		}
	})
}

func TestHuidChunker(t *testing.T) {
	t.Run("new-func", func(t *testing.T) {
		at := assert.New(t)

		tableName := "test-table"

		chunker := NewHuidChunker(tableName, func() dao.HuidRow {
			return nil
		})
		at.Equal(tableName, chunker.Query())
		at.NotNil(chunker.emptyItemFunc)
		at.NotPanics(func() {
			chunker.CreateEmptyRowItem()
		})
	})

	t.Run("new-chunks", func(t *testing.T) {
		at := assert.New(t)
		const maxHuidsPerChunk = 3

		chunker := NewHuidChunker("", nil)
		chunker.MaxHuids = maxHuidsPerChunk

		at.True(chunker.AddRow(&userTestDAO{Huid: 1}))
		at.True(chunker.AddRow(&userTestDAO{Huid: 2}))
		at.True(chunker.AddRow(&userTestDAO{Huid: 3}))
		at.False(chunker.AddRow(&userTestDAO{Huid: 4}))

		chunker.Reset()
		at.True(chunker.AddRow(&userTestDAO{Huid: 1}))
		at.True(chunker.AddRow(&userTestDAO{Huid: 2}))
		at.True(chunker.AddRow(&userTestDAO{Huid: 3}))
		at.True(chunker.AddRow(&userTestDAO{Huid: 3}))
		at.True(chunker.AddRow(&userTestDAO{Huid: 3}))
		at.True(chunker.AddRow(&userTestDAO{Huid: 3}))
		at.False(chunker.AddRow(&userTestDAO{Huid: 4}))
	})

}
