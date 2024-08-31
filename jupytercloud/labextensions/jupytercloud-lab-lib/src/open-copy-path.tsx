import '../style/index.css';

import React, { FunctionComponent, useRef } from 'react';
import {
    InputGroup,
    FormControl,
    Button,
    OverlayTrigger,
    Tooltip
} from 'react-bootstrap';

type NotebookPathBlockProps = {
    path: string;
    url?: string | null;
    title?: string;
    copyButton?: boolean;
    openButton?: boolean;
};

export const OpenCopyPath: FunctionComponent<NotebookPathBlockProps> = (
    props: NotebookPathBlockProps
) => {
    const inputRef = useRef<HTMLInputElement>(null);
    const url = props.url || props.path;

    const copyOnClick = (event: React.MouseEvent<any, MouseEvent>) => {
        inputRef.current.focus();
        inputRef.current.select();
        document.execCommand('copy');
    };

    const delay = { show: 100, hide: 1000 };
    const copyButton = props.copyButton ? (
        <OverlayTrigger
            placement="top"
            delay={delay}
            overlay={<Tooltip id="button-copy-tooltip">Copied!</Tooltip>}
            trigger="click"
        >
            <Button
                variant="primary"
                bsPrefix="btn-jcll"
                className="jcll-addon-button"
                href="#"
                onClick={copyOnClick}
            >
                Copy
            </Button>
        </OverlayTrigger>
    ) : (
        ''
    );

    const openButton = props.openButton ? (
        <Button
            variant="primary"
            bsPrefix="btn-jcll"
            className="jcll-addon-button"
            href={url}
            target="_blank"
        >
            Open
        </Button>
    ) : (
        ''
    );

    return (
        <InputGroup className="mb-3">
            <InputGroup.Prepend>
                <InputGroup.Text
                    id="link-addon"
                    bsPrefix="jcll-input-group-text"
                >
                    {props.title}
                </InputGroup.Text>
            </InputGroup.Prepend>
            <FormControl
                value={props.path}
                ref={inputRef}
                aria-label="Link"
                aria-describedby="link-addon"
                type="text"
                className="jcll-input"
                readOnly
            />
            {copyButton}
            {openButton}
        </InputGroup>
    );
};

OpenCopyPath.defaultProps = {
    url: null,
    title: 'Link',
    copyButton: true,
    openButton: true
};
