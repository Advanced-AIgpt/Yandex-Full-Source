import React from 'react';

import { Container, Row, Button } from 'react-bootstrap';

import { OpenCopyPath } from 'jupytercloud-lab-lib';

import { IExportResult } from '../export/base';

import { NirvanaPanel } from './base';

interface ISuccessPanel {
    closeNirvanaExport: () => void;
    exportResult: IExportResult;
}

export const SuccessPanel = ({
    closeNirvanaExport,
    exportResult
}: ISuccessPanel) => {
    return (
        <NirvanaPanel title="Export is succeed">
            <Container>
                <Row>
                    <OpenCopyPath path={exportResult.url} />
                </Row>
                <Row>
                    <Button onClick={closeNirvanaExport}>Close</Button>
                </Row>
            </Container>
        </NirvanaPanel>
    );
};
