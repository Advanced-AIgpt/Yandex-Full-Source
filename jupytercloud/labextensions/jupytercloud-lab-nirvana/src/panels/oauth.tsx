import React from 'react';

import { Alert, Card } from 'react-bootstrap';

import { NirvanaPanel } from './base';

interface IOauthPanel {
    approveOAuth: () => void;
}

export const OAuthPanel = ({ approveOAuth }: IOauthPanel) => {
    return (
        <NirvanaPanel title="Approving OAuth access">
            <Alert variant="info">
                <p>
                    You need to approve OAuth access for exporting notebook to
                    Nirvana
                </p>
                <p>
                    If OAuth window did not popup,{' '}
                    <Card.Link onClick={approveOAuth}>
                        open it maually
                    </Card.Link>
                </p>
            </Alert>
        </NirvanaPanel>
    );
};

export const OAuthErrorPanel = ({ approveOAuth }: IOauthPanel) => {
    return (
        <NirvanaPanel title="Error while approving OAuth access">
            <Alert variant="danger">
                <p>While approving OAuth access something happend.</p>
                <p>
                    You can try again by clicking{' '}
                    <Card.Link onClick={approveOAuth}>this link</Card.Link>.
                </p>
                <p>Check browser logs to see what goes wrong.</p>
            </Alert>
        </NirvanaPanel>
    );
};
