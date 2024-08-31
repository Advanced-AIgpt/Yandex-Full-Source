package pgdb

import (
	"context"
	"database/sql"
	"database/sql/driver"
	"encoding/json"
	"reflect"

	xmodel "a.yandex-team.ru/alice/iot/adapters/xiaomi_adapter/xiaomi/model"
	"a.yandex-team.ru/alice/library/go/slices"
	"a.yandex-team.ru/library/go/core/log/ctxlog"
	"a.yandex-team.ru/library/go/core/xerrors"
)

var _ = sql.Scanner(&userIDs{})
var _ = driver.Value(userIDs{})

type userIDs []uint64

func (user userIDs) Value() (driver.Value, error) {
	bytes, err := json.Marshal(user)
	return string(bytes), err
}

func (user *userIDs) Scan(value interface{}) error {
	str, ok := value.(string)
	if !ok {
		return xerrors.Errorf("type assertion to string failed, actual type: %s", reflect.TypeOf(value).String())
	}

	return json.Unmarshal([]byte(str), &user)
}

func (c *Client) selectExternalUser(ctx context.Context, tx *sql.Tx, externalUserID string) (*xmodel.ExternalUser, error) {
	query := `
		SELECT
			user_ids, subscription_key
		FROM
			xiaomi.external_users
		WHERE
			external_user_id = $1
	`
	var ids userIDs
	var subscriptionKey string
	if err := tx.QueryRowContext(ctx, query, externalUserID).Scan(&ids, &subscriptionKey); err != nil {
		switch {
		case xerrors.Is(err, sql.ErrNoRows):
			return nil, nil
		default:
			return nil, err
		}
	}
	return &xmodel.ExternalUser{
		ExternalUserID:  externalUserID,
		UserIDs:         ids,
		SubscriptionKey: subscriptionKey,
	}, nil
}

func (c *Client) storeExternalUser(ctx context.Context, tx *sql.Tx, user xmodel.ExternalUser) error {
	query := `
		INSERT INTO
			xiaomi.external_users(external_user_id, user_ids, subscription_key)
		VALUES
			($1, $2, $3)
		ON CONFLICT ON CONSTRAINT pk_external_users DO
		    UPDATE SET user_ids = $2, subscription_key = $3
	`
	_, err := tx.ExecContext(ctx, query, user.ExternalUserID, userIDs(user.UserIDs), user.SubscriptionKey)
	return err
}

func (c *Client) deleteExternalUser(ctx context.Context, tx *sql.Tx, externalUserID string) error {
	deleteExternalUserQuery := `
		DELETE FROM
			xiaomi.external_users
		WHERE
			external_user_id = $1;
	`
	if _, err := tx.ExecContext(ctx, deleteExternalUserQuery, externalUserID); err != nil {
		return xerrors.Errorf("unable to delete external user: %w", err)
	}
	return nil
}

func (c *Client) SelectExternalUser(ctx context.Context, externalUserID string) (*xmodel.ExternalUser, error) {
	query := `
		SELECT
			user_ids, subscription_key
		FROM
			xiaomi.external_users
		WHERE
			external_user_id = $1
	`

	db, err := c.pgClient.GetMaster()
	if err != nil {
		return nil, err
	}

	var ids userIDs
	var subscriptionKey string
	if err := db.QueryRowContext(ctx, query, externalUserID).Scan(&ids, &subscriptionKey); err != nil {
		switch {
		case xerrors.Is(err, sql.ErrNoRows):
			return nil, nil
		default:
			return nil, err
		}
	}
	return &xmodel.ExternalUser{
		ExternalUserID:  externalUserID,
		UserIDs:         ids,
		SubscriptionKey: subscriptionKey,
	}, nil
}

func (c *Client) StoreExternalUser(ctx context.Context, externalUserID string, userID uint64) (err error) {
	db, err := c.pgClient.GetMaster()
	if err != nil {
		return err
	}
	tx, err := db.BeginTx(ctx, &sql.TxOptions{Isolation: sql.LevelSerializable})
	defer func() {
		if tx != nil {
			_ = c.closeTransaction(ctx, tx, err)
		}
	}()

	var user *xmodel.ExternalUser
	user, err = c.selectExternalUser(ctx, tx, externalUserID)
	if err != nil {
		return err
	}

	if user == nil {
		uuid, err := c.uuidGenerator.NewV4()
		if err != nil {
			return err
		}
		user = &xmodel.ExternalUser{
			ExternalUserID:  externalUserID,
			UserIDs:         []uint64{},
			SubscriptionKey: uuid.String(),
		}
	}

	// it's important to put last added user in the beginning of users list to avoid token invalidation
	// more details https://st.yandex-team.ru/IOT-1528
	user.UserIDs = append([]uint64{userID}, user.UserIDs...)
	user.UserIDs = slices.DedupUint64sKeepOrder(user.UserIDs)

	return c.storeExternalUser(ctx, tx, *user)
}

func (c *Client) DeleteExternalUser(ctx context.Context, externalUserID string, userIDToDelete uint64) error {
	db, err := c.pgClient.GetMaster()
	if err != nil {
		return err
	}
	tx, err := db.BeginTx(ctx, &sql.TxOptions{Isolation: sql.LevelSerializable})
	defer func() {
		if tx != nil {
			_ = c.closeTransaction(ctx, tx, err)
		}
	}()

	user, err := c.selectExternalUser(ctx, tx, externalUserID)
	if err != nil {
		return xerrors.Errorf("unable to select external user: %w", err)
	}
	if user == nil {
		ctxlog.Info(ctx, c.logger, "user not found")
		return nil
	}
	restUserIDs := make([]uint64, 0, len(user.UserIDs))
	for _, userID := range user.UserIDs {
		if userID == userIDToDelete {
			continue
		}
		restUserIDs = append(restUserIDs, userID)
	}
	user.UserIDs = restUserIDs

	if len(restUserIDs) > 0 {
		if err = c.storeExternalUser(ctx, tx, *user); err != nil {
			return xerrors.Errorf("unable to store external user: %w", err)
		}
		return nil
	}

	if err = c.deleteExternalUser(ctx, tx, externalUserID); err != nil {
		return xerrors.Errorf("unable to delete user and subscriptions: %w", err)
	}
	return nil
}
