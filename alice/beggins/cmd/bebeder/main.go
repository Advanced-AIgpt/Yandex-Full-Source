package main

import (
	"context"
	"flag"
	"strconv"
	"sync"
	"time"

	"github.com/go-resty/resty/v2"
	"go.uber.org/zap"

	"a.yandex-team.ru/alice/beggins/pkg/begemot"
	"a.yandex-team.ru/yt/go/schema"
	"a.yandex-team.ru/yt/go/ypath"
	"a.yandex-team.ru/yt/go/yt"
	"a.yandex-team.ru/yt/go/yt/ythttp"
)

var logger *zap.SugaredLogger

type (
	Params struct {
		Proxy       string
		InputTable  string
		OutputTable string
		Hosts       []string
		MaxRPS      int
	}
	InputData struct {
		Text string `yson:"text"`
	}
	OutputData struct {
		Text              string    `yson:"text"`
		NormalizedText    string    `yson:"normalized_text"`
		SentenceEmbedding []float32 `yson:"sentence_embedding"`
	}
	Processor struct {
		RequestTimes  []time.Time
		RequestNumber int
		Client        begemot.Client
	}
)

func parseParams() (p Params) {
	flag.StringVar(&p.Proxy, "yt-proxy", "hahn", "")
	flag.StringVar(&p.InputTable, "input-table", "", "")
	flag.StringVar(&p.OutputTable, "output-table", "", "")
	flag.IntVar(&p.MaxRPS, "rps", 10, "")
	flag.Parse()
	p.Hosts = flag.Args()
	logger.Infof("Using config: %#v", p)
	p.Validate()
	return
}

func (p *Processor) Next() {
	delta := time.Since(p.RequestTimes[p.RequestNumber])
	time.Sleep(time.Second - delta)
	p.RequestTimes[p.RequestNumber] = time.Now()
	p.RequestNumber = (p.RequestNumber + 1) % len(p.RequestTimes)
}

func newProcessor(host string, maxRPS int, client *resty.Client) *Processor {
	return &Processor{
		RequestTimes:  make([]time.Time, maxRPS),
		RequestNumber: 0,
		Client: begemot.NewClient(client, begemot.ClientParams{
			Host:        host,
			Rules:       []string{"AliceRequest", "AliceNormalizer", "AliceBegginsEmbedder"},
			ExtraParams: []string{"alice_preprocessing=true"},
			DebugLevel:  2,
		}),
	}
}

func readInputData(yc yt.Client, inputTable string, inputDataChannel chan<- InputData) {
	defer close(inputDataChannel)
	reader, err := yc.ReadTable(context.Background(), ypath.Path(inputTable), nil)
	if err != nil {
		logger.Panic(err)
	}
	defer func() {
		_ = reader.Close()
	}()
	for reader.Next() {
		var inputData InputData
		err = reader.Scan(&inputData)
		if err != nil {
			logger.Errorf("unable to read input row: %v", err)
			continue
		}
		inputDataChannel <- inputData
	}
}

func writeOutputData(yc yt.Client, outputTable string, outputDataChannel <-chan OutputData, group *sync.WaitGroup) {
	defer group.Done()
	tablePath := ypath.Path(outputTable)
	tableSchema := getOutputTableSchema()
	_, err := yt.CreateTable(context.Background(), yc, tablePath, yt.WithSchema(tableSchema), yt.WithForce())
	if err != nil {
		logger.Panic(err)
	}
	writer, err := yc.WriteTable(context.Background(), ypath.Path(outputTable), nil)
	if err != nil {
		logger.Panic(err)
	}

	i := 0
	mod := 1
	for outputData := range outputDataChannel {
		if err = writer.Write(outputData); err != nil {
			logger.Panic(err)
		}
		i++
		if i%mod == 0 {
			logger.Infof("processed %d items", i)
			mod *= 10
		}
	}

	if err = writer.Commit(); err != nil {
		logger.Panic(err)
	}

	logger.Infof("total items: %d", i)
}

func getOutputTableSchema() schema.Schema {
	tableSchema, err := schema.Infer(OutputData{})
	if err != nil {
		logger.Panic(err)
	}
	for i := 0; i < len(tableSchema.Columns); i++ {
		column := &tableSchema.Columns[i]
		if column.Name == "sentence_embedding" {
			column.ComplexType = schema.List{
				Item: schema.TypeFloat64,
			}
		}
	}
	return tableSchema
}

func processInputData(processor *Processor, inputDataChannel <-chan InputData, outputDataChannel chan<- OutputData, wg *sync.WaitGroup) {
	defer wg.Done()
	for inputData := range inputDataChannel {
		processor.Next()
		r, err := processor.Client.Query(context.Background(), inputData.Text)
		if err != nil {
			logger.Errorf("unable to process input: text=%v err=%v", inputData, err)
			continue
		}
		outputDataChannel <- OutputData{
			Text:              inputData.Text,
			NormalizedText:    r.Rules.AliceBegginsEmbedderInput.Query,
			SentenceEmbedding: parseEmbedding(r.Rules.AliceBegginsEmbedder.SentenceEmbedding),
		}
	}
}

func parseEmbedding(embedding []string) (res []float32) {
	res = make([]float32, 0, len(embedding))
	for _, value := range embedding {
		v, _ := strconv.ParseFloat(value, 32)
		res = append(res, float32(v))
	}
	return
}

func run(params Params) {
	yc, err := ythttp.NewClient(&yt.Config{
		Proxy:             params.Proxy,
		ReadTokenFromFile: true,
	})
	if err != nil {
		logger.Panic(err)
	}
	rc := resty.New().SetRetryCount(3).SetTimeout(time.Second)

	inputDataChannel := make(chan InputData, params.MaxRPS*len(params.Hosts)*2)
	outputDataChannel := make(chan OutputData, params.MaxRPS*len(params.Hosts)*2)

	go readInputData(yc, params.InputTable, inputDataChannel)
	processorGroup := sync.WaitGroup{}
	for _, host := range params.Hosts {
		processorGroup.Add(1)
		go processInputData(newProcessor(host, params.MaxRPS, rc), inputDataChannel, outputDataChannel, &processorGroup)
	}
	writerGroup := sync.WaitGroup{}
	writerGroup.Add(1)
	go writeOutputData(yc, params.OutputTable, outputDataChannel, &writerGroup)
	processorGroup.Wait()
	close(outputDataChannel)
	writerGroup.Wait()
}

func (params Params) Validate() {
	if params.MaxRPS <= 0 {
		logger.Panic("MaxRPS must be greater than 0")
	}
	if len(params.Hosts) == 0 {
		logger.Panic("At least one host must be provided")
	}
	if len(params.InputTable) == 0 {
		logger.Panic("InputTable must be provided")
	}
	if len(params.OutputTable) == 0 {
		logger.Panic("OutputTable must be provided")
	}
	if len(params.Proxy) == 0 {
		logger.Panic("YtProxy must be provided")
	}
}

func main() {
	zapLogger, err := zap.NewDevelopment()
	if err != nil {
		panic(err)
	}
	logger = zapLogger.Sugar()
	defer func() {
		_ = logger.Sync()
	}()

	run(parseParams())
}
