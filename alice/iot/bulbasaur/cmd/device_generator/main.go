package main

import (
	"flag"
	"os"

	"a.yandex-team.ru/alice/iot/bulbasaur/model"
)

type GeneratorConfig struct {
	dbFilePath         *string
	userID             *uint64
	login              *string
	ydbPrefix          *string
	ydbEndpoint        *string
	deviceConfigString *string
}

func (gc *GeneratorConfig) IsFilled() bool {
	return (*gc.userID != 0) && (*gc.login != "") && (*gc.deviceConfigString != "")
}

func main() {

	// uncomment to generate configs in ./device_configs/
	//generateConfigs()
	//return

	logger, stop := initLogging()
	defer stop()

	generatorConfig := GeneratorConfig{
		dbFilePath:  flag.String("db", "./database_configs/db_beta.yaml", "Config in database_configs"),
		userID:      flag.Uint64("user_id", 0, "Field for custom user_id"),
		login:       flag.String("login", "", "Field for custom login"),
		ydbPrefix:   flag.String("ydb_prefix", "/ru-prestable/quasar/beta/iotdb", "Ydb prefix"),
		ydbEndpoint: flag.String("ydb_endpoint", "ydb-ru-prestable.yandex.net:2135", "Ydb endpoint"),
		deviceConfigString: flag.String("device_config", "", "If not empty the device config "+
			"is taken from this string"),
	}

	flag.Parse()

	var (
		databaseAccess DatabaseAccess
		userData       UserData
		devices        []model.Device
	)

	if generatorConfig.IsFilled() {
		databaseAccess = DatabaseAccess{
			Endpoint: *generatorConfig.ydbEndpoint,
			Prefix:   *generatorConfig.ydbPrefix,
		}
		userData = UserData{
			UserID:   *generatorConfig.userID,
			Login:    *generatorConfig.login,
			YdbToken: os.Getenv("YDB_TOKEN"),
		}
		device, err := YAMLConfigToDevice(logger, []byte(*generatorConfig.deviceConfigString))
		if err != nil {
			return
		}
		devices = []model.Device{
			device,
		}
	} else {
		var err error
		// all the error handling is inside the function
		databaseAccess, err = readDBConfig(logger, *generatorConfig.dbFilePath)
		if err != nil {
			return
		}

		userData, err = readUserConfig(logger, *generatorConfig.userID, *generatorConfig.login)
		if err != nil {
			return
		}

		devices, err = readDeviceConfigs(logger)
		if err != nil {
			return
		}
		err = askForApply(logger, databaseAccess, userData)
		if err != nil {
			return
		}
	}

	err := applyDeviceConfig(logger, databaseAccess, userData, devices)
	if err != nil {
		return
	}
}
