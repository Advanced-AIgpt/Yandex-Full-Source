(function (namespace) {
    hljs.configure({useBR: true});

    var uni = null;
    var tts_buf = null;
    var interval_id = null;

    var last_time = Date.now();

    document.getElementById('connect_btn').onclick = function () {
        $('#events_log').html('');

        $('#connect_btn').prop('disabled', true);
        $('#disconnect_btn').prop('disabled', false);
        $('#start_btn').prop('disabled', false);
        $('#stop_btn').prop('disabled', true);
        $('#send_raw_event_btn').prop('disabled', false);

        var apikey = $('#apikey').val();
        var uuid = $('#uuid').val();
        var biometry_group = $('#biometry_group').val();
        var biometry = $('#biometry').val();
        var vins_url = $('#vins_url').val();
        var vins_partial = document.getElementById('vins_partial').checked;
        var oauth_token = $('#oauth-token').val();
        var messenger = document.getElementById('messenger').checked;

        var e = document.getElementById('bio_request');
        var biometry_request = "Identify";

        var addEventToTable = function (incoming, res) {
            var newline = '<tr ';

            if (incoming) {
                newline += 'class="info"';
            }

            var now_time = Date.now();

            newline += '><td>' + (now_time - last_time) + "</td>";

            last_time = now_time;

            if (incoming) {
                newline += '<td>&larr; ';
            } else {
                newline += '<td>&rarr; ';
            }
            var header = hljs.highlight("json", JSON.stringify(res.header)).value;
            header = hljs.fixMarkup(header);
            if (res.header.namespace + "." + res.header.name == "ASR.Result") {
                if (res.payload.recognition &&  res.payload.recognition.length > 0) {
                    header += "<br />Best result: " + res.payload.recognition[0].normalized;
                }
            }

            var payload = hljs.highlight("json", JSON.stringify(res.payload)).value;
            payload = hljs.fixMarkup(payload);

            payload = '<div class="panel panel-default">' +
                '<div class="panel-heading">' +
                '<h4 class="panel-title">' +
                '<a data-toggle="collapse" href="#collapse' + res.header.messageId.replace('-', '') + '">payload</a>' +
                '</h4>' +
                '</div>' +
                '<div id="collapse' + res.header.messageId.replace('-','') + '" class="panel-collapse collapse">' +
                '<div class="panel-body">' + payload + '</div>' +
                '</div>' +
                '</div>';

            newline += res.header.namespace + "." + res.header.name + "</td>" +
            "<td>" + header + "</td>" +
            "<td>" + payload + "</td>";
            $('#events_log').append(newline);
        };
        uni = new ya.speechkit.UniProto({
            auth_token: apikey,
            oauth_token: oauth_token,
            uuid: uuid,
            vins_partial: vins_partial,
            biometry_group: biometry_group,
            biometry_score: true,
            biometry_request: biometry_request,
            format: (ya.speechkit.FORMAT.OPUS || ya.speechkit.FORMAT.PCM16).mime,
            voice: document.getElementById("voice").value,
            punctuation: document.getElementById("punctuation").checked,
            advanced_options: {
                partial_results: document.getElementById("partial").checked,
                manual_punctuation: document.getElementById("manual_punctuation").checked,
            },
            biometry_classify: biometry,
            getEngine: function() {
                var e = document.getElementById('select_engine');
                return e.options[e.selectedIndex].value;
            },
            getMusicOAuthToken: function() {
                var e = document.getElementById('music_oauth_token');
                return e.value;
            },
            onResult: function (res) {
            },
            onError: function (err) {
                console.log(err);
            },
            onDirective: function(directive)
            {
                addEventToTable(true, directive)

                var drNamespace = directive["header"]["namespace"];
                var drName = directive["header"]["name"];

                // Biometry UI
                var payload = directive.payload;
                if (drNamespace == "Biometry" && drName == "IdentifyComplete") {
                    if (payload.status == "ok"){
                        $("#bio_status").removeClass("label-warning").addClass("label-success");
                    } else{
                        $("#bio_status").removeClass("label-success").addClass("label-warning");
                    }
                    $("#bio_status").html(payload.status).show().delay(500).fadeOut("fast");

                    $("#bio_request_id").val(payload.request_id)
                    reqs = $("#bio_requests").val();
                    if (reqs != "") reqs += ","
                    $("#bio_requests").val(reqs + payload.request_id)

                    $("#biometry_scoring_results").html("");
                    for (var m in payload.scores_with_mode) {
                        for (var i in m.scores) {
                            user = m.scores[i].uuid;
                            score = m.scores[i].score;
                            $("#biometry_scoring_results").append("<tr><td>"+m.mode+"</td><td>"+user+"</td><td>"+score+"</td></tr>");
                        }
                    }
                }

                if (drNamespace == "Biometry" && (drName == "UserCreation" || drName == "UserRemoved")) {
                    $("#bio_user_status").show().delay(500).fadeOut("fast");
                }

                if (drNamespace == "Messaging" && drName == "AsyncMessage") {
                    var transferId = directive["header"]["transferId"]
                    uni._sendEvent("Messaging", "AsyncMessageOk", {transferId: transferId})
                }

                if (drNamespace == "Messenger" && drName == "Ack") {
                    console.log("message transferred to fanout");
                }
            },
            onEvent: addEventToTable.bind(null, false),
            onStreamControl: function (incoming, res) {
                var newline = '<tr ';

                if (incoming) {
                    newline += 'class="info"';
                }

                var now_time = Date.now();

                newline += '><td>' + (now_time - last_time) + "</td>";

                last_time = now_time;

                if (incoming) {
                    newline += '<td>&larr; ';
                } else {
                    newline += '<td>&rarr; ';
                }
                newline += 'streamcontrol</td>' +
                    "<td>" + hljs.fixMarkup(hljs.highlight("json", JSON.stringify(res)).value) + "</td>" +
                    "<td></td>";
                $('#events_log').append(newline);
            },
            onTts: function (chunk) {
                document.getElementById('mic_div').className = 'spk0';
                if (typeof chunk === 'undefined') {
                    //var res = tts_buf.reduce(function (a,b) {return a.concat(b);}, []);
                    var blob = new Blob([tts_buf], {type: uni.options.format});
                    var url = URL.createObjectURL(blob);
                    ya.speechkit.play(url, uni.startRecognition.bind(uni));
                    document.getElementById('mic_div').className = 'spk1';
                    tts_buf = null;
                } else {
                    if (!tts_buf) {
                        tts_buf = chunk;
                    } else {
                        tts_buf = _appendBuffer(tts_buf, chunk);
                    }
                }
            },
            onSpeechStart: function () {
                interval_id = setInterval(function () {
                    var cur_class = document.getElementById('mic_div').className;
                    document.getElementById('mic_div').className = 'mic' + (1 - cur_class[3]);
                }, 500);
                document.getElementById('mic_div').className = 'mic1';

                $('#start_btn').prop('disabled', true);
                $('#stop_btn').prop('disabled', false);
            },
            onSpeechEnd: function () {
                clearInterval(interval_id);
                document.getElementById('mic_div').className = 'mic0';
            }
        });

        if (vins_url) {
            uni.options.vinsUrl = vins_url;
        }
        if (messenger) {
            uni.options.Messenger = {version: 1};
        }
        uni.start();
    };

    var _appendBuffer = function(buffer1, buffer2) {
        var tmp = new Uint8Array(buffer1.byteLength + buffer2.byteLength);
        tmp.set(new Uint8Array(buffer1), 0);
        tmp.set(new Uint8Array(buffer2), buffer1.byteLength);
        return tmp.buffer;
    };

    document.getElementById('start_btn').onclick = function () {
        $('#content_uttr').html('');
        $('#content_curr').html('');

        if (uni.options.getEngine() == 'tts') {
            uni.startSynthesis(document.getElementById('tts_text').value);
        } else {
            uni.startRecognition();
        }

        $('#start_btn').prop('disabled', true);
        $('#stop_btn').prop('disabled', false);
    };

    document.getElementById('stop_btn').onclick = function () {
        uni.stop();
        $('#start_btn').prop('disabled', false);
        $('#stop_btn').prop('disabled', true);
    };

    document.getElementById('disconnect_btn').onclick = function () {
        uni.disconnect();

        $('#connect_btn').prop('disabled', false);
        $('#disconnect_btn').prop('disabled', true);
        $('#start_btn').prop('disabled', true);
        $('#stop_btn').prop('disabled', true);
        $('#send_raw_event_btn').prop('disabled', true);
    };

    document.getElementById('bio_remove_user').onclick = function () {
        cmd = {
                "event": {
                    "header": {
                        "namespace": "Biometry",
                        "name": "RemoveUser",
                        "messageId": uni._nextMessageId()
                    },
                    "payload": {
                        "biometry_group": $("#biometry_group").val(),
                        "user_id": $("#bio_user_id").val()
                    }
                }
            }
        uni._sendRaw(JSON.stringify(cmd))
    };

    document.getElementById('bio_create_user').onclick = function () {
        cmd = {
                "event": {
                    "header": {
                        "namespace": "Biometry",
                        "name": "CreateOrUpdateUser",
                        "messageId": uni._nextMessageId()
                    },
                    "payload": {
                        "biometry_group": $("#biometry_group").val(),
                        "user_id": $("#bio_user_id").val(),
                        "requests_ids": $("#bio_requests").val().split(",")
                    }
                }
            }
        uni._sendRaw(JSON.stringify(cmd))
    };

    document.getElementById('send_raw_event_btn').onclick = function () {
        var val = document.getElementById('raw_event_input').value;
        uni._sendRaw(val);
    }

    var _updateVisibilityEngineOptions = function() {
        var e = document.getElementById('select_engine');
        var display = 'none';
        if (e.options[e.selectedIndex].value == "asr_music"
            || e.options[e.selectedIndex].value == "asr_music2"
            || e.options[e.selectedIndex].value == "vins_music2"
        ) {
            display = 'inline';
        }
        document.getElementById('music_options').style.display = display;

        if (e.options[e.selectedIndex].value == 'tts') {
            display = 'inline';
        } else {
            display = 'none';
        }
        document.getElementById("tts_options").style.display = display;

        if (e.options[e.selectedIndex].value == 'bio') {
            display = 'inline';
        } else {
            display = 'none';
        }
        document.getElementById("bio_options").style.display = display;

        if (e.options[e.selectedIndex].value === 'messenger_voice') {
            display = 'inline';
        } else {
            display = 'none';
        }
        document.getElementById("messenger_options").style.display = display;
    };

    _updateVisibilityEngineOptions();

    document.getElementById('select_engine').onchange = _updateVisibilityEngineOptions;

    document.getElementById('btn-fanout-push-message').onclick = function () {
        var payload = {
            'Type':         parseInt($('#fanout-type').val()),
            'ToGuid':       $('#fanout-guid').val(),
            'ChatId':       $('#fanout-chat-id').val(),
            'PayloadId':    $('#fanout-payload-id').val(),
            'PayloadData':  JSON.parse($('#fanout-payload-data').val()),
            'TsVal':        Date.now()
        };

        console.log('mssngr -> send "' + payload.Type + '" to "' + payload.ToGuid + '"');
        uni.sendFanoutMessage(payload);
        console.log('mssngr -> sent "' + payload.Type + '" to "' + payload.ToGuid + '"');
    };

    document.getElementById('btn-fanout-push-message-2').onclick = function () {
        var payload = {
            'ClientMessage': {
                'Plain': {
                    'ChatId':       $('#fanout-chat-id').val(),
                    'PayloadId':    $('#fanout-payload-id').val(),
                    'Text': {
                        'MessageText':  $('#fanout-payload-data').val()
                    }
                }
            },
            'ToGuid':       $('#fanout-guid').val(),
            'TsVal':        Date.now()
        };

        console.log('mssngr -> send ClientMessage.Plain to "' + payload.ToGuid + '"');
        uni.sendFanoutMessage(payload);
        console.log('mssngr -> sent ClientMessage.Plain to "' + payload.ToGuid + '"');
    };

    document.getElementById('btn-uniproxy-push-message').onclick = function () {
        var payload = {
            'Type':         parseInt($('#fanout-type').val()),
            'ToGuid':       $('#fanout-guid').val(),
            'ChatId':       $('#fanout-chat-id').val(),
            'PayloadId':    $('#fanout-payload-id').val(),
            'PayloadData':  JSON.parse($('#fanout-payload-data').val()),
            'TsVal':        Date.now()
        };

        console.log('mssngr -> send "' + payload.Type + '" to "' + payload.ToGuid + '"');
        uni.sendUniproxyMessage(payload);
        console.log('mssngr -> sent "' + payload.Type + '" to "' + payload.ToGuid + '"');
    };

    document.getElementById('btn-history-request').onclick = function () {
        var payload = {
            'Guid':         $('#history-guid').val(),
            'ChatId':       $('#history-chat-id').val(),
            'MinTimestamp': parseInt($('#history-min-ts').val()),
            'MaxTimestamp': parseInt($('#history-max-ts').val()),
            'Limit':        parseInt($('#history-limit').val()),
            'Offset':       parseInt($('#history-offset').val()),
            'RequestId':    $('#history-request-id').val()
        };

        uni.sendHistoryRequest(payload);
    };

    document.getElementById('btn-edit-history-request').onclick = function () {
        var payload = {
            'Guid':         $('#history-guid').val(),
            'ChatId':       $('#history-chat-id').val(),
            'MinTimestamp': parseInt($('#history-min-ts').val()),
            'Limit':        parseInt($('#history-limit').val()),
            'RequestId':    $('#history-request-id').val()
        };

        uni.sendEditHistoryRequest(payload);
    };

    document.getElementById('btn-subscription-request').onclick = function () {
        var payload = {
            'Guid':         $('#subscription-guid').val(),
            'ChatId':       $('#subscription-chat-id').val(),
            'ToGuid':       $('#subscription-to-guid').val(),
            'MessageType':  parseInt($('#subscription-message-type').val()),
            'TtlMcs':       parseInt($('#subscription-ttl').val()),
            'RequestId':    $('#subscription-request-id').val()
        };

        uni.sendSubscriptionRequest(payload);
    };

    document.getElementById('btn-set-voice-chats').onclick = function () {
        const payload = {
            ChatIds: $('#set-voice-chats-ids').val().split(','),
        };
        uni.sendSetVoiceChatsRequest(payload);
    };

    $('#disconnect_btn').prop('disabled', true);
    $('#start_btn').prop('disabled', true);
    $('#stop_btn').prop('disabled', true);
    $('#send_raw_event_btn').prop('disabled', true);
    $('#apikey').val(namespace.ya.speechkit.settings.apikey);
    $('#uuid').val(namespace.ya.speechkit.settings.uuid);
}(this));
