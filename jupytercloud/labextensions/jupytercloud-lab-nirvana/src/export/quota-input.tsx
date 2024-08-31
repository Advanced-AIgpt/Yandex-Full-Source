import React from 'react';

import { Form } from 'react-bootstrap';

interface IQuotaInputProps {
    quota: string;
}

export const QuotaInput = ({quota}: IQuotaInputProps) => {
    return (
        <Form.Group controlId="quotaInput">
            <Form.Label>Select quota</Form.Label>
            <Form.Control
                as="select"
                aria-describedby="quotaInputHelp"
                disabled
            >
                <option>{quota}</option>
            </Form.Control>
            <Form.Text id="quotaInputHelp" muted>
                Quota selection disabled in beta-version
            </Form.Text>
        </Form.Group>
    );
};
