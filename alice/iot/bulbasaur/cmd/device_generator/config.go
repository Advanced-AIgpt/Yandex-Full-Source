package main

import (
	"bufio"
	"context"
	"encoding/json"
	"errors"
	"flag"
	"fmt"
	"io/ioutil"
	"os"
	"strconv"
	"strings"

	"gopkg.in/yaml.v2"

	uberzap "go.uber.org/zap"
	"go.uber.org/zap/zapcore"

	"a.yandex-team.ru/alice/iot/bulbasaur/db"
	"a.yandex-team.ru/alice/iot/bulbasaur/model"
	"a.yandex-team.ru/alice/library/go/cli"
	"a.yandex-team.ru/kikimr/public/sdk/go/ydb"
	"a.yandex-team.ru/library/go/core/log/zap"
)

// data about envs - no need to modify. add more if needed
const (
	// endpoints
	stableEndpoint    = "ydb-ru.yandex.net:2135"
	prestableEndpoint = "ydb-ru-prestable.yandex.net:2135"
	devEndpoint       = "ydb-ru-prestable.yandex.net:2135"

	// prefixes
	prodPrefix      = "/ru/quasar/production/iotdb"
	prestablePrefix = "/ru-prestable/quasar/prestable/iotdb"
	betaPrefix      = "/ru-prestable/quasar/beta/iotdb"
	devPrefix       = "/ru-prestable/quasar/development/iotdb"
	mavlyutovPrefix = "/ru-prestable/home/mavlyutov/mydb"
)

