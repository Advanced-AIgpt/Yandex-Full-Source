package main

import (
	"context"
	"encoding/base64"
	"flag"
	"fmt"
	"io/ioutil"
	"os"

	"github.com/golang/protobuf/proto"
	uberzap "go.uber.org/zap"
	"go.uber.org/zap/zapcore"
	"golang.org/x/xerrors"
	"google.golang.org/protobuf/encoding/protojson"

	"a.yandex-team.ru/alice/iot/bulbasaur/db"
	"a.yandex-team.ru/alice/iot/bulbasaur/model"
	"a.yandex-team.ru/kikimr/public/sdk/go/ydb"
	"a.yandex-team.ru/library/go/core/log/zap"
)

// WizExtra generator gets user info from the specified iot database)
// and makes wizextra string for iot_user_info begemot parameter:

type Arguments struct {
	YDBEndpoint string
	YDBPrefix   string
	YDBToken    string
	PathToJSON  string

	UserID uint64

	PrintJSON bool
	Help      bool
}

func (args *Arguments) Parse() {
	flag.Usage = func() {
		fmt.Print(
			"Welcome to my humble user info wizextra generator!\n",
			"It comes in handy if you need to pass some iot config in a begemot request.\n",
			"The output of this script in the form of 'iot_user_info=<long_base64_string>' can be used as wizextra parameter in begemot requests.\n\n",
			"It is particularly useful when it comes to granet forms debugging.\n",
			"Just take the output of the script and pass it to debug_form.sh:\n",
			"./debug_form.sh my_form 'включи свет' --wizextra 'iot_user_info=...'\n\n",
		)
		fmt.Println("The script takes the following parameters:")
		flag.PrintDefaults()
	}
	flag.StringVar(&args.YDBEndpoint, "ydb-endpoint", "", fmt.Sprintf("YDB endpoint. %q by default.", DefaultYDBEndpoint))
	flag.StringVar(&args.YDBPrefix, "ydb-prefix", "", fmt.Sprintf("YDB prefix. %q by default.", DefaultYDBPrefix))
	flag.StringVar(&args.YDBToken, "ydb-token", "", "YDB token.")
	flag.StringVar(&args.PathToJSON, "from-json-file", "", "Path to file with user info json. YDB parameters are not required if this option is used.")
	flag.Uint64Var(&args.UserID, "user-id", 0, "IOT user ID.")
	flag.BoolVar(&args.PrintJSON, "print-json", false, "Just print user info json.")
	flag.Parse()
}

var (
	args Arguments
)

var (
	DefaultYDBEndpoint = "ydb-ru-prestable.yandex.net:2135"
	DefaultYDBPrefix   = "/ru-prestable/quasar/beta/iotdb"
)

type YDBParameters struct {
	Endpoint string
	Prefix   string
	Token    string
}

// Fill tries to get ydb parameters from different sources in the following order:
// cli arguments -> environment variables -> default values
func (parameters *YDBParameters) Fill(arguments Arguments) {
	parameters.PopulateFromArguments(arguments)
	parameters.PopulateFromEnv()
	parameters.PopulateWithDefaults()
}

func (parameters *YDBParameters) PopulateFromArguments(arguments Arguments) {
	if arguments.YDBEndpoint != "" {
		copyIfEmpty(&parameters.Endpoint, arguments.YDBEndpoint)
	}
	if arguments.YDBPrefix != "" {
		copyIfEmpty(&parameters.Prefix, arguments.YDBPrefix)
	}
	if parameters.Token == "" && arguments.YDBToken != "" {
		copyIfEmpty(&parameters.Token, arguments.YDBToken)
	}
}

func (parameters *YDBParameters) PopulateFromEnv() {
	copyIfEmpty(&parameters.Endpoint, os.Getenv("YDB_ENDPOINT"))
	copyIfEmpty(&parameters.Prefix, os.Getenv("YDB_PREFIX"))
	copyIfEmpty(&parameters.Token, os.Getenv("YDB_TOKEN"))
}

func (parameters *YDBParameters) PopulateWithDefaults() {
	copyIfEmpty(&parameters.Endpoint, DefaultYDBEndpoint)
	copyIfEmpty(&parameters.Prefix, DefaultYDBPrefix)
}

