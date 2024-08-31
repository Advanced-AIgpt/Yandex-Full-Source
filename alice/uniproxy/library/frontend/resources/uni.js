(function (namespace) {
    'use strict';

    if (typeof namespace.ya === 'undefined') {
        namespace.ya = {};
    }
    if (typeof namespace.ya.speechkit === 'undefined') {
        namespace.ya.speechkit = {};
    }

    var UniProto = function (options) {
        if (!(this instanceof namespace.ya.speechkit.UniProto)) {
            return new namespace.ya.speechkit.UniProto(options);
        }
        this.options = namespace.ya.speechkit._extend({
            auth_token: namespace.ya.speechkit.settings.apikey,
            oauth_token: null,
            uuid: namespace.ya.speechkit.settings.uuid,
            url: namespace.ya.speechkit.settings.websocketProtocol + namespace.ya.speechkit.settings.uniUrl,
            onError: function () {},
            onTts: function () {},
            onResult: function () {},
            onDirective: function () {},
            onStreamControl: function () {},
            onEvent: function () {},
            onSpeechStart: function () {},
            onSpeechEnd: function () {},
            lang: "ru-RU",
            voice: "levitan",
        }, options);

        this.socket = null;

        this.onFinish = function () {};

        this.totaldata = 0;
        this.streamCallbacks = new Map();
        this.streams = new Map();
        this.streamId = 1;
        this.ready = false;
        this.seq_number = 0;
    };

    UniProto.prototype = {
        _nextMessageId: function () {
            return 'xxxxxxxx-xxxx-4xxx-yxxx-xxxxxxxxxxxx'.replace(
            /[xy]/g,
            function (c) {
                var r = Math.random() * 16 | 0;
                var v = c == 'x' ? r : (r & 0x3 | 0x8);
                return v.toString(16);
            }
            );
        },
        _nextStreamId: function () {
            this.streamId += 2;
            return this.streamId;
        },
        _sendRaw: function (data) {
            if (this.socket) {
                try {
                    this.socket.send(data);
                } catch (e) {
                    this.options.onError('Socket error: ' + e);
                }
            }
        },

        _sendEvent: function (namespace, name, payload, streamId) {

            this.seq_number++;

            var messageId = this._nextMessageId();
            var event = {
                header: {
                    namespace: namespace,
                    name: name,
                    messageId: messageId,
                    seqNumber: this.seq_number
                },
                payload: payload
            };

            if (streamId) {
                event.header.streamId = streamId;
            }

            this.options.onEvent(event);

            event = JSON.stringify({event: event});

            this._sendRaw(event);

            return messageId;
        },

        sendFanoutMessage: function (payload) {
            this._sendEvent('Messenger', 'PostMessage', payload);
        },

        sendHistoryRequest: function (payload) {
            this._sendEvent('Messenger', 'HistoryRequest', payload);
        },

        sendEditHistoryRequest: function (payload) {
            this._sendEvent('Messenger', 'EditHistoryRequest', payload);
        },

        sendSubscriptionRequest: function (payload) {
            this._sendEvent('Messenger', 'SubscriptionRequest', payload);
        },

        sendSetVoiceChatsRequest: function (payload) {
            this._sendEvent('Messenger', 'SetVoiceChats', payload);
        },

        sendUniproxyMessage: function (payload) {
            this._sendEvent('Messenger', 'Unipost', payload);
        },

        start: function () {
            try {
                this.socket = new WebSocket(this.options.url);
                this.socket.binaryType = "arraybuffer";
            } catch (e) {
                this.options.onError('Error on socket creation: ' + e);
                return;
            }

            this.socket.onopen = function () {
                this._sendEvent("System", "SynchronizeState", this.options);
                this.ready = true;
            }.bind(this);

            this.socket.onmessage = function (e) {
                if (e.data instanceof ArrayBuffer) {
                    var dataview = new DataView(e.data);
                    var streamId = dataview.getUint32(0);
                    if (this.streamCallbacks.has(streamId)) {
                        this.streamCallbacks.get(streamId)(e.data.slice(4));
                    }
                } else {
                    var message = JSON.parse(e.data);
                    if (message.hasOwnProperty("streamcontrol")) {
                        this.options.onStreamControl(true, message.streamcontrol);
                        if (message.streamcontrol.streamId % 2 == 1) {
                            if (this.recorder) {
                                this.recorder.stop();
                                this.options.onSpeechEnd();
                                this.recorder = null;
                            }
                        } else {
                            if (this.streamCallbacks.has(message.streamcontrol.streamId)) {
                                this.streamCallbacks.get(message.streamcontrol.streamId)();
                                this.streamCallbacks.delete(message.streamcontrol.streamId);
                            }
                        }
                    } else if (message.hasOwnProperty("directive")) {
                        this.options.onDirective(message.directive);

                        var directive = message.directive;
                        if (!directive.hasOwnProperty("header") || !directive.hasOwnProperty("payload")) {
                            return;
                        }
                        var header = directive.header;
                        var payload = directive.payload;

                        if (header.namespace == "System") {
                            if (header.name == "EventException") {
                                this.options.onError(payload);
                            } else if (header.name == "GoAway") {
                            }
                        } else if (header.namespace == "ASR") {
                            if (header.name == "Result" || header.name == "MusicResult") {
                                this.options.onResult(payload.recognition);
                            }
                        } else if (header.namespace == "TTS") {
                            if (header.name == "Speak") {
                                this.streamCallbacks.set(header.streamId, this.options.onTts);
                            }
                        } else if (header.namespace == "Vins") {
                        } else if (header.namespace == "Messenger") {
                        }
                    }
                }
            }.bind(this);

            this.socket.onerror = function (error) {
                this.options.onError('Socket error: ' + error);
            }.bind(this);

            this.socket.onclose = function (event) {
            }.bind(this);
        },
        addData: function (streamId, data) {
            var tmp = new ArrayBuffer(4);
            var view = new DataView(tmp);
            view.setUint32(0, streamId);
            this._sendRaw(new Blob([tmp, data]));
        },
        closeStream: function (action) {
            if (this.streams.get(this.streamId)) {
                var event = {
                    streamcontrol: {
                        streamId: this.streamId,
                        messageId: this.streams.get(this.streamId),
                        action: action,
                        reason: 0
                    }
                };

                this.options.onStreamControl(false, event.streamcontrol);

                this._sendRaw(JSON.stringify(event));
            }
        },
        startSynthesis: function (text) {
            this._sendEvent(
                "TTS",
                "Generate",
                {
                    "text": text,
                }
            );
        },
        startRecognition: function () {
            var streamId = this._nextStreamId();
            var now = new Date();
            var engine = this.options.getEngine();
            if (engine == "vins") {
                this.streams.set(streamId, this._sendEvent(
                    "VINS",
                    "VoiceInput",
                    {
                        "application": {
                            "app_id": "com.yandex.alicekit.demo", // in order to support more features
                            "app_version": "8.2.3",
                            "os_version": "8.0",
                            "platform": "android",
                            "uuid": this.options.uuid,
                            "lang": "ru-RU",
                            "client_time": (new Date()).toISOString().substr(0, 19).replace(/[:+-]/g,""),
                            "timezone": "Europe/Moscow",
                            "timestamp": "1486736347",
                        },
                        "header": {
                            "request_id": this._nextMessageId(),
                            "sequence_number": null,
                            "prev_req_id": null,
                        },
                        "request": {
                            "event": {
                                "type": "voice_input",
                            },
                            "voice_session": true,
                            "experiments": $("#experiments").val().split(',')
                        },
                        "topic": $("#topic").val()
                    },
                    streamId
                ));
            } else if (engine == "vins_music2") {
                var eventPayload = {
                    "application": {
                        "app_id": "com.yandex.search",
                        "app_version": "1.2.3",
                        "os_version": "5.0",
                        "platform": "android",
                        "uuid": this.options.uuid,
                        "lang": "ru-RU",
                        "client_time": (new Date()).toISOString().substr(0, 19).replace(/[:+-]/g,""),
                        "timezone": "Europe/Moscow",
                        "timestamp": "1486736347",
                    },
                    "header": {
                        "request_id": this._nextMessageId(),
                    },
                    "request": {
                        "event": {
                            "type": "music_input",
                        },
                        "voice_session": true
                    },
                    "topic": $("#topic").val()
                };
                var content_format = namespace.ya.speechkit.FORMAT.OPUS.mime;
                if (content_format == namespace.ya.speechkit.FORMAT.PCM8.mime) {
                    content_format = "audio/pcm-data";
                }
                var music_headers = {
                    "Content-Type": content_format
                };
                var token = this.options.getMusicOAuthToken();
                if (token != '') {
                    music_headers["Authorization"] = "OAuth " + token;
                }
                eventPayload["music_request2"] = {
                    "headers": music_headers
                }
                this.streams.set(streamId, this._sendEvent(
                    "VINS",
                    "MusicInput",
                    eventPayload,
                    streamId
                ));
            } else if (engine === "messenger_voice") {
                this.streams.set(streamId, this._sendEvent(
                    "Messenger",
                    "VoiceInput",
                    {
                        'ClientMessage': {
                            'Plain': {
                                'ChatId':       $('#voice-fanout-chat-id').val(),
                                'PayloadId':    $('#voice-fanout-payload-id').val(),
                                'Text': {
                                    'MessageText':  ''
                                }
                            }
                        },
                        'ToGuid':       $('#voice-fanout-guid').val(),
                        'TsVal':        Date.now()
                    },
                    streamId
                ));
            } else if (engine == "bio") {
                var content_fomat = namespace.ya.speechkit.FORMAT.OPUS.mime;
                this.streams.set(streamId, this._sendEvent(
                    "Biometry",
                    this.options.biometry_request,
                    {
                        "application": {
                            "app_id": "com.yandex.search",
                            "app_version": "1.2.3",
                            "os_version": "5.0",
                            "platform": "android",
                            "uuid": this.options.uuid,
                            "lang": "ru-RU",
                            "client_time": (new Date()).toISOString().substr(0, 19).replace(/[:+-]/g,""),
                            "timezone": "Europe/Moscow",
                            "timestamp": "1486736347",
                            "format": content_fomat,
                        },
                    },
                    streamId
                ));
            } else if (engine == "asr_merge" || engine == "asr" || engine == "asr_music" || engine == "asr_music2") {
                var content_fomat = namespace.ya.speechkit.FORMAT.OPUS.mime;
                var eventPayload = {
                    "lang": "ru-RU",
                    "topic": $("#topic").val(),
                    "application": "test",
                    "format": content_fomat,
                    "key": "developers-simple-key",
                    "punctuation": this.options.punctuation,
                };
                if ($("#grammar_context").val()) {
                    eventPayload["advanced_options"] = {
                      "grammar" : [ $("#grammar_context").val() ],
                      "grammar_as_context": true,
                    };
                }
                if ($("#context_trigger").val() && $("#context_content").val()) {
                    var content = $("#context_content").val();
                    try {
                        content = JSON.parse(content);
                    } catch (e) {
                        content = content.split(',');
                    }

                    eventPayload["context"] = [
                        {
                            id: 'demo',
                            trigger: $("#context_trigger").val().split(','),
                            content: content,
                        }
                    ];
                }

                if (engine == "asr_music" || engine == "asr_music2") {
                    if (content_fomat == namespace.ya.speechkit.FORMAT.PCM8.mime) {
                        content_fomat = "audio/pcm-data";
                    }
                    var music_headers = {
                        "Content-Type": content_fomat
                    };
                    var token = this.options.getMusicOAuthToken();
                    if (token != '') {
                        music_headers["Authorization"] = "OAuth " + token;
                    }
                    var music_request = "music_request2";
                    if (engine == "asr_music") {
                        music_request = "music_request";
                    }
                    eventPayload[music_request] = {
                        "headers": music_headers
                    }
                }
                this.streams.set(streamId, this._sendEvent("ASR", "Recognize", eventPayload, streamId));
            } else {
                return;
            }
            this.recorder = new namespace.ya.speechkit.Recorder();
            this.recorder.start(function (data) {
                this.addData(streamId, data);
            }.bind(this), namespace.ya.speechkit.FORMAT.OPUS);
            this.options.onSpeechStart();
        },

        stop: function () {
            this.closeStream(0);
            if (this.recorder) {
                this.recorder.stop();
                this.options.onSpeechEnd();
                this.recorder = null;
            }
        },

        stop_spotter: function () {
            this.recorder.stop();
            this.closeStream(2);
            this.recorder = new namespace.ya.speechkit.Recorder();
            this.recorder.start(function (data) {
                this.addData(this.streamId, data);
            }.bind(this), namespace.ya.speechkit.FORMAT.OPUS);            
        },


        disconnect: function () {
            this.stop();
            this.socket.close();
        }
    };

    namespace.ya.speechkit.UniProto = UniProto;
}(this));
