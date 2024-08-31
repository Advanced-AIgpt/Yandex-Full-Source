import React from 'react';

import { Form } from 'react-bootstrap';

import { UUID } from '@lumino/coreutils';

import { IGraphInfo } from './graph-input';

export type TAddType =
    | 'add-to-current-draft'
    | 'clone-to-new-instance'
    | 'clone-to-new-workflow';

interface IAddType {
    label: string;
    name: TAddType;
}

const ADD_TYPES: IAddType[] = [
    {
        label: 'Add to the current draft',
        name: 'add-to-current-draft'
    },
    {
        label: 'Clone and add to a new instance',
        name: 'clone-to-new-instance'
    },
    {
        label: 'Clone and add to a new workflow',
        name: 'clone-to-new-workflow'
    }
];

interface IAddTypeChoiceProps {
    graphInfo: IGraphInfo;
    current: TAddType | null;
    setAddType: (addType: TAddType) => void;
}

export class AddTypeChoice extends React.Component<IAddTypeChoiceProps> {
    constructor(props: IAddTypeChoiceProps) {
        super(props);
    }

    get active() {
        const { graphInfo } = this.props;
        const active: Set<TAddType> = new Set();

        if (graphInfo) {
            active.add('clone-to-new-workflow');
        } else {
            return active;
        }

        if (graphInfo.instanceCloneable) {
            active.add('clone-to-new-instance');
        }

        if (graphInfo.instanceModifiable) {
            active.add('add-to-current-draft');
        }

        return active;
    }

    componentDidUpdate = (oldProps: IAddTypeChoiceProps) => {
        const active = this.active;
        let { current } = this.props;

        if (current && active.has(current)) {
            return;
        }

        for (const { name } of ADD_TYPES) {
            if (this.active.has(name)) {
                current = name;
                break;
            }
        }

        if (current && current !== oldProps.current) {
            this.props.setAddType(current);
        }
    };

    onAddTypeChange = (event: React.ChangeEvent<HTMLInputElement>) => {
        const current = event.target.name as TAddType;
        this.props.setAddType(current);
    };

    render = () => {
        const { current } = this.props;

        const radios: React.ReactNode[] = ADD_TYPES.map((type_: IAddType) => {
            // id needed for labels work correctly with clicks
            const id = `add-type-${UUID.uuid4()}`;

            return (
                <Form.Check
                    type="radio"
                    {...type_}
                    disabled={!this.active.has(type_.name)}
                    checked={type_.name === current}
                    key={type_.name}
                    onChange={this.onAddTypeChange}
                    id={id}
                />
            );
        });

        return (
            <fieldset>
                <Form.Group>
                    <Form.Label>Choose action:</Form.Label>
                    {radios}
                </Form.Group>
            </fieldset>
        );
    };
}
