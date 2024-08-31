import React from 'react';
import ReactDOM from 'react-dom';

import ym, { YMInitializer } from 'react-yandex-metrika';

import {
    JupyterFrontEnd,
    JupyterFrontEndPlugin
} from '@jupyterlab/application';

import { Widget } from '@lumino/widgets';

import {
    IYandexMetrika,
    IReachGoal,
    TUserParams,
    TSessionParams
} from 'jupytercloud-lab-lib';

const COUNTER = 64806643;
const EXTENSION_ID = 'jupytercloud-lab-metrika';

const extension: JupyterFrontEndPlugin<IYandexMetrika> = {
    id: EXTENSION_ID,
    autoStart: true,
    requires: [JupyterFrontEnd.IPaths],
    provides: IYandexMetrika,
    activate: (app: JupyterFrontEnd, paths: JupyterFrontEnd.IPaths): IYandexMetrika => {
        if (!paths.urls.hubUser) {
            console.warn(
                `JupyterLab extension ${EXTENSION_ID} is not activated due to lack of hubUser`
            );
            return;
        }

        app.started.then(() => {
            const container = new Widget();
            container.id = 'yndxmetrika-container';
            Widget.attach(container, document.body);

            const metrika = (
                <YMInitializer
                    accounts={[COUNTER]}
                    options={{ webvisor: true, clickmap: true }}
                />
            );
            ReactDOM.render(metrika, container.node);

            ym('setUserID', paths.urls.hubUser);
        });

        return {
            reachGoal: ({ target, sessionParams, callback, ctx }: IReachGoal) => ym(
                'reachGoal', target, sessionParams, callback, ctx
            ),
            setUserParams: (userParams: TUserParams) => ym('userParams', userParams),
            setSessionParams: (sessionParams: TSessionParams) => ym('params', sessionParams),
        }
    }
};

export default extension;
