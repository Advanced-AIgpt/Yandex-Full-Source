package session

import (
	"context"
	"fmt"
	"go.mongodb.org/mongo-driver/bson"
	"go.mongodb.org/mongo-driver/bson/primitive"
	"go.mongodb.org/mongo-driver/mongo"
	"go.mongodb.org/mongo-driver/mongo/options"
	"go.mongodb.org/mongo-driver/x/bsonx"
	"go.uber.org/zap"
)

const (
	_chatIDIndex = "chat_id"
)

type MongoStorage struct {
	coll *mongo.Collection
}

func NewMongoStorage(coll *mongo.Collection) *MongoStorage {
	return &MongoStorage{
		coll: coll,
	}
}

func (storage *MongoStorage) LoadByObjectID(objectID string) (*Session, error) {
	id, err := primitive.ObjectIDFromHex(objectID)
	if err != nil {
		return nil, fmt.Errorf("invalid objectID: %w", err)
	}
	result := storage.coll.FindOne(context.Background(), bson.M{"_id": id})
	return storage.onFindOneResult(result)
}

func (storage *MongoStorage) Count() (int64, error) {
	cnt, err := storage.coll.CountDocuments(context.Background(), bson.D{})
	if err != nil {
		return 0, fmt.Errorf("unable to cound documents: %w", err)
	}
	return cnt, nil
}

func (storage *MongoStorage) Load(chatID int64) (*Session, error) {
	return storage.LoadWithContext(context.Background(), chatID)
}

func (storage *MongoStorage) LoadWithContext(ctx context.Context, chatID int64) (*Session, error) {
	result := storage.coll.FindOne(ctx, bson.M{_chatIDIndex: chatID})
	return storage.onFindOneResult(result)
}

func (storage *MongoStorage) Save(session *Session) error {
	return storage.SaveWithContext(context.Background(), session)
}

func (storage *MongoStorage) SaveWithContext(ctx context.Context, session *Session) error {
	filter := bson.M{_chatIDIndex: session.ChatID}
	update := bson.M{"$set": newSessionView(session)}
	updateOptions := options.Update().SetUpsert(true)
	if _, err := storage.coll.UpdateOne(ctx, filter, update, updateOptions); err != nil {
		return err
	}
	return nil
}

func (storage *MongoStorage) BuildIndex(ctx context.Context, sugar *zap.SugaredLogger) error {
	indexes := storage.coll.Indexes()
	cursor, err := indexes.List(ctx)
	if err != nil {
		return fmt.Errorf("unable to list indices: %w", err)
	}
	for cursor.Next(ctx) {
		doc := &bsonx.Doc{}
		if err := cursor.Decode(doc); err != nil {
			return fmt.Errorf("unable to decode bson index document: %w", err)
		}
		if value, ok := doc.Lookup("name").StringValueOK(); ok && value == _chatIDIndex {
			sugar.Infof("Index %s already exists", value)
			return nil
		}
	}
	index := mongo.IndexModel{
		Keys:    bsonx.Doc{}.Append(_chatIDIndex, bsonx.Int32(1)),
		Options: options.Index().SetName(_chatIDIndex).SetUnique(true).SetBackground(true),
	}
	name, err := indexes.CreateOne(ctx, index)
	if err != nil {
		return err
	}
	sugar.Infof("Index %s successfully created", name)
	return nil
}

func (storage *MongoStorage) onFindOneResult(result *mongo.SingleResult) (*Session, error) {
	if err := result.Err(); err != nil {
		if err == mongo.ErrNoDocuments {
			return nil, ErrSessionNotFound
		}
		return nil, err
	}
	session := new(Session)
	if err := result.Decode(session); err != nil {
		return nil, fmt.Errorf("unable to decode session: %w", err)
	}
	return session, nil
}
