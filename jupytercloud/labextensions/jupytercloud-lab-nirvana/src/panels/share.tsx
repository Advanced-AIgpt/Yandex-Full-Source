import React from 'react';

import { Alert, Button, ButtonToolbar } from 'react-bootstrap';

import moment from 'moment';

import { IShareHistoryEntry, OpenCopyPath } from 'jupytercloud-lab-lib';

import { NirvanaPanel } from './base';

interface ISharePanel {
    doShare: () => void;
    skipShare: () => void;
    lastShareEntry: IShareHistoryEntry;
}

export const SharePanel = ({
    doShare,
    skipShare,
    lastShareEntry
}: ISharePanel) => {
    const buttons = [
        <Button className="mr-2" key="doShare" onClick={doShare}>
            Share notebook to Arcadia
        </Button>
    ];

    let lastShareBlock = null;
    if (lastShareEntry) {
        buttons.push(
            <Button className="mr-2" key="useLast" onClick={skipShare}>
                Use last share
            </Button>
        );

        const date = moment(lastShareEntry.timestamp);

        lastShareBlock = (
            <>
                <p>
                    Last time this notebook was shared {date.fromNow()} with
                    next path:
                </p>
                <OpenCopyPath path={lastShareEntry.link} />
            </>
        );
    }

    return (
        <NirvanaPanel title="Share to Arcadia">
            <Alert variant="info">
                Notebook must be shared to Arcadia to use in JupyterCloud
                nirvana graph.
            </Alert>
            {lastShareBlock}
            <ButtonToolbar className="justify-content-end">
                {buttons}
            </ButtonToolbar>
        </NirvanaPanel>
    );
};
