define(['base/js/namespace', 'jquery', './share_form'], function(IPython, $, share_form) {
    const BUTTON_TEXT = 'Save and Share';
    const DIALOG_TITLE = 'Save and Upload notebook to Arcadia';

    function get_username(notebook) {
        var base_url = notebook.base_url;
        var url_parts = base_url.split('/')
        console.assert(
            url_parts.length > 2 && url_parts[1] == 'user',
            'failed to extract username from notebook base_url'
        )
        return url_parts[2];
    }

    function get_upload_url(user, notebook_path) {
        var url = new URL(location);
        url.pathname = `/hub/api/arcadia/upload/${user}/${notebook_path}`;
        return url.pathname
    }

    function handler(env) {
        const username = get_username(env.notebook);
        const filename = env.notebook.notebook_path;
        const upload_url = get_upload_url(username, filename);

        var form = new share_form.ShareForm(DIALOG_TITLE, env, upload_url, filename);

        env.notebook.save_notebook()
            .then(form.show.bind(form));
    }

    var action_config = {
        help: DIALOG_TITLE,
        label: 'Arcadia',
        icon: 'fa-upload',
        handler: handler,
        help_index : '',
    }

    function load_ipython_extension() {
        new share_form.ShareForm(123);

        var action_name = IPython.actions.register(action_config, 'share', 'arcadia');

        IPython.toolbar.add_buttons_group(
            [{
                label: BUTTON_TEXT,
                action: action_name,
            }],
            'arcadia',
        );

        var shortcuts = {
            'Ctrl-Shift-A': action_name,
        };

        IPython.keyboard_manager.edit_shortcuts.add_shortcuts(shortcuts);
        IPython.keyboard_manager.command_shortcuts.add_shortcuts(shortcuts);

        var menu_item = $('<li/>')
            .insertAfter($('#save_checkpoint'));

        $('<a/>')
            .attr('href', '#')
            .attr('title', DIALOG_TITLE)
            .appendTo(menu_item)
            .text(BUTTON_TEXT)
            .click(function () {IPython.actions.call(action_name)});
    }

    return {
        load_ipython_extension: load_ipython_extension
    };
});

