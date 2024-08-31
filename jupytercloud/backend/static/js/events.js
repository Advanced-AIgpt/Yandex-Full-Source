require(["jquery", "jhapi"], function(
  $,
  JHAPI,
) {
    "use strict";

    const events_url = '/hub/api/events';
    const events_panel = $('#events-panel');
    const events_panel_body = $('#events-panel-body');

    $.get(events_url)
        .always(() => {
            $('#events-load-placeholder').hide();
        })
        .fail((jqXHR) => {
            $('<h4/>')
                .text('Fail to fetch info about upcoming events, please contact us')
                .addClass('text-danger text-center')
                .appendTo(events_panel_body);

            console.error('Fail to fetch events info, response:', jqXHR);
        })
        .done((data) => {
            if (data.events.length == 0) {
                $('<h4/>')
                    .text('No events planned in next two weeks.')
                    .addClass('text-muted text-center')
                    .appendTo(events_panel_body);

                return;
            }

            events_panel_body.hide();
            events_panel.removeClass('panel panel-default');

            data.events.forEach((info) => {
                let group = $('<div/>')
                    .addClass('list-group')
                    .appendTo(events_panel);

                $('<div/>')
                    .addClass('list-group-item')
                    .addClass(
                        info.ongoing ? 'list-group-item-danger' : 'list-group-item-warning'
                    )
                    .html(info.message.replace('\n', '<br/>'))
                    .appendTo(group);

                $('<a/>')
                    .addClass('list-group-item')
                    .text('Details')
                    .attr('href', info.event_url)
                    .attr('target', '_blank')
                    .appendTo(group);

                if (info.migration_doc_url) {
                    $('<a/>')
                        .addClass('list-group-item')
                        .text('Migragion docs')
                        .attr('href', info.migration_doc_url)
                        .appendTo(group);
                }
            });
        });

});
