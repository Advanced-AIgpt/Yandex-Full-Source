import React from "react";

import { Form } from "react-bootstrap";
import { Pencil, XCircle, ExclamationCircleFill } from "react-bootstrap-icons";

import { OverlayBootstrapIcon, OverlaySpinner } from "../overlayIcon";

import { PropertyRow } from "./propertyRow";

interface EditablePropertyRowProps {
    title: string;
    value: string;
    onChange: (value: string) => Promise<void>;
}

enum EditStatus {
    NoEdit = 1,
    Editing,
    Applying,
    Error,
}

interface iconFuncArgs {
    msg?: string;
    callback?: () => void;
}
type iconFunc = (args: iconFuncArgs) => React.ReactElement;

const StatusIcons: { [status in EditStatus]: iconFunc } = {
    [EditStatus.NoEdit]: function noEditIcon() {
        return <OverlayBootstrapIcon title="Edit" icon={Pencil} />;
    },
    [EditStatus.Editing]: function editingIcon({ callback }) {
        return <OverlayBootstrapIcon title="Cancel" icon={XCircle} onClick={callback} />;
    },
    [EditStatus.Applying]: function applyingIcon() {
        return <OverlaySpinner title="Applying..." />;
    },
    [EditStatus.Error]: function errorIcon({ msg }) {
        return <OverlayBootstrapIcon title={msg} icon={ExclamationCircleFill} />;
    },
};

export const EditablePropertyRow: React.VFC<EditablePropertyRowProps> = (props) => {
    const [value, setValue] = React.useState<string>();
    const [status, setStatus] = React.useState<EditStatus>(EditStatus.NoEdit);
    const [message, setMessage] = React.useState<string>();

    React.useEffect(() => {
        setValue(props.value);
    }, [props.value]);

    const cancelEdit = (): void => {
        setStatus(EditStatus.NoEdit);
        setValue(props.value);
    };

    const startEdit = (): void => {
        setStatus(EditStatus.Editing);
    };

    const applyEdit = (): void => {
        if (value === props.value) {
            setStatus(EditStatus.NoEdit);
            return;
        }

        setStatus(EditStatus.Applying);

        props
            .onChange(value)
            .then(() => {
                setStatus(EditStatus.NoEdit);
            })
            .catch((error: any) => {
                const text = (error.json && error.json.message) || error.text || error.toString();
                setStatus(EditStatus.Error);
                setMessage(text);
            });
    };

    const onKeyDown = (event: React.KeyboardEvent<HTMLInputElement>): void => {
        if (event.key === "Enter") {
            applyEdit();
        } else if (event.key === "Escape") {
            cancelEdit();
        }
    };

    const icon = StatusIcons[status]({ msg: message, callback: cancelEdit });

    return (
        <PropertyRow title={props.title} statusIcon={icon} onClick={startEdit}>
            {status === EditStatus.NoEdit || status == EditStatus.Error ? (
                value
            ) : (
                <Form.Control
                    autoFocus
                    type="text"
                    disabled={status === EditStatus.Applying}
                    value={value}
                    onBlur={applyEdit}
                    onKeyDown={onKeyDown}
                    className="editable-property-input"
                    onChange={(event) => setValue(event.target.value)}
                />
            )}
        </PropertyRow>
    );
};
