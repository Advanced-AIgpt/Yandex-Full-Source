(function (namespace) {
    'use strict';

    if (typeof namespace.ya === 'undefined') {
        namespace.ya = {};
    }
    if (typeof namespace.ya.speechkit === 'undefined') {
        namespace.ya.speechkit = {};
    }

    var WebAudioRecognition = function (options) {
        if (!(this instanceof namespace.ya.speechkit.WebAudioRecognition)) {
            return new namespace.ya.speechkit.WebAudioRecognition(options);
        }
        this.recognition = null;
        this.recorder = null;
        this.options = namespace.ya.speechkit._extend(
                                namespace.ya.speechkit._extend(
                                    namespace.ya.speechkit._defaultOptions(),
                                    options),
                                {format: namespace.ya.speechkit.FORMAT.PCM44});
    };

    WebAudioRecognition.prototype = {
        _onstart: function () {
            this.send = 0;
            this.send_bytes = 0;

            this.recognition = namespace.ya.speechkit._extend(this.recognition,
                {
                    interim_transcript: '',
                    lang: this.options.lang,
                    onend: this.stop.bind(this),
                    onresult: function (event) {
                        this.interim_transcript = '';
                        var arr = [];
                        for (var i = event.resultIndex; i < event.results.length; ++i) {
                            if (event.results[i].isFinal) {
                                arr.push({0:{
                                    transcript: event.results[i][0].transcript,
                                    confidence: event.results[i][0].confidence
                                }});
                                this.backref.options.dataCallback(event.results[i][0].transcript, true, 1);
                                this.interim_transcript = '';
                            } else {
                                this.interim_transcript += event.results[i][0].transcript;
                            }
                        }
                        if (arr.length) {
                            this.backref.recognizer._sendJson(arr);
                        }
                        this.backref.options.dataCallback(this.interim_transcript, false, 1);
                    },
                    continuous: true,
                    interimResults: true,
                    maxAlternatives: 5,
                    errorCallback: this.options.errorCallback,
                    onerror: function (e) { this.errorCallback(e.error); }
                });
            this.recognition.backref = this;

            this.recorder = new namespace.ya.speechkit.Recorder();
            this.recognizer = new namespace.ya.speechkit.Recognizer(
                namespace.ya.speechkit._extend(this.options,
                {
                    url: this.options.url.replace('asrsocket.ws', 'logsocket.ws'),
                    samplerate: this.options.format.sampleRate,
                    onInit: function (sessionId, code) {
                        this.recorder.start(function (data) {
                            if (this.options.vad && this.vad) {
                                this.vad.update();
                            }
                            this.send++;
                            this.send_bytes += data.byteLength;
                            this.options.infoCallback({
                                send_bytes: this.send_bytes,
                                format: this.options.format,
                                send_packages: this.send,
                                processed: this.proc
                            });
                            this.recognizer.addData(data);
                        }.bind(this), this.options.format);
                        this.recognition.onstart = this.options.initCallback.bind(this, sessionId, code, 'native');
                        this.recognition.start();
                    }.bind(this),
                    onResult: function () {},
                    onError: function (msg) {
                                this.recorder.stop(function () {});
                                this.recognizer.close();
                                this.recognizer = null;
                                this.options.errorCallback(msg);
                            }.bind(this),
                }));
            this.recognizer.start();
        },
        start: function () {
            if (typeof namespace.webkitSpeechRecognition !== 'undefined') {
                this.recognition = new namespace.webkitSpeechRecognition();

                if (namespace.ya.speechkit._stream !== null) {
                    this._onstart();
                } else {
                    namespace.ya.speechkit.initRecorder(
                        this._onstart.bind(this),
                        this.options.errorCallback
                    );
                }
            } else {
                this.options.errorCallback('Your browser doesn\'t implement Web Speech API');
            }
        },
        stop: function (cb) {
            if (this.recognition) {
                this.recognition.onend = function () {};
                this.recognition.stop();
            }
            if (this.recorder) {
                this.recorder.stop();
            }
            if (this.recognizer) {
                this.recognizer.close();
            }
            this.options.stopCallback();
            if (typeof cb !== 'undefined') {
                if (Object.prototype.toString.call(cb) == '[object Function]') {
                    cb();
                }
            }
        },
        pause: function () {
        },
        isPaused: function () {
            return false;
        },
        getAnalyserNode: function () {
            if (this.recorder) {
                return this.recorder.getAnalyserNode();
            }
        }
    };

    namespace.ya.speechkit.WebAudioRecognition = WebAudioRecognition;
}(this));
