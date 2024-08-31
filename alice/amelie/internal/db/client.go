package db

import (
	"context"
	"errors"
	"fmt"
	"time"

	"github.com/gofrs/uuid"
	"go.mongodb.org/mongo-driver/bson"
	"go.mongodb.org/mongo-driver/mongo"
	"go.mongodb.org/mongo-driver/mongo/options"

	"a.yandex-team.ru/alice/amelie/internal/model"
	"a.yandex-team.ru/alice/amelie/pkg/logging"
	mongoHelper "a.yandex-team.ru/alice/amelie/pkg/mongo"
	"a.yandex-team.ru/library/go/core/log/ctxlog"
)

const (
	sessionsCollection = "sessions"
	sessionIDKey       = "session_id"
)

type MongoClient struct {
	db *mongo.Database
}

func NewMongoClient(ctx context.Context, db *mongo.Database) (*MongoClient, error) {
	client := &MongoClient{
		db: db,
	}
	if err := client.createIndexes(ctx); err != nil {
		return nil, fmt.Errorf("unable to create MongoDB index: %w", err)
	}
	return client, nil
}

func (m *MongoClient) createIndexes(ctx context.Context) error {
	return createIndex(ctx, m.sessions(), sessionIDKey)
}

func createIndex(ctx context.Context, coll *mongo.Collection, key string) error {
	logger := logging.ServiceLogger(ctx)
	meta := fmt.Sprintf("collection=%s key=%s", coll.Name(), key)
	ctxlog.Infof(ctx, logger, "Trying to create MongoDB index: %s", meta)
	if err := mongoHelper.CreateIndex(ctx, coll, key); err != nil {
		if errors.Is(err, &mongoHelper.IndexAlreadyExistsError{}) {
			ctxlog.Infof(ctx, logger, "Index already exists: %s", meta)
			return nil
		}
		return err
	}
	ctxlog.Infof(ctx, logger, "Index successfully created: %s", meta)
	return nil
}

func (m *MongoClient) Count(ctx context.Context) (int64, error) {
	cnt, err := m.sessions().EstimatedDocumentCount(ctx)
	if err != nil {
		return 0, fmt.Errorf("unable to cound documents: %w", err)
	}
	return cnt, nil
}

func (m *MongoClient) Load(ctx context.Context, sessionID string) (model.Session, error) {
	flashback := model.RateFlashback{
		Time: time.Now(),
		ID:   uuid.Must(uuid.NewV4()).String(),
	}
	result := m.sessions().FindOneAndUpdate(ctx, bson.M{sessionIDKey: sessionID},
		bson.M{
			"$push": bson.M{
				"rate.rps_history": flashback,
			},
		},
		new(options.FindOneAndUpdateOptions).SetUpsert(true).SetReturnDocument(options.After),
	)
	if err := m.checkFindOneResult(result); err != nil {
		return model.Session{}, err
	}
	session, err := m.decodeSession(result)
	if err != nil {
		return session, err
	}
	for i := 0; i < len(session.Rate.RPSHistory); i++ {
		if session.Rate.RPSHistory[i].ID == flashback.ID {
			session.Rate.RPSHistory[i].Actual = true
		}
	}
	return session, nil
}

func (m *MongoClient) Save(ctx context.Context, session model.Session) error {
	filter := bson.M{sessionIDKey: session.ID}
	update := bson.M{
		"$set": bson.M{
			"last_update_time": time.Now(),
			"user":             session.User,
			"state":            session.State,
			"uuid":             session.UUID,
		},
		"$pull": bson.M{
			"rate.rps_history": bson.M{
				"time": bson.M{
					"$lt": session.Rate.RPSHistory[len(session.Rate.RPSHistory)-1].Time,
				},
			},
		},
	}
	updateOptions := options.Update().SetUpsert(true)
	if _, err := m.sessions().UpdateOne(ctx, filter, update, updateOptions); err != nil {
		return &model.DBError{InternalError: err}
	}
	return nil
}

func (m *MongoClient) sessions() *mongo.Collection {
	return m.db.Collection(sessionsCollection)
}

func (m *MongoClient) checkFindOneResult(r *mongo.SingleResult) error {
	if err := r.Err(); err != nil {
		if err == mongo.ErrNoDocuments {
			return &model.NotFoundError{}
		}
		return &model.DBError{InternalError: err}
	}
	return nil
}

func (m *MongoClient) decodeSession(raw *mongo.SingleResult) (model.Session, error) {
	session := model.Session{}
	if err := raw.Decode(&session); err != nil {
		return model.Session{}, &model.SessionCorruptedError{InternalError: err}
	}
	return session, nil
}
