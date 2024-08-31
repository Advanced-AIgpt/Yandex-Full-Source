var last_request = null;
var last_response = null;
var last_sessionId = null;

$(document).ready(function () {
    var tts_stream = new ya.speechkit.Tts({errorCallback: console.log});

    tts_stream.speakers().then(
        function (data) {
            var langs = [];
            $.each(data, function (j, value) {
                if (value.coreVoice) {
                    $('#sel_name').append(
                        $('<div class="list-group-item">').append(
                            $('<span>' + value.displayName + '</span>')).append(
                                $(' <input type="text" name="voice" val="' + value.name +
                                    '" value="' + (value.name == 'oksana' ? '1.0' : '0.0') + '" />')));
                }

                $.each(value.languages, function (i, lang) {
                    if ($.inArray(lang, langs) == -1) {
                        $('#smpl_sel_lang').append(
                            $('<option name="smpl_lang" value="' + lang + '">' + lang + '</option>'));
                        $('#sel_lang').append(
                            $('<option name="lang" value="' + lang + '">' + lang + '</option>'));
                        langs.push(lang);
                    }
                });
                $('#smpl_sel_name').append(
                    $('<option name="smpl_voice" lang="*" value="' + 
                    value.name + '">' + value.displayName + '</option>'));
            });

            $('option[name=smpl_emotion]').prop('selected', false)
                            .filter('[value="' + $('#saved_emotion').val() + '"]')
                                                .prop('selected', true);
            $('option[name=smpl_lang]').prop('selected', false)
                            .filter('[value^="' + $('#saved_lang').val() + '"]')
                                                .prop('selected', true);
            // $('option[name=smpl_voice]').hide().prop('selected', false);
            $('option[lang=' + $('option[name=smpl_lang]').val() + ']')
                                .show().filter(':first').prop('selected', true);

            $('option[name=smpl_voice]').prop('selected', false)
                            .filter('[value="' + $('#saved_voice').val() + '"]')
                                                .prop('selected', true);
            smpl_call_tts_stream(function (blob) {
                $('#analyser').analyser('option', {audio: blob});
                $('#analyser').find('div.cell:first').get(0).onclick();
            });
        }
    ).catch(
        function (msg) {
            console.log(msg);
        }
    );

    var analyser_created = false;

    $('#sel_engine').on('change', function () {
        if (this.value == 'ytcp') {
            $('#sel_name_ytcp').show();
            $('#sel_name_tcp').hide();
            $('#speed').parent().show();
        } else {
            $('#speed').parent().hide();
            $('#sel_name_ytcp').show();
            $('#sel_name_tcp').show();
            $('#sel_name_ytcp').hide();
        }
    });

    var change_default_text = function (text_control) {
        return function () {
            if (this.value == 'uk-UA') {
                $(text_control).val('Ще не вмерла України і слава, і воля');
            } else if (this.value == 'en-US' || this.value == 'en_GB') {
                $(text_control).val('God save our gracious Queen, Long live our noble Queen');
            } else if (this.value == 'tr-TR' || this.value == 'tr_TR') {
                $(text_control).val('Korkma, sönmez bu şafaklarda yüzen al sancak');
            } else if (this.value == 'cs_CZ') {
                $(text_control).val('Kde domov můj? Kde domov můj? A to je ta krásná země, země česká domov můj!');
            } else if (this.value == 'de_DE') {
                $(text_control).val('Deutschland, Deutschland über alles, über alles in der Welt');
            } else if (this.value == 'it_IT') {
                $(text_control).val("Fratelli d'Italia, l'Italia s'è desta!");
            } else if (this.value == 'fr_FR') {
                $(text_control).val('Allons enfants de la Patrie, Le jour de gloire est arrivé !');
            } else if (this.value == 'es_ES') {
                $(text_control).val('¡Viva España! Cantemos todos juntos con distinta voz y un solo corazón.');
            } else if (this.value == 'pl_PL') {
                $(text_control).val('Jeszcze Polska nie zginęła, Kiedy my żyjemy.');
            } else if (this.value == 'sv_SE') {
                $(text_control).val('Du gamla, Du fria, Du fjällhöga nord, Du tysta, Du glädjerika sköna!');
            } else if (this.value == 'pt_PT') {
                $(text_control).val('Heróis do mar, nobre povo, Nação valente, imortal, Levantai hoje de novo O esplendor de Portugal!');
            } else if (this.value == 'fi_FI') {
                $(text_control).val('Oi maamme, Suomi, synnyinmaa, soi, sana kultainen!');
            } else if (this.value == 'ar_AE') {
                $(text_control).val('عيشي بلادي عاش اتحاد إماراتنا عشت لشعب دينه الإسلام هديه القرآن');
            } else if (this.value == 'ca_ES') {
                $(text_control).val('Catalunya triomfant, tornarà a ser rica i plena.');
            } else if (this.value == 'da_DK') {
                $(text_control).val('Der er et yndigt land, det står med brede bøge nær salten østerstrand :|');
            } else if (this.value == 'nl_NL') {
                $(text_control).val('Wilhelmus van Nassouwe ben ik, van Duitsen bloed, den vaderland getrouwe blijf ik tot in den dood.');
            } else if (this.value == 'el_GR') {
                $(text_control).val('Σε γνωρίζω από την κόψη Του σπαθιού την τρομερή, Σε γνωρίζω από την όψη, Που με βιά μετράει τη γη. ');
            } else if (this.value == 'no_NO') {
                $(text_control).val('Ja, vi elsker dette landet, som det stiger frem, furet, værbitt over vannet.');
            } else if (this.value == 'zh_CN') {
                $(text_control).val('起来！不愿做奴隶的人们！ 把我们的血肉， 筑成我们新的长城！');
            } else if (this.value == 'ja_JP') {
                $(text_control).val('君が代は 千代に八千代に さざれ（細）石の いわお（巌）となりて こけ（苔）の生すまで');
            } else {
                $(text_control).val('Россия — священная наша держава');
            }
        };
    };
    $('#sel_lang').on('change', change_default_text('#text'));
    $('#smpl_sel_lang').on('change', change_default_text('#smpl_text'));

    var call_tts_stream = function (callback) {
        var genders = [];
        $('input[name=gender]').each(function (i) {
            genders[i] = [$(this).attr('val'), $(this).val()];
        });

        var emotions = [];
        $('input[name=emotion]').each(function (i) {
            emotions[i] = [$(this).attr('val'), $(this).val()];
        });

        var voices = [];
        $('input[name=voice]').each(function (i) {
            voices[i] = [$(this).attr('val'), $(this).val()];
        });

        var engine = 'ytcp';

        var speed = $('#speed').val();
        if (analyser_created) {
            $('#analyser').analyser('destroy');
        }
        $('#analyser').analyser();
        analyser_created = true;

        last_request = $('#text').val();

        var opts = {
            lang: $('option[name=lang]:selected').val(),
            voices: voices,
            emotions: emotions,
            genders: genders,
            speed: speed,
            volume: $('#volume').val(),
            apikey: $('#apikey').val(),
            fast: $('#fast').is(':checked'),
            engine: engine,
            quality: $('option[name=quality]:selected').val(),
            dataCallback: callback,
            infoCallback: function (info) {
                last_response = info;
                last_sessionId = tts_stream.sessionId;
                $('#analyser').analyser('option', {alignResponse: info});
            },
            errorCallback: function (err) {
                console.log(err);
            }
        };

        $('input[name=tuning]').each(function (i) {
            if ($(this).val() !== '') {
                opts[$(this).attr('val')] = $(this).val();
            }
        });

        tts_stream.speak($('#text').val(), opts);
    };

    var smpl_call_tts_stream = function (callback) {
        var emotion = $('option[name=smpl_emotion]:selected').val();
        var voice = $('option[name=smpl_voice]:selected').val();
        var speed = $('#smpl_speed').val();

        var engine = 'ytcp2';

        if (analyser_created) {
            $('#analyser').analyser('destroy');
        }
        $('#analyser').analyser();
        analyser_created = true;

        last_request = $('#smpl_text').val();
        tts_stream.speak($('#smpl_text').val(), {
            lang: $('option[name=smpl_lang]:selected').val(),
            voice: voice,
            emotion: emotion,
            speed: speed,
            volume: $('#smpl_volume').val(),
            quality: $('option[name=smpl_quality]:selected').val(),
            apikey: $('#apikey').val(),
            engine: engine,
            dataCallback: callback,
            infoCallback: function (info) {
                last_response = info;
                last_sessionId = tts_stream.sessionId;
                $('#analyser').analyser('option', {alignResponse: info});
            },
            errorCallback: function (err) {
                console.log(err);
            }
        });
    };

    $('#download_btn').click(function () {
        call_tts_stream(function (blob) {
            $('#analyser').analyser('option', {audio: blob});

            var a = document.createElement('a');

            document.body.appendChild(a);
            a.style = 'display: none';

            var url = window.URL.createObjectURL(blob);
            a.href = url;
            a.download = 'tts.wav';
            a.click();
            // window.URL.revokeObjectURL(url);
            document.body.removeChild(a);
        });
    });

    $('#speak_btn').click(function () {
        call_tts_stream(function (blob) {
            $('#analyser').analyser('option', {audio: blob});
            $('#analyser').find('div.cell:first').get(0).onclick();
        });
    });

    $('#smpl_download_btn').click(function () {
        smpl_call_tts_stream(function (blob) {
            $('#analyser').analyser('option', {audio: blob});

            var a = document.createElement('a');

            document.body.appendChild(a);
            a.style = 'display: none';

            var url = window.URL.createObjectURL(blob);
            a.href = url;
            a.download = 'tts.wav';
            a.click();
            // window.URL.revokeObjectURL(url);
            document.body.removeChild(a);
        });
    });

    $('#smpl_speak_btn').click(function () {
        smpl_call_tts_stream(function (blob) {
            $('#analyser').analyser('option', {audio: blob});
            $('#analyser').find('div.cell:first').get(0).onclick();
        });
    });

});
