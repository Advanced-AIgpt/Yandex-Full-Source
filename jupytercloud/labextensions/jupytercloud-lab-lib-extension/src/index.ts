import { Menu } from '@lumino/widgets';

import { URLExt } from '@jupyterlab/coreutils';
import {
    JupyterFrontEnd,
    JupyterFrontEndPlugin
} from '@jupyterlab/application';
import { IMainMenu } from '@jupyterlab/mainmenu';

import { IJupyterCloud } from 'jupytercloud-lab-lib';

const extension: JupyterFrontEndPlugin<IJupyterCloud> = {
    id: 'jupytercloud-lab-lib-extension',
    requires: [IMainMenu],
    optional: [JupyterFrontEnd.IPaths],
    provides: IJupyterCloud,
    autoStart: true,
    activate: (
        app: JupyterFrontEnd,
        mainMenu: IMainMenu,
        paths: JupyterFrontEnd.IPaths
    ) => {
        const menu = new Menu({ commands: app.commands });
        menu.title.label = 'JupyterCloud';
        mainMenu.addMenu(menu, { rank: 160 });

        const isHubAvailable = (extensionName: string): boolean => {
            if (!paths || !paths.urls.hubUser) {
                console.warn(
                    'JupyterLab extension ${EXTENSION_ID} is not activated due to lack of hubUser'
                );
                return false;
            }
            return true;
        };

        const getHubUrl = (...parts: string[]): string => {
            if (!paths || !paths.urls.hubPrefix) {
                throw new Error('failed to get hub url');
            }

            return (
                '/' +
                URLExt.encodeParts(URLExt.join(paths.urls.hubPrefix, ...parts))
            );
        };

        return {
            menu,
            isHubAvailable,
            getHubUrl
        };
    }
};

export default extension;