func generateDevices() []model.Device {
	return []model.Device{ // note: external name will be shown in UI
		generateConditioner("Кондиционер", memeNameGenerator(), externalIDGenerator(),
			WithUnretrievableOnOff(), WithUnretrievableThermostat(), WithUnretrievableFanSpeed(),
			WithThermostatModes(model.AutoMode, model.HeatMode, model.CoolMode),
			WithFanSpeedModes(model.AutoMode, model.LowMode, model.HighMode),
			WithNoTemperatureRangeOption(),
		),

		generateThermostat("ТермостатТемп", memeNameGenerator(), externalIDGenerator(),
			WithUnretrievableOnOff(), WithUnretrievableThermostat(), WithUnretrievableFanSpeed(), WithUnretrievableTemperatureRange(),
			WithThermostatModes(model.AutoMode, model.HeatMode, model.CoolMode),
			WithFanSpeedModes(model.AutoMode, model.LowMode, model.HighMode),
			WithTemperatureRangeBounds(18, 28),
		),

		generateThermostat("ТермостатБезТемп", memeNameGenerator(), externalIDGenerator(),
			WithUnretrievableOnOff(), WithUnretrievableThermostat(), WithUnretrievableFanSpeed(),
			WithThermostatModes(model.AutoMode, model.HeatMode, model.CoolMode),
			WithFanSpeedModes(model.AutoMode, model.LowMode, model.HighMode),
			WithNoTemperatureRangeOption(),
		),

		generateCustomDevice("Пекарь", "Пекарь", externalIDGenerator(),
			WithCustomButton("включи"),
			WithCustomButton("сделай пюре"),
			WithCustomButton("приготовь кашу"),
		),

		generateSpeaker(model.YandexStation2DeviceType, "feedface-e8a2-4439-b2e7-000000000001"),
		generateSpeaker(model.YandexStationMini2DeviceType, "feedface-e8a2-4439-b2e7-000000000002"),

		generateSpeaker(model.SmartSpeakerDeviceType, externalIDGenerator()),
		generateSpeaker(model.YandexStationDeviceType, externalIDGenerator()),
		generateSpeaker(model.YandexStation2DeviceType, externalIDGenerator()),
		generateSpeaker(model.DexpSmartBoxDeviceType, externalIDGenerator()),
		generateSpeaker(model.IrbisADeviceType, externalIDGenerator()),
		generateSpeaker(model.LGXBoomDeviceType, externalIDGenerator()),
		generateSpeaker(model.ElariSmartBeatDeviceType, externalIDGenerator()),
		generateSpeaker(model.YandexStationMiniDeviceType, externalIDGenerator()),
		generateSpeaker(model.JetSmartMusicDeviceType, externalIDGenerator()),
		generateSpeaker(model.PrestigioSmartMateDeviceType, externalIDGenerator()),
		generateSpeaker(model.DigmaDiHomeDeviceType, externalIDGenerator()),
		generateSpeaker(model.JBLLinkPortableDeviceType, externalIDGenerator()),
		generateSpeaker(model.JBLLinkMusicDeviceType, externalIDGenerator()),
		generateTV("Телевизор ИК 1", memeNameGenerator(), externalIDGenerator(),
			WithUnretrievableOnOff(), WithUnretrievableVolumeRange(), WithUnretrievableChannelRange(),
			WithUnretrievablePauseToggle(), WithUnretrievableMuteToggle(), WithUnretrievableInputSource(),
			WithInputSourceModes(model.OneMode)),
		generateTV("Телевизор ИК 123", memeNameGenerator(), externalIDGenerator(),
			WithUnretrievableOnOff(), WithUnretrievableVolumeRange(), WithUnretrievableChannelRange(),
			WithUnretrievablePauseToggle(), WithUnretrievableMuteToggle(), WithUnretrievableInputSource(),
			WithInputSourceModes(model.OneMode, model.TwoMode, model.ThreeMode)),
		generateTV("Телевизор Умный 1", memeNameGenerator(), externalIDGenerator(),
			WithInputSourceModes(model.OneMode)),
		generateTV("Телевизор Умный 123", memeNameGenerator(), externalIDGenerator(),
			WithInputSourceModes(model.OneMode, model.TwoMode, model.ThreeMode)),
		generateHumidifier("Увлажнитель", memeNameGenerator(), externalIDGenerator()),
		generateLamp("Белая 1", memeNameGenerator(), externalIDGenerator(), WithNoColorModel()),
		generateLamp("Белая 2", memeNameGenerator(), externalIDGenerator(), WithNoColorModel()),
		generateLamp("Цветная 1", memeNameGenerator(), externalIDGenerator()),
		generateLamp("Цветная 2", memeNameGenerator(), externalIDGenerator()),
		generateSocket("Розетка", memeNameGenerator(), externalIDGenerator()),
		generateConditioner("Кондиционер", memeNameGenerator(), externalIDGenerator()),
		generateTV("Телевизор", memeNameGenerator(), externalIDGenerator()),
		generateCoffeeMaker("Кофеварка", memeNameGenerator(), externalIDGenerator()),
		generateKettle("Чайник", memeNameGenerator(), externalIDGenerator()),
		generateVacuumCleaner("Пылесос", memeNameGenerator(), externalIDGenerator()),
		generateWashingMachine("Стиралка", memeNameGenerator(), externalIDGenerator()),
		generateKettle("Чайник", memeNameGenerator(), externalIDGenerator(), WithTeaModes()),
		generatePetFeeder("Кормушка", memeNameGenerator(), externalIDGenerator()),
		generateLamp("Сценическая лампа", memeNameGenerator(), externalIDGenerator()),
		generatePurifier("Очиститель", memeNameGenerator(), externalIDGenerator()),
		generateEventSensor("Ивентовый датчик", memeNameGenerator(), externalIDGenerator()),
	}
}

func generateVoiceQueriesEvoTestConfig() []model.Device {
	return []model.Device{
		generateSpeaker(model.YandexStationMiniDeviceType, externalIDGenerator()),
		generateConditioner("Кондиционер", "Кондиционер", externalIDGenerator(),
			WithThermostatModes(model.AutoMode, model.HeatMode, model.CoolMode),
			WithFanSpeedModes(model.AutoMode, model.LowMode, model.HighMode),
			WithNoTemperatureRangeOption(),
		),
		generateThermostat("Термостат", "Термостат", externalIDGenerator(),
			WithThermostatModes(model.AutoMode, model.HeatMode, model.CoolMode),
			WithFanSpeedModes(model.AutoMode, model.LowMode, model.HighMode),
			WithTemperatureRangeBounds(18, 28),
		),
		generateTV("Телевизор", "Телевизор", externalIDGenerator(),
			WithInputSourceModes(model.OneMode, model.TwoMode, model.ThreeMode),
		),
		generateHumidifier("Увлажнитель", "Увлажнитель", externalIDGenerator()),
		generateLamp("Белая лампа", "Белая лампа", externalIDGenerator(), WithNoColorModel()),
		generateLamp("Люстра", "Люстра", externalIDGenerator()),
		generateSocket("Розетка", "Розетка", externalIDGenerator()),
		generateCoffeeMaker("Кофеварка", "Кофеварка", externalIDGenerator()),
		generateKettle("Чайник", "Чайник", externalIDGenerator()),
		generateVacuumCleaner("Пылесос", "Пылесос", externalIDGenerator()),
		generateWashingMachine("Стиралка", "Стиралка", externalIDGenerator()),
		generateMulticooker("Мультиварка", "Мультиварка", externalIDGenerator()),
		generateCurtain("Штора", "Штора", externalIDGenerator()),
		generatePurifier("Очиститель", "Очиститель", externalIDGenerator()),
		generateDishwasher("Посудомойка", "Посудомойка", externalIDGenerator()),
	}
}

