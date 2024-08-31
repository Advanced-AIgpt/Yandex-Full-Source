function createTimer() {
    var startTime = new Date();
    return {
        elapsed: function () {
            var endTime = new Date();
            var timeDiff = endTime - startTime;
            return timeDiff / 1000;
        }
    };
}

$('option[name=format]').filter('[value="MP3"]').hide();
$('option[name=format]').filter('[value="WAV"]').hide();
$('option[name=format]').filter('[value="SPX"]').hide();
var dict = new ya.speechkit.SpeechRecognition();
$('#uuid').val(ya.speechkit.settings.uuid);
$('#deviceuuid').val('xxxxxxxx-xxxx-4xxx-yxxx-xxxxxxxxxxxx'.replace(
    /[xy]/g,
    function (c) {
        var r = Math.random() * 16 | 0;
        var v = c == 'x' ? r : (r & 0x3 | 0x8);
        return v.toString(16);
    }
));
$('#custom_div').hide();

$('#custom_lm').change(function () {
    //$('#sel_model').prop('disabled', $(this).is(':checked'));
    //$('#sandbox_model').prop('disabled', $(this).is(':checked'));
    //$('option[name=model]').prop('selected', false).filter('[value="onthefly"]').prop('selected', true);
    //$('#sandbox_model').val('onthefly');
    //$('#sel_lang').prop('disabled', $(this).is(':checked'));
    if ($(this).is(':checked')) {
        $('#custom_div').show();
    } else {
        $('#custom_div').hide();
    }
});

$('#sel_format').on('change', function (e) {
   $('#format_mime').val(ya.speechkit.FORMAT[this.value].mime);
});

$('#custom_lang').change(function () {
    $('#sel_model').prop('disabled', $(this).is(':checked'));
    $('#sandbox_model').prop('disabled', $(this).is(':checked'));
    $('#sel_lang').prop('disabled', $(this).is(':checked'));
    $('#inp_custom_lang').prop('disabled', !$(this).is(':checked'));
});

$('#sel_model').on('change', function (e) {
    $('#sandbox_model').val(this.value);
});


