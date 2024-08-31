package main

import (
	"context"
	"fmt"
	"path"

	"a.yandex-team.ru/alice/iot/bulbasaur/db"
	"a.yandex-team.ru/alice/iot/bulbasaur/model"
	"a.yandex-team.ru/alice/library/go/tools"
	"a.yandex-team.ru/kikimr/public/sdk/go/ydb"
	"a.yandex-team.ru/kikimr/public/sdk/go/ydb/table"
	"a.yandex-team.ru/library/go/core/log/ctxlog"
	"a.yandex-team.ru/library/go/core/xerrors"
	"a.yandex-team.ru/library/go/ptr"
	"github.com/gofrs/uuid"
)

type MigrationDBClient struct {
	*db.DBClient
}

type UserWithHousehold struct {
	ID        uint64
	Household model.Household
}

func (db *MigrationDBClient) StreamUsers(ctx context.Context) <-chan uint64 {
	usersChannel := make(chan uint64)

	go func() {
		defer close(usersChannel)

		s, err := db.SessionPool.Get(ctx)
		if err != nil {
			logger.Fatalf("Can't get session: %v", err)
		}

		usersTablePath := path.Join(db.Prefix, "Users")
		logger.Infof("Reading Users table from path %q", usersTablePath)

		res, err := s.StreamReadTable(ctx, usersTablePath,
			table.ReadColumn("id"),
			table.ReadColumn("current_household_id"),
		)
		if err != nil {
			logger.Fatalf("Failed to read table: %v", err)
		}
		defer func() {
			if err := res.Close(); err != nil {
				logger.Fatalf("Error while closing result set: %v", err)
			}
		}()

		var usersCount int
		for res.NextStreamSet(ctx) {
			for res.NextRow() {
				usersCount++

				res.SeekItem("id")
				userID := res.OUint64()

				res.SeekItem("current_household_id")
				var currentHouseholdID *string
				if !res.IsNull() {
					currentHouseholdID = ptr.String(string(res.OString()))
				}

				if err := res.Err(); err != nil {
					logger.Warnf("Error occurred while reading %s: %v", usersTablePath, err)
					continue
				}

				if currentHouseholdID == nil {
					usersChannel <- userID
				}
			}
		}

		logger.Infof("Finished reading %d users from %s", usersCount, usersTablePath)
	}()

	return usersChannel
}

func (db *MigrationDBClient) CreateCurrentHouseholdForUser(ctx context.Context, users []uint64) error {
	query := fmt.Sprintf(`
		--!syntax_v1
		PRAGMA TablePathPrefix("%s");

		DECLARE $householdValues AS List<Struct<
			huid: Uint64,
			id: String,
			user_id: Uint64,
			name: String,
			address: Optional<String>,
			short_address: Optional<String>,
			latitude: Optional<Double>,
			longitude: Optional<Double>,
			archived: Bool,
			created: Timestamp
		>>;

		DECLARE $userValues AS List<Struct<
			hid: Uint64,
			current_household_id: String
		>>;

		UPSERT INTO
			Households (huid, id, user_id, name, address, short_address, latitude, longitude, archived, created)
		SELECT
			huid, id, user_id, name, address, short_address, latitude, longitude, archived, created
		FROM AS_TABLE($householdValues);

		UPSERT INTO
			Users (hid, current_household_id)
		SELECT
			hid, current_household_id
		FROM AS_TABLE($userValues);
	`, db.Prefix)

	usersWithHouseholds := make([]UserWithHousehold, 0, len(users))
	for _, u := range users {
		uid, err := uuid.NewV4()
		if err != nil {
			ctxlog.Warnf(ctx, db.Logger, "failed to generate UUID: %v", err)
		}
		household := model.GetDefaultHousehold()
		household.ID = uid.String()
		newUser := UserWithHousehold{
			ID:        u,
			Household: household,
		}
		usersWithHouseholds = append(usersWithHouseholds, newUser)
	}

	householdValues := make([]ydb.Value, 0, len(usersWithHouseholds))
	userValues := make([]ydb.Value, 0, len(usersWithHouseholds))
	for _, u := range usersWithHouseholds {
		var addressValue ydb.Value
		var shortAddressValue ydb.Value
		var latitudeValue ydb.Value
		var longitudeValue ydb.Value
		if location := u.Household.Location; location != nil {
			longitudeValue = ydb.OptionalValue(ydb.DoubleValue(location.Longitude))
			latitudeValue = ydb.OptionalValue(ydb.DoubleValue(location.Latitude))
			addressValue = ydb.OptionalValue(ydb.StringValue([]byte(location.Address)))
			shortAddressValue = ydb.OptionalValue(ydb.StringValue([]byte(location.ShortAddress)))
		} else {
			latitudeValue = ydb.NullValue(ydb.TypeDouble)
			longitudeValue = ydb.NullValue(ydb.TypeDouble)
			addressValue = ydb.NullValue(ydb.TypeString)
			shortAddressValue = ydb.NullValue(ydb.TypeString)
		}
		householdValues = append(householdValues, ydb.StructValue(
			ydb.StructFieldValue("huid", ydb.Uint64Value(tools.Huidify(u.ID))),
			ydb.StructFieldValue("id", ydb.StringValue([]byte(u.Household.ID))),
			ydb.StructFieldValue("user_id", ydb.Uint64Value(u.ID)),
			ydb.StructFieldValue("name", ydb.StringValue([]byte(u.Household.Name))),
			ydb.StructFieldValue("address", addressValue),
			ydb.StructFieldValue("short_address", shortAddressValue),
			ydb.StructFieldValue("latitude", latitudeValue),
			ydb.StructFieldValue("longitude", longitudeValue),
			ydb.StructFieldValue("archived", ydb.BoolValue(false)),
			ydb.StructFieldValue("created", ydb.TimestampValue(db.CurrentTimestamp().YdbTimestamp())),
		))

		userValues = append(userValues, ydb.StructValue(
			ydb.StructFieldValue("hid", ydb.Uint64Value(tools.Huidify(u.ID))),
			ydb.StructFieldValue("current_household_id", ydb.StringValue([]byte(u.Household.ID))),
		))
	}
	params := table.NewQueryParameters(
		table.ValueParam("$householdValues", ydb.ListValue(householdValues...)),
		table.ValueParam("$userValues", ydb.ListValue(userValues...)),
	)
	if err := db.Write(ctx, query, params); err != nil {
		return xerrors.Errorf("failed to create firse household for users: %w", err)
	}

	return nil
}
