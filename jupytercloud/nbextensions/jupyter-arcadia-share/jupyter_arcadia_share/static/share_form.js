define(['base/js/dialog', 'jquery'], function(dialog, $) {
    const PLACEHOLDER_URL = "https://cdnjs.cloudflare.com/ajax/libs/slick-carousel/1.5.8/ajax-loader.gif";

    function create_button(text, cls, keep_form) {
        var button = $("<button/>")
            .addClass("btn btn-default btn-sm")
            .text(text);

        if (!keep_form) {
            button = button.attr("data-dismiss", "modal");
        }

        if (cls) {
            button.addClass(cls);
        }

        return button;
    }

    function create_load_placeholder(title) {
        return $('<p/>')
            .text(title)
            .append(
                $('<img/>').attr('src', PLACEHOLDER_URL)
            );
    }

    class ShareForm {
        constructor(title, env, upload_url, filename) {
            this.title = title;
            this.env = env;
            this.upload_url = upload_url;
            this.filename = filename;

            this.body = $('<div/>').addClass('text-center');
            this.buttons_group = null;
            this.input = null;
        }

        show() {
            create_load_placeholder('Verifying upload possibility...')
                .appendTo(this.body);

            dialog.modal({
                body: this.body,
                title: this.title,
                open: this.draw_upload_form.bind(this),
                buttons: {},
                notebook: this.env.notebook,
                keyboard_manager: this.env.notebook.keyboard_manager,
            });
        }

        draw_upload_form() {
            this.buttons_group = $('.modal-footer');
            $.get(this.upload_url)
                .always(this.empty.bind(this))
                .fail(this.draw_fail_form.bind(this))
                .done(this.check_upload_callback.bind(this));
        }

        draw_fail_form(jqXHR) {
            var level = jqXHR.status == 400 ? 'warning' : 'danger';

            create_button('OK', `btn-${level}`)
                .appendTo(this.buttons_group)
                .focus();

            var error_text = $('<div/>')
                .addClass(`alert alert-${level}`)
                .attr('role', 'alert')
                .appendTo(this.body);

            $('<p/>')
                .text(`JupyterHub request to ${this.upload_url} failed with error:`)
                .appendTo(error_text);

            $('<h3/>')
                .text(`${jqXHR.status}: ${jqXHR.statusText}`)
                .appendTo(error_text);

            $('<pre/>')
                .text(jqXHR.responseJSON.message || jqXHR.responseText)
                .appendTo(error_text);
        }

        check_upload_callback(data) {
            if (data.result) {
                this.draw_commit_form(data);
            } else {
                this.draw_impossible_form(data);
            }
        }

        draw_commit_form(data) {
            var form_group = $('<div/>')
                .addClass('form-group')
                .appendTo(this.body);

            $('<label/>')
                .text("Please enter commit message. Only this notebook will be uploaded.")
                .attr('for', 'commit-message')
                .appendTo(form_group);

            this.input = $('<textarea/>')
                .addClass("form-control")
                .attr({
                    'rows': 2,
                    'placeholder': 'Commit message',
                    'id': 'commit-message',
                })
                .appendTo(form_group)
                .focus();

            create_button('Cancel').appendTo(this.buttons_group);

            if (!data.exists) {
                create_button('Upload', 'btn-primary', true)
                    .click(function () {this.upload_file(this.filename)}.bind(this))
                    .appendTo(this.buttons_group);

                return;
            }

            $('<p/>')
                .addClass('alert alert-warning')
                .attr('role', 'alert')
                .html(
                    `File already exists in <a href="${data.link}" target="_blank">Arcadia</a>`
                )
                .appendTo(this.body);

            if (data.additional_choices && data.additional_choices.length > 0) {
                var alternative = data.additional_choices[0];
                var short_alternative = alternative.split('/').slice(-1)[0];

                create_button(`Upload as ${short_alternative}`, 'btn-primary', true)
                    .click(function () {this.upload_file(alternative)}.bind(this))
                    .appendTo(this.buttons_group);
            }

            var rewrite_button = create_button('Upload and rewrite', 'btn-warning', true)
                .click(function () {this.upload_file(this.filename)}.bind(this))
                .appendTo(this.buttons_group)
                .focus();
        }

        draw_impossible_form(data) {
            create_button('OK', 'btn-warning')
                .appendTo(this.buttons_group)
                .focus();

            var error_text = $('<div/>')
                .addClass('alert alert-warning')
                .attr('role', 'alert')
                .appendTo(this.body);

            $('<h4/>')
                .text("Can't upload notebook to Arcadia due to:")
                .appendTo(error_text);

            $('<p/>')
                .text(data.message)
                .appendTo(error_text);
        }

        draw_success_form(data) {
            create_button('OK', 'btn-primary')
                .appendTo(this.buttons_group)
                .focus();

            $('<label/>')
                .addClass('alert alert-success')
                .attr({
                    'role': 'alert',
                    'for': 'link-box',
                })
                .text("Notebook successfully uploaded")
                .appendTo(this.body);

            var link_box = $('<div/>')
                .addClass('input-group')
                .appendTo(this.body);

            $('<span/>')
                .addClass('input-group-addon')
                .attr('id', 'link-box-addon')
                .text('Link')
                .appendTo(link_box);

            var link_input = $('<input/>')
                .addClass('form-control')
                .attr({
                    'aria-describedby': 'link-box-addon',
                    'type': 'text',
                    'value': data.link,
                    'readonly': true,
                })
                .appendTo(link_box);

            var input_group_btn = $('<span/>')
                .addClass('input-group-btn')
                .appendTo(link_box);

            $('<button/>')
                .addClass('btn btn-default')
                .attr('type', 'button')
                .text('Copy')
                .click(function () {
                    const current_focus = document.activeElement;
                    const target = link_input[0];

                    target.focus();
                    target.setSelectionRange(0, target.value.length);
                    document.execCommand("copy");
                    if (current_focus && typeof current_focus.focus === "function") {
                        current_focus.focus()
                    }
                })
                .appendTo(input_group_btn);

            $('<a/>')
                .addClass('btn btn-default')
                .attr({
                    'role': 'button',
                    'href': data.link,
                    'target': '_blank',
                })
                .text('Open')
                .appendTo(input_group_btn);
        }

        upload_file(filename) {
            this.empty();

            create_load_placeholder('Uploading notebook to Arcadia...')
                .appendTo(this.body);

            var data = {path: filename};
            var message = this.input.val();
            if (message) {
                data.message = message;
            }

            $.post({
                url: this.upload_url,
                data: JSON.stringify(data),
                dataType: 'json',
                xhrFields: {
                    withCredentials: true
                },
            })
                .always(this.empty.bind(this))
                .fail(this.draw_fail_form.bind(this))
                .done(this.draw_success_form.bind(this));
        }

        empty() {
            this.buttons_group.empty();
            this.body.empty();
        }

    }

    return {
        ShareForm: ShareForm
    }
})