// Getting rid of last_updated field
func recurseFilterLastUpdated(inter interface{}) {
	switch x := inter.(type) {
	case map[string]interface{}:
		delete(x, "last_updated")
		for _, v := range x {
			recurseFilterLastUpdated(v)
		}
	case []interface{}:
		for _, v := range x {
			recurseFilterLastUpdated(v)
		}
	}
}

func filterLastUpdated(jsonBytes []byte) (interface{}, error) {
	var config interface{}
	if err := json.Unmarshal(jsonBytes, &config); err != nil {
		return nil, err
	}
	recurseFilterLastUpdated(config)
	return config, nil
}

func recurseYAMLToJSONConverter(inter interface{}) interface{} {
	switch x := inter.(type) {
	case map[interface{}]interface{}:
		newMap := map[string]interface{}{}
		for k, v := range x {
			newMap[k.(string)] = recurseYAMLToJSONConverter(v)
		}
		return newMap

	case []interface{}:
		for i, v := range x {
			x[i] = recurseYAMLToJSONConverter(v)
		}
		return x

	default:
		return inter
	}
}

func YAMLToJSONConverter(jsonBytes []byte) ([]byte, error) {
	var jsonConfig interface{}
	if err := yaml.Unmarshal(jsonBytes, &jsonConfig); err != nil {
		return nil, err
	}
	jsonConfig = recurseYAMLToJSONConverter(jsonConfig)
	res, err := json.Marshal(jsonConfig)
	if err != nil {
		return nil, err
	}
	return res, nil
}

func initLogging() (logger *zap.Logger, stop func()) {
	encoderConfig := uberzap.NewDevelopmentEncoderConfig()
	encoderConfig.EncodeLevel = zapcore.CapitalColorLevelEncoder
	encoderConfig.EncodeCaller = zapcore.ShortCallerEncoder
	encoder := zapcore.NewConsoleEncoder(encoderConfig)

	core := zapcore.NewCore(encoder, zapcore.AddSync(os.Stderr), uberzap.DebugLevel)
	stop = func() {
		_ = core.Sync()
	}
	logger = zap.NewWithCore(core, uberzap.AddStacktrace(uberzap.FatalLevel), uberzap.AddCaller())

	return
}

type SmallDevice struct {
	Name         string          `json:"name" yaml:"name"`
	SkillID      string          `json:"skill_id" yaml:"skill_id"`
	Type         string          `json:"type" yaml:"type"`
	Capabilities json.RawMessage `json:"capabilities" yaml:"capabilities"`
	Properties   json.RawMessage `json:"properties" yaml:"properties"`
}

type DatabaseAccess struct {
	Endpoint string `json:"endpoint" yaml:"endpoint"`
	Prefix   string `json:"prefix" yaml:"prefix"`
}

type UserData struct {
	UserID   uint64 `json:"user_id" yaml:"user_id"`
	Login    string `json:"login" yaml:"login"`
	YdbToken string `json:"ydb_token" yaml:"ydb_token"`
}

func generateConfigs() {
	allDevices := generateDevices()
	for _, device := range allDevices {
		mcap, _ := json.Marshal(device.Capabilities)
		mpro, _ := json.Marshal(device.Properties)
		curDevice := SmallDevice{
			Name:         device.Name,
			SkillID:      device.SkillID,
			Type:         string(device.Type),
			Capabilities: mcap,
			Properties:   mpro,
		}
		marshalledDevice, _ := json.MarshalIndent(curDevice, "", "  ")
		jsonDeviceInterface, _ := filterLastUpdated(marshalledDevice)
		yamlMarshalledDevice, _ := yaml.Marshal(jsonDeviceInterface)
		fPath := "./device_configs/" + strings.Replace(device.Name, " ", "_", -1) + ".yaml"
		_ = ioutil.WriteFile(fPath, yamlMarshalledDevice, 0666)
	}
}