$.each(['ru-RU', 'en-US', 'tr-TR', 'uk-UA', 'de-DE', 'es-ES', 'fr-FR', 'it-IT'], function (i, lang) {
    if (ya.speechkit.isLanguageSupported(lang)) {
        $('#sel_lang')
            .append($('<option name="lang">').val(lang)
            .text(lang +
            ((ya.speechkit.settings.langWhitelist.indexOf(lang) < 0) ? ' (native)' : ' (yandex)')));
    }
});
document.getElementById('start_btn').onclick = function () {
    var apikey = $('#apikey').val();
    var uuid = $('#uuid').val();
    var deviceuuid = $('#deviceuuid').val();
    var snr_flags = $('#snr_flags').val();
    if (snr_flags) {
        snr_flags = snr_flags.split(",").map(function(a,b) { var tmp = a.split('='); return {'name': tmp[0], 'value': tmp[1]}; });
    } else {
        snr_flags = [];
    }
    
    var modelValue = sandbox_model.value;
    var formatValue = $('option[name=format]:selected').val();
    var langValue = $('option[name=lang]:selected').val();

    if (langValue != 'ru-RU' && modelValue == 'mapsyari')
    {
        modelValue = 'maps';
    }

    console.log('Format: ' + formatValue + ' model: ' + modelValue + ' lang: ' + langValue);

    var format = ya.speechkit.FORMAT[formatValue];

    $('#content_uttr').html('');
    $('#content_curr').html('');

    var timer = createTimer();
    $('#uttr_log').html('');

    dict.start({
        apikey: apikey,
        uuid: uuid,
        yandexuid: $('#yandexuid').val(),
        biometry_group: deviceuuid,
        model: modelValue,
        applicationName: (sel_model.value == 'custom' ? 'sandbox' : undefined),
        lang: langValue,
        format: format,
        errorCallback: function (msg) {
            // console.log(msg);
            alert(msg);
        },
        dataCallback: function (text, uttr, merge, ai, biometry, metainfo) {
            if (uttr) {
                console.log(metainfo.lang + '-' + metainfo.topic + '-' + metainfo.version + ' from ' + metainfo.load_timestamp);
                $('#content_uttr').append(' ' + text);
                $('#uttr_log').append('<div><span class="label label-default">' +
                    timer.elapsed() +
                    ' </span><span class="label label-info">' +
                    text +
                    '</span></div>');
                text = '';

                $('#alignblock').append('<tr></tr');
                $('<tr class="info"/><td>Beam</td><td>' + metainfo.minBeam + '</td><td>' + metainfo.maxBeam + '</td></tr>').appendTo('#alignblock');
                // $('<tr class="info"/><td>' + metainfo.lang + '-' + metainfo.topic + '-' + metainfo.version + '</td><td>' + metainfo.load_timestamp + '</td></tr>').appendTo('#alignblock');
            }
            var align_info = ai;
            $.each(align_info, function (j, recognition) {
                var newline = $('<tr' + (j === 0 ? ' class="info" ' : '') + '/>');

                var newcell = $('<td>' + recognition.confidence.toFixed(3) + '</td>');
                newcell.hover(
                            function () {
                                $(this).popover('show');
                            },
                            function () {
                                $(this).popover('hide');
                            }
                        );
                newcell.appendTo(newline);
                newcell.popover({
                        title: recognition.confidence,
                        placement: 'top',
                        container: 'body',
                        content: '<p>' +
                            ['Start: ' + recognition.start,
                            'End: ' + recognition.end,
                            'Acoustic score: ' + recognition.acoustic.toFixed(3),
                            'Graph score: ' + recognition.graph.toFixed(3),
                            'LM score: ' + recognition.lm.toFixed(3),
                            'Total: ' + recognition.total.toFixed(3)
                            ].join('</p><p>') + '</p>',
                        html: true
                    });

                $.each(recognition.words, function (i, word) {
                    newcell = $('<td>' + word.value +
                                    ' <span class="badge">' + word.confidence.toFixed(3) + '</span></td>');
                    newcell.hover(
                            function () {
                                $(this).popover('show');
                            },
                            function () {
                                $(this).popover('hide');
                            }
                        );
                    newcell.appendTo(newline);
                    newcell.popover({
                            title: word.value,
                            placement: 'top',
                            container: 'body',
                            content: '<p>' +
                                ['Start: ' + word.align.start,
                                'End: ' + word.align.end,
                                'Acoustic score: ' + word.align.acoustic.toFixed(3),
                                'Graph score: ' + word.align.graph.toFixed(3),
                                'LM score: ' + word.align.lm.toFixed(3),
                                'Total: ' + word.align.total.toFixed(3)
                                ].join('</p><p>') + '</p>',
                            html: true
                        });
                });

                newline.appendTo('#alignblock');
            });
            
            $.each(biometry, function (j, bio) {
                var newline = $('<tr' + (j === 0 ? ' class="info" ' : '') + '/>');
                var newcell = $('<td>' + bio.confidence.toFixed(3) + '</td><td>' + bio.tag + '</td><td>' + bio.class + '</td>');
                newcell.appendTo(newline);
                newline.appendTo('#alignblock');
            });


            $('#content_curr').html(text);
        },
        infoCallback: function (info) {
            $('#bytes_send').html(info.send_bytes);
            $('#packages_send').html(info.send_packages);
            $('#processed').html(info.processed);
        },
        customGrammar:  (custom_lm.checked && !custom_model.value.startsWith("<") ? custom_model.value.split('\n') : []),
        srgs:  (custom_lm.checked && custom_model.value.startsWith("<") ? custom_model.value : undefined),
        capitalize: capitalize.checked,
        expNumCount: parseInt(exp_num_count.value),
        allowStrongLanguage: !stronglanguagefilter.checked,
        allowMultiUtterance: allowmultiutt.checked,
        punctuation: punctuation.checked,
        manualPunctuation: manual_punctuation.checked,
        partialResults: partialresults.checked,
        utteranceSilence: parseInt(uttr_sil.value),
        cmnWindow: parseInt(cmn_window.value),
        cmnLatency: parseInt(cmn_latency.value),
        chunkProcessLimit: parseInt(chunk_process_limit.value),
        biometry: sel_biometry.value,
        use_snr: use_snr.checked,
        snr_flags: snr_flags,
    });
};

document.getElementById('pause_btn').onclick = dict.pause.bind(dict);
document.getElementById('stop_btn').onclick = dict.stop.bind(dict);
