import {
    JupyterFrontEnd,
    JupyterFrontEndPlugin
} from '@jupyterlab/application';

import { Widget } from '@lumino/widgets';

import { addYndxBug } from './yndxbug';

/**
 * Initialization data for the jupyterlab-yndxbug extension.
 */
const extension: JupyterFrontEndPlugin<void> = {
    id: 'jupyterlab-yndxbug',
    autoStart: true,
    activate: (app: JupyterFrontEnd) => {
        app.started.then(() => {
            const container = new Widget();
            container.id = 'yndxbug-containter';
            Widget.attach(container, document.body);
            addYndxBug(container.node, '', 'JupyterLab');
        });
    }
};

export default extension;