func readDBConfig(logger *zap.Logger, dbFilePath string) (DatabaseAccess, error) {
	logger.Infof("Opening database config file")
	yamlFile, err := os.Open(dbFilePath)
	if err != nil {
		logger.Warnf("Failed to open database config file: %s", err.Error())
		return DatabaseAccess{}, err
	}

	data, err := ioutil.ReadAll(yamlFile)
	if err != nil {
		logger.Warnf("Failed to read database config file: %s", err.Error())
		return DatabaseAccess{}, err
	}

	var da DatabaseAccess
	err = yaml.Unmarshal(data, &da)
	if err != nil {
		logger.Warnf("Failed to parse YAML from database config file: %s", err.Error())
		return DatabaseAccess{}, err
	}
	logger.Infof("You are using: {prefix: %s, endpoint: %s} ", da.Prefix, da.Endpoint)
	return da, nil
}

func initUserConfig(logger *zap.Logger) error {
	logger.Infof("Couldn't find the config file. Creating new user config file")
	fmt.Print("Enter your user_id: ")
	reader := bufio.NewReader(os.Stdin)
	userID, err := reader.ReadString('\n')
	if err != nil {
		logger.Warnf("Couldn't read from stdin: %s", err.Error())
		return err
	}
	newUserData := UserData{}
	newUserData.UserID, err = strconv.ParseUint(strings.TrimSuffix(userID, "\n"), 10, 64)
	if err != nil {
		logger.Warnf("Couldn't parse uint64: %s", err.Error())
		return err
	}
	fmt.Print("Enter your login: ")
	login, err := reader.ReadString('\n')
	if err != nil {
		logger.Warnf("Couldn't read from stdin: %s", err.Error())
		return err
	}
	newUserData.Login = strings.TrimSuffix(login, "\n")
	newUserData.YdbToken = ""

	marshalledNewUserData, _ := yaml.Marshal(newUserData)
	err = ioutil.WriteFile("./user_config.yaml", marshalledNewUserData, 0666)
	if err != nil {
		logger.Warnf("Error while creating new config file: %s", err.Error())
		return err
	}
	logger.Infof("Successfully created new config file")
	return nil
}

func readUserConfig(logger *zap.Logger, userID uint64, login string) (UserData, error) {
	logger.Infof("Opening user config file")
	yamlFile, err := os.Open("user_config.yaml")

	if err != nil {
		logger.Warnf("Failed to open user config file: %s", err.Error())
		err = initUserConfig(logger)
		if err != nil {
			return UserData{}, err
		}
		yamlFile, err = os.Open("user_config.yaml")
		if err != nil {
			logger.Warnf("Failed to open user config file again: %s, aborting", err.Error())
			return UserData{}, err
		}
	}

	data, err := ioutil.ReadAll(yamlFile)
	if err != nil {
		logger.Warnf("Failed to read user config file: %s", err.Error())
		return UserData{}, err
	}

	var ud UserData
	err = yaml.Unmarshal(data, &ud)
	if err != nil {
		logger.Warnf("Failed to parse YAML from user config file: %s", err.Error())
		return UserData{}, err
	}
	if userID != 0 {
		ud.UserID = userID
	}
	if login != "" {
		ud.Login = login
	}
	if ud.UserID == 0 {
		logger.Warnf("Field user_id in user config is zero, please set it to a valid userid")
		return UserData{}, errors.New("field user_id in user config is zero")
	}
	if ud.Login == "" {
		logger.Warnf("Field login in user config is empty, please set it to a valid login")
		return UserData{}, errors.New("field login in user config is empty")
	}
	if ud.YdbToken == "" {
		ud.YdbToken = os.Getenv("YDB_TOKEN")
	}
	return ud, nil
}

