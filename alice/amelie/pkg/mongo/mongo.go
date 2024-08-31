package mongo

import (
	"context"

	"go.mongodb.org/mongo-driver/mongo"
	"go.mongodb.org/mongo-driver/mongo/options"
	"go.mongodb.org/mongo-driver/x/bsonx"
)

const (
	IndexAlreadyExists = "INDEX_ALREADY_EXISTS"
)

// IndexAlreadyExistsError identifies that the index with such key already exists.
type IndexAlreadyExistsError struct{}

func (err *IndexAlreadyExistsError) Error() string {
	return IndexAlreadyExists
}

// CreateIndex tries to create unique index with the key over the mongo.Collection.
// CreateIndex returns the IndexAlreadyExistsError if there is an index with the same name.
// CreateIndex forwards underlying mongo.Error as-is if any is returned.
func CreateIndex(ctx context.Context, coll *mongo.Collection, key string) error {
	indexView := coll.Indexes()
	cursor, err := indexView.List(ctx)
	if err != nil {
		return err
	}
	for cursor.Next(ctx) {
		doc := &bsonx.Doc{}
		if err := cursor.Decode(doc); err != nil {
			return err
		}
		if value, ok := doc.Lookup("name").StringValueOK(); ok && value == key {
			return &IndexAlreadyExistsError{}
		}
	}
	index := mongo.IndexModel{
		Keys:    bsonx.Doc{}.Append(key, bsonx.Int32(1)),
		Options: options.Index().SetName(key).SetUnique(true),
	}
	_, err = indexView.CreateOne(ctx, index)
	return err
}
