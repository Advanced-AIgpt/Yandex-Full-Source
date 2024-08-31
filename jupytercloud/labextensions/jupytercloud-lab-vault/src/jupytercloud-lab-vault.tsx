import Cookies from 'universal-cookie';

import { URLExt } from '@jupyterlab/coreutils';
import { INotebookTracker, NotebookActions } from '@jupyterlab/notebook';
import { ServerConnection } from '@jupyterlab/services';
import { IObservableJSON } from '@jupyterlab/observables';
import { getVaultSecrets, setVaultSecrets, IYandexMetrika } from 'jupytercloud-lab-lib';

import {
    ISecretResult,
    showVaultSecrets,
    showSecretErrorDialog
} from './table'

const requestSecretTokens = (metadata: IObservableJSON, clickResult: any) => {
    setVaultSecrets(metadata, clickResult.value);

    const cookies = new Cookies(document.cookie);
    const xsrfCookie = cookies.get('_xsrf');

    const settings = ServerConnection.makeSettings();

    const tokenRequests = getVaultSecrets(metadata).map((item) => {
        const requestUrl = URLExt.join(
            settings.baseUrl,
            'vault/token', // API Namespace
            item.uuid
        );
        const method = 'POST';
        const body = new FormData();
        body.append('_xsrf', xsrfCookie);

        return fetch(new Request(
            requestUrl,
            { method, body }
        ));
    });

    return Promise.allSettled(tokenRequests);
}

const editSecretCell = (secretResults: ISecretResult[], notebookTracker: INotebookTracker) => {
    if (!secretResults.every(result => result.ok)) {
        showSecretErrorDialog(secretResults);
    }

    const notebook = notebookTracker.currentWidget.content;

    //@ts-ignore
    let cellsWalked = 0;
    let cellFound = false;

    do {
        NotebookActions.selectAbove(notebook);
        ++cellsWalked;

        const cellMetadata = notebook.activeCell.model.metadata || new Map();
        const tags: string[] = cellMetadata.get('tags') || [];

        if (tags.includes('jupytercloud_vault')) {
            cellFound = true;
            break;
        }
    } while (notebook.activeCellIndex !== 0);

    if (!cellFound) {
        NotebookActions.insertAbove(notebook);

        const cellMetadata = notebook.activeCell.model.metadata || new Map();
        const tags: string[] = cellMetadata.get('tags') || [];

        tags.push('jupytercloud_vault');
        cellMetadata.set('tags', tags);
        // notebook.activeCell.model.metadata = cellMetadata;
        ++cellsWalked;
    }

    notebook.activeCell.model.value.text = generateSecretCellText(secretResults);

    // FIXME: make it run
    // NotebookActions.run(notebook).then(runResult => {
    //     console.log("Run ", runResult);
    //     for (let i=0; i < cellsWalked; ++i) {
    //         NotebookActions.selectBelow(notebook);
    //     }
    // });
}

const generateSecretCellText = (secretResults: ISecretResult[]) => {
    const preamble = 'from jupytercloud.library import get_secret\n';
    const secrets = secretResults
        .filter(x => x.ok)
        .map(x => `${x.secret.name} = get_secret("${x.secret.uuid}")`)
        .join('\n');

    return preamble + secrets;
};

export const manageSecrets = (notebookTracker: INotebookTracker, yandexMetrika: IYandexMetrika) => {
    let panel = notebookTracker.currentWidget;
    let metadata: IObservableJSON = panel.content.model.metadata;

    if (yandexMetrika !== null) {
        yandexMetrika.reachGoal({ target: 'vault_manage:enter' });
    }

    showVaultSecrets(getVaultSecrets(metadata))
        .then((clickResult: any) => {
            if (!clickResult.button.accept) {
                return Promise.reject('cancelled');
            }

            return requestSecretTokens(metadata, clickResult);
        })
        .then((values) => {
            const secrets = getVaultSecrets(metadata);

            return Promise.allSettled(values.map((value, index) => {
                if (value.status === 'fulfilled') {
                    return Promise.allSettled(
                        [
                            secrets[index],
                            (value.value as Response).ok,
                            (value.value as Response).text()
                        ]
                    );
                } else {
                    return Promise.allSettled(
                        [
                            secrets[index],
                            false,
                            value.reason
                        ]
                    );
                }
            }));
        })
        .then((values) => {
            // unpacks promises to a more handy form
            return values.map((secret, index) => {
                if (secret.status === 'fulfilled') {
                    const unPromise = secret.value;

                    // this is needed to please TypeScript type checks
                    if (
                        unPromise[0].status === 'fulfilled' &&
                        unPromise[1].status === 'fulfilled' &&
                        unPromise[2].status === 'fulfilled'
                    ) {
                        return {
                            secret: unPromise[0].value,
                            ok: unPromise[1].value,
                            reason: unPromise[2].value
                        };
                    }
                }
            });
        })
        .then(secretResults => {
            editSecretCell(secretResults, notebookTracker);
        })
        .catch((error: any) => {
            console.error(error);
        });
};