func YAMLConfigToDevice(logger *zap.Logger, data []byte) (model.Device, error) {
	data, err := YAMLToJSONConverter(data)
	if err != nil {
		logger.Warnf("Failed to convert YAML to JSON: %s", err.Error())
		return model.Device{}, err
	}

	smallDevice := SmallDevice{}
	err = json.Unmarshal(data, &smallDevice)
	if err != nil {
		logger.Warnf("Failed to parse JSON from the config file: %s", err.Error())
		return model.Device{}, err
	}
	device := model.Device{
		Name:         smallDevice.Name,
		ExternalID:   externalIDGenerator(),
		ExternalName: smallDevice.Name,
		SkillID:      smallDevice.SkillID,
		Type:         model.DeviceType(smallDevice.Type),
		OriginalType: model.DeviceType(smallDevice.Type),
	}

	device.Capabilities, err = model.JSONUnmarshalCapabilities(smallDevice.Capabilities)
	if err != nil {
		logger.Warnf("Failed to unmarshal capabilities: %s", err.Error())
		return model.Device{}, err
	}
	device.Properties, err = model.JSONUnmarshalProperties(smallDevice.Properties)
	if err != nil {
		logger.Warnf("Failed to unmarshal properties: %s", err.Error())
		return model.Device{}, err
	}

	deviceRaw, _ := json.MarshalIndent(smallDevice, "", "  ")
	logger.Infof("Config of a device that is about to be added:\n%s", string(deviceRaw))
	return device, nil
}

func readDeviceConfigs(logger *zap.Logger) ([]model.Device, error) {
	var pathsToConfigs []string
	devices := make([]model.Device, 0)
	if flag.Arg(0) != "" {
		pathsToConfigs = flag.Args()
	} else {
		logger.Warnf("No paths to the device_generator were provided")
		return nil, errors.New("no unnamed arguments were provided")
	}
	logger.Infof("All paths to config files:\n")
	for _, path := range pathsToConfigs {
		fmt.Println(path)
	}
	for _, path := range pathsToConfigs {
		logger.Infof("Opening " + path + " config file")
		yamlFile, err := os.Open(path)
		if err != nil {
			logger.Warnf("Failed to open device config file: %s", err.Error())
			return nil, err
		}
		data, err := ioutil.ReadAll(yamlFile)
		if err != nil {
			logger.Warnf("Failed to read the config file: %s", err.Error())
			return nil, err
		}
		device, err := YAMLConfigToDevice(logger, data)
		if err != nil {
			return nil, err
		}
		devices = append(devices, device)
	}
	return devices, nil
}

func askForApply(logger *zap.Logger, da DatabaseAccess, ud UserData) error {
	user := model.User{ID: ud.UserID, Login: ud.Login}
	msg := fmt.Sprintf("Do you want to generate devices for {login: %s, uid: %d} at %s:%s", user.Login, user.ID, da.Endpoint, da.Prefix)
	shouldContinue := cli.AskForConfirmation(msg, logger)
	if !shouldContinue {
		logger.Info("Bye")
		return errors.New("generation was aborted")
	}
	return nil
}

func applyDeviceConfig(logger *zap.Logger, da DatabaseAccess, ud UserData, devices []model.Device) error {
	dbcli, err := db.NewClient(context.Background(), logger, da.Endpoint, da.Prefix, ydb.AuthTokenCredentials{AuthToken: ud.YdbToken}, false)
	if err != nil {
		panic(err.Error())
	}

	user := model.User{ID: ud.UserID, Login: ud.Login}

	logger.Infof("Creating new user: {login: %s, uid: %d} at %s:%s", user.Login, user.ID, da.Endpoint, da.Prefix)
	err = dbcli.StoreUser(context.Background(), user)
	if err != nil {
		logger.Warnf("failed to create new user: %s", err.Error())
		return err
	}

	for _, device := range devices {
		logger.Infof("Generating devices for {login: %s, uid: %d} at %s:%s", user.Login, user.ID, da.Endpoint, da.Prefix)
		storedDevice, storeResult, err := dbcli.StoreUserDevice(context.Background(), user, device)
		if err != nil {
			logger.Warnf("Unable to create device {name: %s, type: %s} for userID %d: %s", device.Name, device.Type, user.ID, err)
			return err
		}
		switch storeResult {
		case model.StoreResultNew:
			logger.Infof("Created device {id: %s, name: %s, type: %s} for userID %d", storedDevice.ID, device.Name, device.Type, user.ID)
		case model.StoreResultLimitReached:
			logger.Warnf("Unable to create device {name: %s, type: %s} for userID %d: %s", device.Name, device.Type, user.ID, &model.DeviceLimitReachedError{})
			return &model.DeviceLimitReachedError{}
		default:
			logger.Infof("Overwritten device {id: %s, name: %s, type: %s} for userID %d", storedDevice.ID, device.Name, device.Type, user.ID)
		}
	}
	return nil
}