func (parameters *YDBParameters) Validate() error {
	if parameters.Token == "" {
		return xerrors.New("ydb token is not specified")
	}
	if parameters.Endpoint == "" {
		return xerrors.New("ydb endpoint is not specified")
	}
	if parameters.Prefix == "" {
		return xerrors.New("ydb prefix is not specified")
	}
	return nil
}

func init() {
	args.Parse()
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

func main() {
	logger, stopLogging := initLogging()
	defer stopLogging()

	var iotUserInfoWizExtra string
	var err error

	if args.PathToJSON != "" {
		if iotUserInfoWizExtra, err = getWizExtraFromJSONFile(); err != nil {
			logger.Errorf("failed to make wizextra from JSON file: %v", err)
			return
		}
	} else {
		if iotUserInfoWizExtra, err = getWizExtraFromDatabase(logger); err != nil {
			logger.Errorf("failed to make wizextra from database: %v", err)
			return
		}
	}

	logger.Info("success!")
	if args.PathToJSON != "" && args.PrintJSON {
		logger.Info("consider using cat next time")
	}
	fmt.Print(iotUserInfoWizExtra)
}

func getWizExtraFromJSONFile() (string, error) {
	ctx := context.Background()
	jsonFile, err := os.Open(args.PathToJSON)
	if err != nil {
		return "", xerrors.Errorf("failed to open %q: %w", args.PathToJSON, err)
	}

	rawJSON, err := ioutil.ReadAll(jsonFile)
	if err != nil {
		return "", xerrors.Errorf("failed to read from %q: %w", args.PathToJSON, err)
	}

	if args.PrintJSON {
		return string(rawJSON), nil
	}

	userInfoProto := model.UserInfo{}.ToUserInfoProto(ctx)
	if err := protojson.Unmarshal(rawJSON, userInfoProto); err != nil {
		return "", xerrors.Errorf("failed to unmarshal json to userInfoProto: %w", err)
	}

	userInfoProtoBytes, err := proto.Marshal(userInfoProto)
	if err != nil {
		return "", xerrors.Errorf("failed to marshal iot user info: %w", err)
	}

	userInfoBase64 := base64.URLEncoding.EncodeToString(userInfoProtoBytes)
	return fmt.Sprintf("iot_user_info=%s", userInfoBase64), nil
}

func getWizExtraFromDatabase(logger *zap.Logger) (string, error) {
	ydbParameters := YDBParameters{}
	ydbParameters.Fill(args)

	if err := ydbParameters.Validate(); err != nil {
		return "", xerrors.Errorf("failed to validate ydb parameters: %w", err)
	}

	if args.UserID == 0 {
		return "", xerrors.New("user-id parameter is not specified")
	}

	ctx := context.Background()
	dbClient, err := db.NewClient(
		ctx,
		logger,
		ydbParameters.Endpoint,
		ydbParameters.Prefix,
		ydb.AuthTokenCredentials{AuthToken: ydbParameters.Token},
		false,
	)

	if err != nil {
		return "", xerrors.Errorf("failed to create ydb client: %w", err)
	}

	userInfo, err := dbClient.SelectUserInfo(ctx, args.UserID)
	if err != nil {
		return "", xerrors.Errorf("failed to select user %d: %w", args.UserID, err)
	}

	marshalledUserInfo, err := marshalUserInfo(ctx, userInfo, args.PrintJSON)
	if err != nil {
		return "", xerrors.Errorf("failed to make user info string: %w", err)
	}

	return marshalledUserInfo, nil
}

func marshalUserInfo(ctx context.Context, userInfo model.UserInfo, printJSON bool) (string, error) {
	userInfoProto := userInfo.ToUserInfoProto(ctx)
	userInfoProtoBytes, err := proto.Marshal(userInfoProto)
	if err != nil {
		return "", xerrors.Errorf("failed to marshal iot user info: %w", err)
	}

	if printJSON {
		marshalledUserInfo, err := protojson.Marshal(userInfoProto)
		if err != nil {
			return "", xerrors.Errorf("failed to marshal userInfo: %w", err)
		}
		return string(marshalledUserInfo), nil
	}

	userInfoBase64 := base64.URLEncoding.EncodeToString(userInfoProtoBytes)
	return fmt.Sprintf("iot_user_info=%s", userInfoBase64), nil
}

func copyIfEmpty(dst *string, src string) {
	if dst != nil && *dst == "" {
		*dst = src
	}
}
