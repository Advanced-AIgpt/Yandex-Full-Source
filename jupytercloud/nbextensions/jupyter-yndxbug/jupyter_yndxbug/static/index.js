define(['jquery'], function($) {
    const form_id = '10016227';
    const bug_src = 'https://yastatic.net/s3/frontend/butterfly/v0.146.0/butterfly.js';

    function safe_path_get(source, path, default_) {
        let val = source;

        for (const prop of path) {
            if (val === null || typeof val === 'undefined') {
                return default_;
            }

            val = val[prop];
        }

        return typeof val === 'undefined' ? default_ : val;
    }

    function insert_yndxbug() {
        return $('<script/>')
            .attr({
              'src': bug_src,
              'id': 'yndxbug',
              'position': 'left',
              'modules': 'forms,info',
              'form': form_id,
              'data-text': '',
              'custom-theme': '',
              'screenshot': 'true',
              'data-domain': 'yandex',
              'async': '',
              'data-data': '',
              'data-info': '',
              'crossorigin': ''
            })
            .appendTo($('body'));
    }

    function collect_debug_info() {
        let notebook = requirejs('base/js/namespace').notebook;

        let result = {};

        result.notebook_name = safe_path_get(notebook, ['notebook_name'], 'N/A');
        result.notebook_path = safe_path_get(notebook, ['notebook_path'], 'N/A');
        result.writable = safe_path_get(notebook, ['writable'], 'N/A');
        result.nbformat = safe_path_get(notebook, ['nbformat'], 'N/A');
        result.trusted = safe_path_get(notebook, ['trusted'], 'N/A');
        result.kernelspec = safe_path_get(notebook, ['metadata', 'kernelspec'], 'N/A');
        result.extensions = safe_path_get(notebook, ['config', 'data', 'load_extensions'], 'N/A');
        result.kernel_busy = safe_path_get(notebook, ['kernel_busy'], 'N/A');
        result.kernel_name = safe_path_get(result, ['kernelspec', 'display_name'], 'Unknown');

        // Страшновато, вдруг там токены будут.
        // result.tracebacks = collect_tracebacks(notebook);

        return result;
    }

    function try_set_bug_onclick(script) {
        // Жучок появляется после вставки тега скрипт, но когда-то потом.
        // Поэтому делаем здесь такой поллинг.
        let yndxbug = $('.YndxBug');
        let children = yndxbug.children();

        if (
            yndxbug.length > 0 &&
            children.length > 0 &&
            children.children().length > 0

        ) {
            // На div-е жучка есть onclick.
            // Также он забирает данные в момент открытия жучка.
            // Тут мы вешаем обработчик на svg-шку с жучком, которая сетапит
            // данные, которые будет забирать жучок при открытии.
            // Т.к. svg-шка - ребенок div-а, обработчик на ней сработает
            // раньше.
            let bug_svg = children.children();

            bug_svg.click(() => {
                let debug_info = collect_debug_info();
                let serialized_debug_info = JSON.stringify(debug_info, null, 4);
                script[0].dataset['info'] = serialized_debug_info;
                script[0].dataset['data'] = serialized_debug_info;
                script[0].dataset['text'] = `Notebook (kernel ${debug_info.kernel_name})`;
            });
        } else {
            setTimeout(try_set_bug_onclick, 100, script);
        }
    }

    function load_ipython_extension(){
        let is_notebook_page = null;
        try {
            requirejs('notebook/js/notebook').Notebook;
            is_notebook_page = true;
        } catch (exception) {
            is_notebook_page = false;
        }

        let script = insert_yndxbug();

        if (is_notebook_page) {
            // В случае, если мы на странице ноутбука,
            // мы хотим собирать дополнительные данные.
            try_set_bug_onclick(script);
        }
    }

    return {
        load_ipython_extension: load_ipython_extension
    };
});

