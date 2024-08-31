package main

import (
	"fmt"
	"os"
	"path"
	"time"

	"a.yandex-team.ru/alice/library/go/cli"
	"a.yandex-team.ru/library/go/core/log/zap"
	"github.com/go-cmd/cmd"
	flag "github.com/spf13/pflag"
	uberzap "go.uber.org/zap"
	"go.uber.org/zap/zapcore"
)

var gifSubstBinPath string
var inputsDirectoryPath string
var outputsDirectoryPath string

var logger *zap.Logger
var stop func()

func init() {
	logger, stop = initLogging()
	flag.StringVar(&gifSubstBinPath, "gifsubst", "", "path to gifsubst binary")
	flag.StringVarP(&inputsDirectoryPath, "inputs", "i", "inputs", "path to inputs directory")
	flag.StringVarP(&outputsDirectoryPath, "outputs", "o", "outputs", "path to outputs directory")
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

type TemperatureTemplateGifs struct {
	MinusOneDigitTemplate   string
	MinusTwoDigitTemplate   string
	MinusThreeDigitTemplate string
	OneDigitTemplate        string
	PlusOneDigitTemplate    string
	PlusTwoDigitTemplate    string
	PlusThreeDigitTemplate  string
}

func generateTemperatureGifs(gifSubstBinPath string, gifPaths TemperatureTemplateGifs, outputDirectoryPath string) {
	// generate -100..-200 and 100..200
	for i := 100; i <= 200; i++ {
		plusThreeDigitsOutputGifPath := path.Join(outputDirectoryPath, fmt.Sprintf("%d.gif", i))
		logger.Infof("generating %s", plusThreeDigitsOutputGifPath)

		gifSubstPlusThreeDigitsCmd := cmd.NewCmd(gifSubstBinPath,
			"-i", gifPaths.PlusThreeDigitTemplate,
			"-b", fmt.Sprint(i/100), "-r", fmt.Sprint((i/10)%10), "-g", fmt.Sprint(i%10), "-f", "clock_large", "-o", plusThreeDigitsOutputGifPath,
		)
		plusThreeDigitsStatusCh := gifSubstPlusThreeDigitsCmd.Start()
		select {
		case finalStatus := <-plusThreeDigitsStatusCh:
			if err := finalStatus.Error; err != nil {
				logger.Infof("gifSubst got error while generating %s: %+v\nStdout: %+v\nStderr: %+v", plusThreeDigitsOutputGifPath, err, finalStatus.Stdout, finalStatus.Stderr)
			}
		case <-time.After(time.Second * 5):
			logger.Infof("gifSubst timeout while generating %s", plusThreeDigitsOutputGifPath)
		}

		minusThreeDigitsOutputGifPath := path.Join(outputDirectoryPath, fmt.Sprintf("-%d.gif", i))
		logger.Infof("generating %s", minusThreeDigitsOutputGifPath)

		gifSubstMinusThreeDigitsCmd := cmd.NewCmd(gifSubstBinPath,
			"-i", gifPaths.MinusThreeDigitTemplate,
			"-b", fmt.Sprint(i/100), "-r", fmt.Sprint((i/10)%10), "-g", fmt.Sprint(i%10), "-f", "clock_large", "-o", minusThreeDigitsOutputGifPath,
		)
		minusThreeDigitsStatusCh := gifSubstMinusThreeDigitsCmd.Start()
		select {
		case finalStatus := <-minusThreeDigitsStatusCh:
			if err := finalStatus.Error; err != nil {
				logger.Infof("gifSubst got error while generating %s: %+v\nStdout: %+v\nStderr: %+v", minusThreeDigitsOutputGifPath, err, finalStatus.Stdout, finalStatus.Stderr)
			}
		case <-time.After(time.Second * 5):
			logger.Infof("gifSubst timeout while generating %s", minusThreeDigitsOutputGifPath)
		}
	}
	// generate -10..-99 and 10..99
	for i := 10; i <= 99; i++ {
		plusTwoDigitsOutputGifPath := path.Join(outputDirectoryPath, fmt.Sprintf("%d.gif", i))
		logger.Infof("generating %s", plusTwoDigitsOutputGifPath)

		gifSubstPlusTwoDigitsCmd := cmd.NewCmd(gifSubstBinPath,
			"-i", gifPaths.PlusTwoDigitTemplate,
			"-b", fmt.Sprint(i/10), "-r", fmt.Sprint(i%10), "-f", "clock_large", "-o", plusTwoDigitsOutputGifPath,
		)
		plusTwoDigitsStatusCh := gifSubstPlusTwoDigitsCmd.Start()
		select {
		case finalStatus := <-plusTwoDigitsStatusCh:
			if err := finalStatus.Error; err != nil {
				logger.Infof("gifSubst got error while generating %s: %+v\nStdout: %+v\nStderr: %+v", plusTwoDigitsOutputGifPath, err, finalStatus.Stdout, finalStatus.Stderr)
			}
		case <-time.After(time.Second * 5):
			logger.Infof("gifSubst timeout while generating %s", plusTwoDigitsOutputGifPath)
		}

		minusTwoDigitsOutputGifPath := path.Join(outputDirectoryPath, fmt.Sprintf("-%d.gif", i))
		logger.Infof("generating %s", minusTwoDigitsOutputGifPath)

		gifSubstMinusTwoDigitsCmd := cmd.NewCmd(gifSubstBinPath,
			"-i", gifPaths.MinusTwoDigitTemplate,
			"-b", fmt.Sprint(i/10), "-r", fmt.Sprint(i%10), "-f", "clock_large", "-o", minusTwoDigitsOutputGifPath,
		)
		minusTwoDigitsStatusCh := gifSubstMinusTwoDigitsCmd.Start()
		select {
		case finalStatus := <-minusTwoDigitsStatusCh:
			if err := finalStatus.Error; err != nil {
				logger.Infof("gifSubst got error while generating %s: %+v\nStdout: %+v\nStderr: %+v", minusTwoDigitsOutputGifPath, err, finalStatus.Stdout, finalStatus.Stderr)
			}
		case <-time.After(time.Second * 5):
			logger.Infof("gifSubst timeout while generating %s", minusTwoDigitsOutputGifPath)
		}
	}
	// generate -1..-9
	for i := 1; i <= 9; i++ {
		plusOneDigitsOutputGifPath := path.Join(outputDirectoryPath, fmt.Sprintf("%d.gif", i))
		logger.Infof("generating %s", plusOneDigitsOutputGifPath)

		gifSubstPlusOneDigitsCmd := cmd.NewCmd(gifSubstBinPath,
			"-i", gifPaths.PlusOneDigitTemplate,
			"-b", fmt.Sprint(i), "-f", "clock_large", "-o", plusOneDigitsOutputGifPath,
		)
		plusOneDigitsStatusCh := gifSubstPlusOneDigitsCmd.Start()
		select {
		case finalStatus := <-plusOneDigitsStatusCh:
			if err := finalStatus.Error; err != nil {

				logger.Infof("gifSubst got error while generating %s: %+v\nStdout: %+v\nStderr: %+v", plusOneDigitsOutputGifPath, err, finalStatus.Stdout, finalStatus.Stderr)
			}
		case <-time.After(time.Second * 5):
			logger.Infof("gifSubst timeout while generating %s", plusOneDigitsOutputGifPath)
		}

		minusOneDigitsOutputGifPath := path.Join(outputDirectoryPath, fmt.Sprintf("-%d.gif", i))
		logger.Infof("generating %s", minusOneDigitsOutputGifPath)

		gifSubstMinusOneDigitsCmd := cmd.NewCmd(gifSubstBinPath,
			"-i", gifPaths.MinusOneDigitTemplate,
			"-b", fmt.Sprint(i), "-f", "clock_large", "-o", minusOneDigitsOutputGifPath,
		)
		minusOneDigitsStatusCh := gifSubstMinusOneDigitsCmd.Start()
		select {
		case finalStatus := <-minusOneDigitsStatusCh:
			if err := finalStatus.Error; err != nil {
				logger.Infof("gifSubst got error while generating %s: %+v", minusOneDigitsOutputGifPath, err)
			}
		case <-time.After(time.Second * 5):
			logger.Infof("gifSubst timeout while generating %s", minusOneDigitsOutputGifPath)
		}
	}
	// generate 0
	zeroOutputGifPath := path.Join(outputDirectoryPath, "0.gif")
	logger.Infof("generating %s", zeroOutputGifPath)

	gifSubstZeroCmd := cmd.NewCmd(gifSubstBinPath,
		"-i", gifPaths.OneDigitTemplate,
		"-b", "0", "-f", "clock_large", "-o", zeroOutputGifPath,
	)
	zeroStatusCh := gifSubstZeroCmd.Start()
	select {
	case finalStatus := <-zeroStatusCh:
		if err := finalStatus.Error; err != nil {
			logger.Infof("gifSubst got error while generating %s: %+v", zeroOutputGifPath, err)
		}
	case <-time.After(time.Second * 5):
		logger.Infof("gifSubst timeout while generating %s", zeroOutputGifPath)
	}
}

type PercentageTemplateGifs struct {
	OneDigitTemplate   string
	TwoDigitTemplate   string
	ThreeDigitTemplate string
}

func generatePercentGifs(gifSubstBinPath string, gifPaths PercentageTemplateGifs, outputDirectoryPath string) {
	// generate 100
	hundredOutputGifPath := path.Join(outputDirectoryPath, fmt.Sprintf("%d.gif", 100))
	logger.Infof("generating %s", hundredOutputGifPath)

	gifSubstHundredCmd := cmd.NewCmd(gifSubstBinPath,
		"-i", gifPaths.ThreeDigitTemplate,
		"-b", "1", "-r", "0", "-g", "0", "-f", "clock_large", "-o", hundredOutputGifPath,
	)
	hundredStatusCh := gifSubstHundredCmd.Start()
	select {
	case finalStatus := <-hundredStatusCh:
		if err := finalStatus.Error; err != nil {
			logger.Infof("gifSubst got error while generating %s: %+v\nStdout: %+v\nStderr: %+v", hundredOutputGifPath, err, finalStatus.Stdout, finalStatus.Stderr)
		}
	case <-time.After(time.Second * 5):
		logger.Infof("gifSubst timeout while generating %s", hundredOutputGifPath)
	}

	// generate 10..99
	for i := 10; i <= 99; i++ {
		twoDigitsOutputGifPath := path.Join(outputDirectoryPath, fmt.Sprintf("%d.gif", i))
		logger.Infof("generating %s", twoDigitsOutputGifPath)

		gifSubstTwoDigitsCmd := cmd.NewCmd(gifSubstBinPath,
			"-i", gifPaths.TwoDigitTemplate,
			"-b", fmt.Sprint(i/10), "-r", fmt.Sprint(i%10), "-f", "clock_large", "-o", twoDigitsOutputGifPath,
		)
		twoDigitsStatusCh := gifSubstTwoDigitsCmd.Start()
		select {
		case finalStatus := <-twoDigitsStatusCh:
			if err := finalStatus.Error; err != nil {
				logger.Infof("gifSubst got error while generating %s: %+v\nStdout: %+v\nStderr: %+v", twoDigitsOutputGifPath, err, finalStatus.Stdout, finalStatus.Stderr)
			}
		case <-time.After(time.Second * 5):
			logger.Infof("gifSubst timeout while generating %s", twoDigitsOutputGifPath)
		}
	}
	// generate 0..9
	for i := 0; i <= 9; i++ {
		oneDigitOutputGifPath := path.Join(outputDirectoryPath, fmt.Sprintf("%d.gif", i))
		logger.Infof("generating %s", oneDigitOutputGifPath)

		gifSubstOneDigitCmd := cmd.NewCmd(gifSubstBinPath,
			"-i", gifPaths.OneDigitTemplate,
			"-b", fmt.Sprint(i), "-f", "clock_large", "-o", oneDigitOutputGifPath,
		)
		oneDigitStatusCh := gifSubstOneDigitCmd.Start()
		select {
		case finalStatus := <-oneDigitStatusCh:
			if err := finalStatus.Error; err != nil {
				logger.Infof("gifSubst got error while generating %s: %+v\nStdout: %+v\nStderr: %+v", oneDigitOutputGifPath, err, finalStatus.Stdout, finalStatus.Stderr)
			}
		case <-time.After(time.Second * 5):
			logger.Infof("gifSubst timeout while generating %s", oneDigitOutputGifPath)
		}
	}
}

func createDirectory(path string, mode os.FileMode) error {
	switch fileInfo, err := os.Stat(path); {
	case os.IsNotExist(err):
		if err := os.Mkdir(path, mode); err != nil {
			return err
		}
	case !fileInfo.IsDir():
		if err := os.Mkdir(path, mode); err != nil {
			return err
		}
	}
	return nil
}

func main() {
	logger, stop = initLogging()
	defer stop()

	flag.Parse()
	if gifSubstBinPath == "" {
		logger.Fatal("gifSubstBinPath can't be empty")
	}

	msg := fmt.Sprintf("Would you like to generate gifs using %s from inputs at %s and save them to %s?", gifSubstBinPath, inputsDirectoryPath, outputsDirectoryPath)
	cli.AskForConfirmation(msg, logger)
	if err := createDirectory(outputsDirectoryPath, 0777); err != nil {
		panic(fmt.Sprintf("can't create directory %s: %v", outputsDirectoryPath, err))
	}
	percentsDirectoryPath := path.Join(outputsDirectoryPath, "percents")
	temperatureDirectoryPath := path.Join(outputsDirectoryPath, "temperature")
	if err := createDirectory(percentsDirectoryPath, 0777); err != nil {
		panic(fmt.Sprintf("can't create directory %s: %v", percentsDirectoryPath, err))
	}
	if err := createDirectory(temperatureDirectoryPath, 0777); err != nil {
		panic(fmt.Sprintf("can't create directory %s: %v", temperatureDirectoryPath, err))
	}
	generatePercentGifs(gifSubstBinPath, PercentageTemplateGifs{
		OneDigitTemplate:   path.Join(inputsDirectoryPath, "percents_templates/one_digit.gif"),
		TwoDigitTemplate:   path.Join(inputsDirectoryPath, "percents_templates/two_digits.gif"),
		ThreeDigitTemplate: path.Join(inputsDirectoryPath, "percents_templates/three_digits.gif"),
	}, percentsDirectoryPath)
	generateTemperatureGifs(gifSubstBinPath, TemperatureTemplateGifs{
		MinusOneDigitTemplate:   path.Join(inputsDirectoryPath, "temperature_templates/minus_one_digit_3.gif"),
		MinusTwoDigitTemplate:   path.Join(inputsDirectoryPath, "temperature_templates/minus_two_digits_3.gif"),
		MinusThreeDigitTemplate: path.Join(inputsDirectoryPath, "temperature_templates/minus_three_digits_3.gif"),
		OneDigitTemplate:        path.Join(inputsDirectoryPath, "temperature_templates/one_digit_3.gif"),
		PlusOneDigitTemplate:    path.Join(inputsDirectoryPath, "temperature_templates/plus_one_digit_3.gif"),
		PlusTwoDigitTemplate:    path.Join(inputsDirectoryPath, "temperature_templates/plus_two_digits_3.gif"),
		PlusThreeDigitTemplate:  path.Join(inputsDirectoryPath, "temperature_templates/plus_three_digits_3.gif"),
	}, temperatureDirectoryPath)
}
