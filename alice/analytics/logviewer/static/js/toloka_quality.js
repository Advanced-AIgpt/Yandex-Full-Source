var SELECTOR_NAMES = {
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
        "other": "Другое"
    },
}

var TH_HINTS = {
    'url': 'Скриншот диалога',
    'fielddate': 'Дата запроса',
    'uuid': 'id пользователя',
    'context': 'Контекст запроса',
    'reply': 'Оцениваемый ответ Алисы',
    'action': 'Событие последнего ответа Алисы',
    'app': 'Приложение пользователя',
    'intent': 'Интент последнего ответа Алисы',
    'req_id': 'request_id запроса'
}

var TH_DISPLAY = {
    'url': 'screenshot'
}

var SAMPLE_TABLE = d3.select('table#query-result');
var HIDDEN_FIELDS = ['context_0', 'context_1', 'context_2'];
var MAX_COLUMN_LENGTH = 60;

function split_into_mupltiple_lines(string) {
    var res = '';
    for (i = 0; i < string.length; i += MAX_COLUMN_LENGTH) {
        res += string.slice(i, i + MAX_COLUMN_LENGTH);
        res += '\n';
    }
    return res;
}

var FIELD_CONV = {
    'intent': function (row) {
        return stripIntent(row['intent']);
    },

    'app': function (row) {
        return SELECTOR_NAMES['app'][row['app']];
    },

    'context': function (row) {
        var context = '';
        if (row['context_2'] || row['context_1']) {
            context += ('<span class="text-muted">Пользователь: </span>' + row['context_2'] || '—');
            context += ('\n<span class="text-muted">Алиса: </span>' + row['context_1'] || '—');
            context += '\n';
        }
        context += ('<span class="text-muted">Пользователь: </span>' + row['context_0'] || '—');
        return context;
    },

    'reply': function (row) {
        return split_into_mupltiple_lines(row['reply']);
    },

    'action': function (row) {
        return split_into_mupltiple_lines(row['action']);
    },

    'url': function(row) {
        return '<img class="screenshot" src="' + row['url'] + '">';
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
    columns.splice(3, 0, 'context');

    var th = table.select("thead tr").selectAll("th").data(columns);
    th.enter().append("th");
    th.exit().remove();
    th.attr("title", function(d) {return TH_HINTS[d]});
    th.text(function(d){
        if (d in TH_DISPLAY) {
            return TH_DISPLAY[d]
        } else {
            return d
        }
    });

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
}

function qsToSelectors() {
    var params = new URLSearchParams(window.location.search);
    for (var el of params.entries()) {
        var id = el[0];
        var val = el[1];
        var $selector = $('#selectors [name=' + id + ']');
        $selector.val(val);
    }
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

    var params = new URLSearchParams(window.location.search);
    $.ajax('/toloka-quality/query?' + params.toString(), {
        'success': function (data) {
            $waiter.hide();
            $submitter.prop( "disabled", false);
            updateSampleTable(SAMPLE_TABLE, data);
        },
        'error': function (request, status, err) {
            $waiter.hide();
            $submitter.prop( "disabled", false);
            if (request.status === 400) {
                showErrors(JSON.parse(request.responseText));
            } else {
                var msg = "Не удалось загрузить выборку плохих ответов из разметки толокеров.<br/>" +
                    request.status + ' / ' + err + '<br/>'
                    + request.responseText;
                showErrors({'all': [msg]});
            }
        }
    });
}

fillSelector('app');
qsToSelectors();

if (window.location.search) {
    loadSample();
}
