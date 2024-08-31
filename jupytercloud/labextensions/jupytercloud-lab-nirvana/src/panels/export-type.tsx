import React from 'react';

import { ListGroup, Button } from 'react-bootstrap';

import { slugify } from 'jupytercloud-lab-lib';
import { NirvanaPanel } from './base';

interface IExportAction {
    title: string;
    action: () => void;
}

interface IExportTypePanel {
    doBack: () => void;
    actions: IExportAction[];
}

export const ExportTypePanel = ({ doBack, actions }: IExportTypePanel) => {
    return (
        <NirvanaPanel title="Choose export type" bodyWrap={false}>
            <ListGroup variant="flush">
                {actions.map((action: IExportAction) => {
                    return (
                        <ListGroup.Item action onClick={action.action} key={slugify(action.title)}>
                            {action.title}
                        </ListGroup.Item>
                    );
                })}
                <ListGroup.Item>
                    <Button className="mr-2" onClick={doBack}>
                        Back to previous menu
                    </Button>
                </ListGroup.Item>
            </ListGroup>
        </NirvanaPanel>
    );
};
