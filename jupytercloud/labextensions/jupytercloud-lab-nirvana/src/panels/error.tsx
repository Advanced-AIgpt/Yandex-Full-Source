import React from 'react';

import { Card, Button } from 'react-bootstrap';

import { NirvanaPanel } from './base';

import { IResponseParseResult } from '../requester';
import { IExportResult } from '../export/base';

export type TRequestError = IResponseParseResult | IExportResult | string;

interface IErrorPanel {
    error: TRequestError;
    tryAgain: () => void;
}

export const ErrorPanel = ({ error, tryAgain }: IErrorPanel) => {
    let header = null;
    let body = error;

    if (error.hasOwnProperty('status')) {
        const requestError = error as IResponseParseResult;

        header = (
            <Card.Title>
                {requestError.status}: {requestError.statusText}
            </Card.Title>
        );
        body = requestError.json
            ? JSON.stringify(requestError.json, null, '  ')
            : requestError.text;
    } else if (error.hasOwnProperty('reason')) {
        const exportError = error as IExportResult;
        header = (
            <Card.Title>
                Nirvana returned an error while export attempt
            </Card.Title>
        );
        body = exportError.reason;
    }

    return (
        <NirvanaPanel title="Unexpected error" border="danger">
            {header}
            <pre>{body}</pre>
            <p>
                <Button onClick={tryAgain}>Start over</Button>
            </p>
        </NirvanaPanel>
    );
};
