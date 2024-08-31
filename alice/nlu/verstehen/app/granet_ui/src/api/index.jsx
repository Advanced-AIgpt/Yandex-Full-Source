const $ = require('jquery');

export function sendSearch(app, index, positive_query, negative_query=[], opts={}, n_samples=50) {
    let query = {
        positive: positive_query,
        negative: negative_query
    }
    let filters = undefined;

    if (opts['filters']) {
        filters = opts['filters']
    }
    query = {...query, ...opts}
    console.log('Check sendSearch query:');
    console.log(query);

    return $.ajax({
        url: '/search',
        dataType: "json",
        type: 'POST',
        headers: {'Content-Type': 'application/json'},
        data: JSON.stringify({
            query: query,
            app_name: app,
            index_name: index,
            n_samples: n_samples,
            filters: filters
        }),
        timeout: 30000
    });
}

export function sendEstimate(app, index, positive_query, negative_query=[], opts={}) {
    let query = {
        positive: positive_query,
        negative: negative_query
    }
    let filters = []

    if (opts['filters']) {
        filters = opts['filters']
    }

    return $.ajax({
        url: '/estimate',
        dataType: "json",
        type: 'POST',
        headers: {'Content-Type': 'application/json'},
        data: JSON.stringify({
            query: query,
            app_name: app,
            index_name: index,
            filters: filters
        }),
        timeout: 30000
    });
}

export function fetchApps() {
    return  $.ajax({
        url: '/apps',
        timeout: 10000
    });
}

// downloading a file from the web page
export function downloadFileFromText(content, fileName, contentType) {
    let a = document.createElement('a');
    let file = new Blob([content], {type: contentType});
    a.href = URL.createObjectURL(file);
    a.download = fileName;
    a.dispatchEvent(new MouseEvent('click'));  // not a.click(); since it doesn't work for FireFox
}
