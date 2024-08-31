var SELECTOR_NAMES = {
    'skill_id': {},
    'app': {
        "search_app_prod" : "Поисковое приложение (прод)",
        "search_app_beta" : "Поисковое приложение (бета)",
        "stroka" : "Десктоп. Строка",
        "browser_prod" : "Браузер (прод)",
        "browser_alpha": "Браузер (альфа)",
        "browser_beta": "Браузер (бета)",
        "navigator": "Навигатор",
        "quasar": "Станция",
        "launcher": "Лончер",
        "auto": "Автоголова",
        "elariwatch": "Часы Elari",
        "small_smart_speakers": "Мини-колонка",
        "yandex_phone": "Телефон",
        "yabro_prod": "Десктопный браузер (прод)",
        "yabro_beta": "Десктопный браузер (бета)",
        "yandex_phone": "Телефон",
      	"music_app_prod": "Яндекс Музыка",
        "other": "Другое"
    },
}

var TH_HINTS = {
    'ts': 'Дата и время запроса',
    'uuid': 'id пользователя',
    'query': 'Запрос пользователя',
    'reply': 'Ответ Алисы',
    'app': 'Приложение пользователя',
    'intent': 'Интент ответа Алисы',
    'req_id': 'request_id запроса'
}

var SAMPLE_TABLE = d3.select('table#query-result');

var HIDDEN_FIELDS = ['gc_intent', 'skill_id'];

var FIELD_CONV = {
    'intent': function (row) {
        var intent = stripIntent(row['intent']);
        var info = '';

        if (row['gc_intent']) {
            info += ('\n<span class="text-muted">gc_intent: </span>' + stripIntent(row['gc_intent']));
        }

        var skill_id = row['skill_id'];
        if (skill_id) {
            var skill_name = SELECTOR_NAMES['skill_id'][skill_id] || skill_id;
            info += ('\n<span class="text-muted">skill: </span>' + skill_name);
        }

        if (info) {
            return '<span class="text-muted">intent: </span>' + intent + info;
        } else {
            return intent;
        }
    },

    'ts': function (row) {
        d = new Date(row['ts'] * 1000);
        return d.toISOString().replace(/^(\d+)-(\d+)-(\d+)T(\d+):(\d+):(\d+)\..*/, '$3.$2 $4:$5:$6');
    },

    'app': function (row) {
        return SELECTOR_NAMES['app'][row['app']];
    }
};

function stripIntent(intent) {
    return intent
        .replace(/^personal_assistant\t/, '')
        .replace(/^scenarios\t/, '')
        .replace(/^general_conversation\t/, '');
}

function updateSampleTable(table, data) {
    var columns = $.map(data['meta'], function (col) {
        return col['name'];
    });
    columns = $.grep(columns, function (col) {
        return !HIDDEN_FIELDS.includes(col);
    });

    var th = table.select("thead tr").selectAll("th").data(columns);
    th.enter().append("th");
    th.exit().remove();
    th.attr("title", function(d) {return TH_HINTS[d]});
    th.text(function(d){return d});

    var tr = table.select("tbody").selectAll("tr")
        .data(data['data']);

    tr.enter().append("tr");
    tr.exit().remove();

    var td = tr.selectAll("td")
      .data(function(row) {
          return $.map(columns, function (col) {
              var conv = FIELD_CONV[col];
              if (conv) {
                  return conv(row);
              } else {
                  return row[col];
              }
          })
      });

    td.exit().remove();
    td.enter().append("td");
    td.html(function(d) {
        return d.replace(/\n/g, '<br/>').replace(/\t/g, '.')
    });
}


function updateStatistics(counts, total) {
    $('#total').append("Всего запросов: " + total);

    if (counts) {
        var tr = d3.select('table#counts').select("tbody").selectAll("tr").data(counts["data"]);
        tr.enter().append("tr");
        tr.exit().remove();

        var td = tr.selectAll("td").data(function(row){
            return $.map(['fielddate', 'cnt'], function(col) {return row[col]});
        });
        td.exit().remove();
        td.enter().append("td");
        td.html(function(d){return d});

        $('#counts_display').show();
        $('#counts_display').click(function() {
            $(this).next().toggle();
        })
    }
}


function fillSelector(id) {
    var $selector = $('#selectors [name=' + id + ']');

    var arr = Object.entries(SELECTOR_NAMES[id]).sort(function (a, b) {
        if (a[1].toLowerCase() < b[1].toLowerCase()) {
            return -1
        } else {
            return 1
        }
    });
    $.each(arr, function(idx, pair) {
        $selector.append(new Option(pair[1], pair[0]));
    });
    // TODO: Нужно ли удаление значений?
}

function qsToSelectors() {
    var params = new URLSearchParams(window.location.search);
    for (var el of params.entries()) {
        var id = el[0];
        var val = el[1];

        if (id == 'stats_daily' && val == '1') {
            document.getElementById('id_stats_daily').checked = true;
            continue;
        }
        var $selector = $('#selectors [name=' + id + ']');
        $selector.val(val);
    }
}


function loadSkills() {
    $.ajax('dialogs/ext-skills', {
        // FIXME: Теоретически, этот урл может загрузиться позже, чем /dialogs/query/ и названия скилов отображаться не будут
        'success': function (data) {
            SELECTOR_NAMES['skill_id'] = data;
            fillSelector('skill_id');
            qsToSelectors();
        },
        'error': function (request, status, err) {
            var msg = "Не удалось загрузить названия скилов<br/>"
                + request.status + ' / ' + err + '<br/>'
                + request.responseText;
            showErrors({'skill_id': [msg]});
        }
    });
}


function showErrors(errors) {
    var $selectors = $('#selectors');
    for (var el of Object.entries(errors)) {
        var $errlist = $selectors.find('#err_' + el[0]);
        for (var err of el[1]) {
            $errlist.append('<li class="alert alert-danger" role="alert">' + err + '</li>');
        }
    }
}


function loadSample() {
    var $waiter = $('#query-waiter');
    $waiter.show();
    var $submitter = $('#selectors input[type=submit]');
    $submitter.prop( "disabled", true);
    // TODO: Убираем старые ошибочки

    var params = new URLSearchParams(window.location.search);
    $.ajax('/dialogs/query?' + params.toString(), {
        'success': function (data) {
            $waiter.hide();
            $submitter.prop( "disabled", false);
            updateSampleTable(SAMPLE_TABLE, data['sample_table']);
            updateStatistics(data["counts"], data["total"]);
        },
        'error': function (request, status, err) {
            $waiter.hide();
            $submitter.prop( "disabled", false);
            if (request.status === 400) {
                showErrors(JSON.parse(request.responseText));
            } else {
                var msg = "Не удалось загрузить выборку диалогов<br/>" +
                    request.status + ' / ' + err + '<br/>'
                    + request.responseText;
                showErrors({'all': [msg]});
            }
        }
    });
}

loadSkills();
fillSelector('app');

if (window.location.search) {
    loadSample();
}
