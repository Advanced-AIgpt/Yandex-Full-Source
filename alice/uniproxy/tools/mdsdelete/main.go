package main

import (
	"bufio"
	"flag"
	"fmt"
	"log"
	"net/http"
	"os"
	"time"
)

type Result int8

const (
	Ok Result = iota
	NotFound
	ReqErr
	RespErr
)

func delete(client *http.Client, key string) (Result, string) {
	url := fmt.Sprintf("http://storage-int.mds.yandex.net:1111/delete-speechbase/%s", key)
	req, err := http.NewRequest("GET", url, nil)
	if err != nil {
		log.Printf("delete error %s -> %s", key, err)
		return ReqErr, ""
	} else {
		resp, err := client.Do(req)
		if err != nil {
			return RespErr, ""
		}
		defer resp.Body.Close()
		if resp.StatusCode == 200 {
			return Ok, ""
		} else if resp.StatusCode == 404 {
			return NotFound, ""
		} else {
			return RespErr, fmt.Sprintf("%s STATUS %d -> %s", key, resp.StatusCode, resp.Status)
		}
	}
}

func deleteWithRetries(client *http.Client, key string, retries int) Result {
	res := Ok
	logMessage := ""
	for i := 0; i < retries; i++ {
		if i != 0 {
			time.Sleep(time.Second)
		}
		res, logMessage = delete(client, key)
		if res == Ok || res == NotFound {
			if logMessage != "" {
				log.Printf("%s", logMessage)
			}
			return res
		}
	}
	if logMessage != "" {
		log.Printf("%s", logMessage)
	}
	return res
}

func deleter(keys chan string, write chan string, end chan int, retries int) {
	client := &http.Client{}

	counter := 0
	reqErrors := 0
	respErrors := 0
	success := 0
	notFound := 0
	for {
		k := <-keys
		if k == "" {
			break
		}
		res := deleteWithRetries(client, k, retries)
		switch res {
		case Ok:
			success += 1
		case NotFound:
			notFound += 1
		case ReqErr:
			reqErrors += 1
			write <- k
		case RespErr:
			respErrors += 1
			write <- k
		}
		counter += 1
		if counter%1000 == 0 {
			log.Printf(
				"total %d succ %d NotFound %d req err %d delete errors %d",
				counter, success, notFound, reqErrors, respErrors,
			)
		}
	}
	log.Printf(
		"end of work, total %d succ %d NotFound %d req err %d delete errors %d",
		counter, success, notFound, reqErrors, respErrors,
	)
	end <- 1
}

func writer(file *os.File, write chan string, end chan int) {
	writeBuf := bufio.NewWriter(file)
	for {
		k := <-write
		if k == "" {
			break
		}
		k += "\n"
		nbytes, err := writeBuf.WriteString(k)
		if err != nil || nbytes != len(k) {
			panic("Can't write key to file")
		}
	}
	err := writeBuf.Flush()
	if err != nil {
		panic("Can't flush buffer")
	}
	end <- 1
}

func dummyWriter(write chan string, end chan int) {
	for {
		k := <-write
		if k == "" {
			break
		}
	}
	end <- 1
}

func main() {
	var filename = flag.String("file", "keys.txt", "mds keys file")
	var fileWithErrors = flag.String("output", "", "output file to save not deleted keys")
	var jobs = flag.Int("jobs", 128, "parallel jobs count")
	var retries = flag.Int("retries", 5, "count of retries in case of errors")

	flag.Parse()

	file, err := os.Open(*filename)
	if err != nil {
		log.Fatalf("failed open %s", err)
	}
	scanner := bufio.NewScanner(file)
	scanner.Split(bufio.ScanLines)
	keys := make(chan string)
	write := make(chan string)
	end := make(chan int)
	for i := 0; i < *jobs; i++ {
		go deleter(keys, write, end, *retries)
	}
	var outputFile *os.File
	if *fileWithErrors != "" {
		outputFile, err = os.Create(*fileWithErrors)
		if err != nil {
			log.Fatalf("failed create %s", err)
		}
		defer outputFile.Close()
		go writer(outputFile, write, end)
	} else {
		go dummyWriter(write, end)
	}
	scannedLines := 0
	for scanner.Scan() {
		x := scanner.Text()
		if x == "" {
			continue
		}
		keys <- scanner.Text()
		scannedLines += 1
	}

	for i := 0; i < *jobs; i++ {
		keys <- ""
	}
	for i := 0; i < *jobs; i++ {
		<-end
	}
	write <- ""
	<-end
	log.Printf("total lines was scanned %d", scannedLines)
}
