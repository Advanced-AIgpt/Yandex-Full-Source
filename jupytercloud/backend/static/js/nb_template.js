require(["jquery", "jhapi"], function(
  $,
  JHAPI,
) {
    "use strict";

    const LS_INTERFACE_KEY = '@jupytercloud/preferred-interface';
    const MINIMUM_PREVIEW_HEIGHT = 100;

    const $interfaceButtons = $('#interface-group').children();
    const $templateButtons = $('#template-group').children();
    const $deployButton = $('#deploy-button');
    const $tabs = $('[role="tabpanel"]');

    function adjustRightPane() {
        const $preview = $tabs
            .filter('.active')
            .find('.preview');

        const $previewBody = $preview
            .find('.panel-body')
            .css({overflow: 'auto'});

        const previewMargin = (
            $preview.outerHeight(true)  // with margins
            - $preview[0].clientHeight // without margins and border
        );
        const topOffset = $previewBody.offset().top;

        let previewHeight = window.innerHeight - topOffset - previewMargin;
        if (previewHeight < MINIMUM_PREVIEW_HEIGHT) {
            previewHeight = MINIMUM_PREVIEW_HEIGHT;
        }

        $previewBody.outerHeight(previewHeight);  // it takes paddings into considiration
    }

    function getTemplateFromURL() {
        const url = new URL(window.location.href);

        return url.hash ? url.hash.substr(1) : $templateButtons.filter('.active').data('target');
    }

    function getInterfaceFromLocalStorage() {
        try {
            return localStorage.getItem(LS_INTERFACE_KEY);
        } catch (error) {
            console.error('failed to fetch info from local storage: ', error);
        }
        return $interfaceButtons.filter('.active').data('interface');
    }

    function updateDeployButton(selectedTemplateName, selectedInterfaceName) {
        selectedTemplateName = (
            selectedTemplateName
            || $templateButtons.filter('.active').data('target')
        );
        selectedInterfaceName = (
            selectedInterfaceName
            || $interfaceButtons.filter('.active').data('interface')
        );

        let url = new URL(window.location.href);

        url.pathname += '/' + selectedTemplateName;
        url.search += '&_jupytercloud_interface_=' + selectedInterfaceName;
        url.hash = '';

        $deployButton.off('click');
        $deployButton.click(() => {
            $deployButton.prop('disabled', true);
            $('#placeholder-dialog').modal();
            location.assign(url);
        });
    }

    $interfaceButtons.click((event_) => {
        const selectedInterfaceName = $interfaceButtons
            .removeClass('active')
            .filter(event_.target)
            .addClass('active')
            .data('interface');

        updateDeployButton(null, selectedInterfaceName);
        adjustRightPane();

        try {
            localStorage.setItem(LS_INTERFACE_KEY, selectedInterfaceName);
        } catch (error) {
            console.error('failed to set info to local storage: ', error);
        }
    });

    $templateButtons.click((event_) => {
        const selectedTemplateName = $templateButtons
            .removeClass('active')
            .filter(event_.target)
            .addClass('active')
            .data('target');

        $tabs
            .hide()
            .removeClass('active')
            .filter(`#tab-${selectedTemplateName}`)
            .addClass('active')
            .show();

        updateDeployButton(selectedTemplateName, null);
        adjustRightPane();
    });

    const defaultTemplateName = getTemplateFromURL();
    $templateButtons
        .filter(`[data-target="${defaultTemplateName}"]`)
        .click();

    const defaultInterfaceName = getInterfaceFromLocalStorage();
    $interfaceButtons
        .filter(`[data-interface="${defaultInterfaceName}"]`)
        .click();

    $(window).resize(adjustRightPane);
});
