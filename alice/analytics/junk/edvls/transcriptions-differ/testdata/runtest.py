#!/usr/bin/python
# -*- coding: utf-8 -*-

import sys, os
import requests
from datetime import datetime
import time
import random

from transport import Transport

from basic_pb2 import ConnectionResponse
from ttsbackend_pb2 import Generate, GenerateResponse
from tts_pb2 import GenerateRequest, ConnectionRequest, ParamsRequest, ParamsResponse 


gWavHeader = ""
def generateWavHeader(sample_rate, mono=True):
    global gWavHeader
    gWavHeader = "RIFF\xff\xff\xff\xffWAVEfmt \x10\x00\x00\x00\x01\x00" + ("\x01" if mono else "\x02") + "\x00"
    wav_rate = ""
    wav_rate_align = ""
    sample_rate_align = sample_rate * 2
    for i in xrange(0, 4):
        wav_rate += chr(sample_rate % (256 if mono else 512))  # sample_rate * block_align (2 for mono) as int32
        wav_rate_align += chr(sample_rate_align % 256)  # sample_rate as int32
        sample_rate /= 256
        sample_rate_align /= 256
    gWavHeader += wav_rate
    gWavHeader += wav_rate_align
    gWavHeader += "\x02" if mono else "\x04"
    gWavHeader += "\x00\x10\x00data\xff\xff\xff\xff"
generateWavHeader(16000)

if len(sys.argv) < 6:
    print "Usage is: %s server port lang test_set_file target_dir [key]" % (sys.argv[0],)
    sys.exit(1)

gServer = sys.argv[1]
gPort = sys.argv[2]
gLang = sys.argv[3]
gFile = open(sys.argv[4], "r")
gDir = sys.argv[5]
if len(sys.argv)>6:
    gKey = sys.argv[6]

def ping():
    maxTries = 30
    
    while True:
        try:
           print >> sys.stderr, "Try #%s" % (31 - maxTries, )
           pingRequest = requests.get("http://%s:%s/ping" % (gServer, gPort))
           if pingRequest.status_code == 200 and pingRequest.text in ["Pong\n", "ready"]:
               print >> sys.stderr, "Ping ok"
               return True
        except Exception as ex:
           print >> sys.stderr, "ping %s" % (ex, )
        maxTries -= 1
        time.sleep(5)
        if (maxTries == 0):
            print >> sys.stderr, "Ping failed"
            break
    return False

def upgradeToProtobuf(transport):
        transport.verbose = False
        transport.send("GET /ytcp2 HTTP/1.1\r\n" +
                "User-Agent:KeepAliveClient\r\n" +
                "Host: %s:%s\r\n" % (gServer, gPort) +
                "Upgrade: websocket\r\n\r\n");
        check = "HTTP/1.1 101"
        checkRecv = ""
        while True:
            checkRecv += transport.recv(1)
            if checkRecv.startswith(check) and checkRecv.endswith("\r\n\r\n"):
                break
            if len(checkRecv) > 300:
                return False
        return True

if not ping():
    sys.exit(1)
        
for num_line, test_line in enumerate(gFile.readlines()):
    if not test_line.strip():
        continue
    pcmfile = open("%s/%s.wav" % (gDir, num_line), "wb")
    pcmfile.write(gWavHeader)
    transfile = open("%s/%s.txt" % (gDir, num_line), "w")
    transfile.write(test_line.strip() + "\n")

    with Transport(gServer, gPort) as t:
        if not upgradeToProtobuf(t):
            print "Wrong response on upgrade request"
            sys.exit(1)
        print "Upgraded to protobuf, sending connect request"
        
        if gKey:
            t.sendProtobuf(ConnectionRequest(
                serviceName="tts",
                speechkitVersion="test",
                uuid='123',
                apiKey=gKey
            ))

            connectionResponse = t.recvProtobuf(ConnectionResponse)
            
            if connectionResponse.responseCode != 200:
                print "Bad response code for test #%s" % (num_line,)
                sys.exit(1)

            t.sendProtobuf(ParamsRequest(
                listVoices=True
            ))

            res = t.recvProtobuf(ParamsResponse)

        t.sendProtobuf(GenerateRequest(
            lang=gLang,
            text=unicode(test_line.strip().decode('utf8')),
            application="test",
            platform="local",
            voice="oksana",
            requireMetainfo=True,
            format=GenerateRequest.Pcm,
            chunked=True
        ))

        total_bytes = 0
        while True:
            ttsResponse = t.recvProtobuf(GenerateResponse)
            
            phones = []
            words = []
            for x in ttsResponse.phonemes:
                phones.append((x.positionInBytesStream / 2.0 / 48000.0, x.ttsPhoneme, x.durationMs * 0.001))

            for x in ttsResponse.words:
                words.append(
                    {
                        'word': x.text,
                        'from': x.bytesLengthInSignal / 2.0 / 48000.0,
                        'postag': x.postag,
                        'homographTag': x.homographTag,
                        'trans': '',
                    }
                )
            if ttsResponse.words:
                words.append({'word': 'END OF UTT', 'from': -1, 'postag': '', 'homographTag': '', 'trans': ''})
                word_i = 0
                trans = [[] for x in words]
                for p in phones:
                    while words[word_i]['from'] < 0 and word_i < len(words) - 1:
                        word_i += 1
                    if words[word_i]['from'] >= 0 and (word_i == len(words) - 1 or words[word_i + 1]['from'] > p[0] + p[2]*0.9):
                        pass
                    else:
                        word_i += 1
                    words[word_i]["trans"] += " " + p[1]
                for w in words:
                    transfile.write("%s (%s, %s) %s\n" % (w['word'].encode('utf8'),
                        w['postag'].encode('utf8'),
                        w['homographTag'].encode('utf8'),
                        w['trans'].encode('utf8'),
                    ))            
            if not ttsResponse.completed:
                pcmfile.write(ttsResponse.audioData)
            else:
                pcmfile.close()
                transfile.close()
                break
    print "Done test #%s" % (num_line,)
