require(["jquery"], function ($) {
    $("#refresh").click(function () {
        window.location.reload();
    });

    // hook up event-stream for progress
    var evtSource = new EventSource(PROGRESS_URL);
    var progressMessage = $("#progress-message");
    var progressBar = $("#progress-bar");
    var srProgress = $("#sr-progress");
    var progressLog = $("#progress-log");

    evtSource.onmessage = function(e) {
        var evt = JSON.parse(e.data);
        console.log(evt);
        if (evt.progress !== undefined) {
            // update progress
            var progText = evt.progress.toString();
            progressBar.attr('aria-valuenow', progText);
            srProgress.text(progText + '%');
            progressBar.css('width', progText + '%');
        }
        // update message
        var html_message;
        if (evt.html_message !== undefined) {
            progressMessage.html(evt.html_message);
            html_message = evt.html_message;
        } else if (evt.message !== undefined) {
            progressMessage.text(evt.message);
            html_message = progressMessage.html();
        }
        if (html_message) {
            progressLog.append(
                $("<div>")
                    .addClass('progress-log-event')
                    .html(html_message)
            );
        }

        if (evt.ready) {
            evtSource.close();
            window.location.replace(SETTINGS_URL);
        }

        if (evt.failed) {
            evtSource.close();
            // turn progress bar red
            progressBar.addClass('progress-bar-danger');
            // open event log for debugging
            $('#progress-details').prop('open', true);
        }
    };

});
