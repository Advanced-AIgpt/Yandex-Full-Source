import React from 'react';

import { Spinner } from 'react-bootstrap';

import { NirvanaPanel } from './base';

interface IPlaceholderPanel {
    title: string;
}

export const PlaceholderPanel = (props: IPlaceholderPanel) => {
    return (
        <NirvanaPanel title={props.title}>
            <div className="text-center">
                <Spinner animation="border" role="status">
                    <span className="sr-only">Loading...</span>
                </Spinner>
            </div>
        </NirvanaPanel>
    );
};
