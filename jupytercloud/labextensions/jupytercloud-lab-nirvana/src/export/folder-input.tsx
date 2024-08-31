import React from 'react';

import { Form } from 'react-bootstrap';

interface IFolderInputProps {
    folderId: string;
    folderTitle: string;
}

export const FolderInput = ({folderId, folderTitle}: IFolderInputProps) => {
    return (
        <Form.Group controlId="folderInput">
            <Form.Label>Select Nirvana folder</Form.Label>
            <Form.Control
                as="select"
                aria-describedby="folderInputHelp"
                disabled
            >
                <option value={folderId}>{folderTitle}</option>
            </Form.Control>
            <Form.Text id="folderInputHelp" muted>
                Folder selection disabled in beta-version
            </Form.Text>
        </Form.Group>
    );
};
