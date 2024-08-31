import { CheckBox, Spin } from 'lego-on-react';
import block from 'propmods';
import React, { FC, ReactNode, useState } from 'react';
import VerificationError from '../Settings/VerificationError';
import Caption from './Caption';
import './Form.scss';
import FormFieldContext from '../../context/formFieldContext';

const b = block('Form');

interface FormProps {
    error?: string;
    onSubmit?: (event: React.FormEvent<HTMLFormElement>) => void;
}

const Form: React.SFC<FormProps> = ({ children, error, onSubmit }) => (
    <form
        {...b({ invalid: !!error })}
        onSubmit={(event) => {
            if (onSubmit) {
                event.preventDefault();
                onSubmit(event);
                return false;
            }
        }}
    >
        {children}
    </form>
);

interface HeaderProps {
    title?: string;
    subtitle?: string | ReactNode;
    caption?: React.ReactNode;
}

const Header: React.FC<HeaderProps> = ({ title, subtitle, children, caption }) => (
    <header {...b('header', { 'with-caption': Boolean(caption) })}>
        <h2>{title || children}</h2>
        {subtitle && <p>{subtitle}</p>}
        {caption && <Caption>{caption}</Caption>}
    </header>
);

interface ExtendedHeaderProps {
    caption?: React.ReactNode;
    documentationUrl?: string;
}

const ExtendedHeader: React.FC<ExtendedHeaderProps> = ({ children, caption, documentationUrl }) => {
    return (
        <header {...b('extended-header')}>
            <h2>{children}</h2>
            {caption && (
                <Caption url={documentationUrl} wide>
                    {caption}
                </Caption>
            )}
        </header>
    );
};

export interface FieldProps {
    caption?: React.ReactNode;
    sideCaption?: React.ReactNode;
    description?: React.ReactElement<any>;
    documentationUrl?: string;
    error?: string;
    height?: number;
    label?: string | React.ReactNode;
    name?: string;
    progress?: boolean;
    required?: boolean;
    verificationError?: string;
    theme?: 'self-centered';
}

const Field: React.FC<FieldProps> = ({
    caption,
    children,
    documentationUrl,
    error,
    height,
    label,
    name,
    progress,
    required,
    description,
    verificationError,
    sideCaption,
    theme,
}) => {
    const [nonValidationError, setNonValidationError] = useState<React.ReactNode>('');
    const displayedError = error || nonValidationError;

    return (
        <FormFieldContext.Provider value={{ nonValidationError, setNonValidationError }}>
            <div
                {...b('field', {
                    invalid: Boolean(displayedError),
                    needToVerify: Boolean(verificationError),
                    name,
                    theme,
                })}
            >
                {label && (
                    <div {...b('label', { required })}>
                        <label htmlFor={name}>{label}</label>
                        {sideCaption ? <Caption offsetTop={false}>{sideCaption}</Caption> : null}
                    </div>
                )}
                <div {...b('fieldBody')} style={{ height }}>
                    {children}
                    {progress && <Spin size='xxs' progress />}
                    {displayedError && <div {...b('error')}>{displayedError}</div>}
                    {caption && <Caption url={documentationUrl}>{caption}</Caption>}
                    {verificationError && <VerificationError {...b('VerificationError')} text={verificationError} />}
                    {description}
                </div>
            </div>
        </FormFieldContext.Provider>
    );
};

interface SubfieldProps {
    error?: string;
    height?: number;
    progress?: boolean;
}

const Subfield: React.SFC<SubfieldProps> = ({ children, error, height, progress }) => {
    const [nonValidationError, setNonValidationError] = useState<React.ReactNode>('');
    const displayedError = error || nonValidationError;

    return (
        <FormFieldContext.Provider value={{ nonValidationError, setNonValidationError }}>
            <div {...b('subfield', { invalid: !!displayedError })} style={{ height }}>
                {children}
                {progress && <Spin size='xxs' progress />}
                {displayedError && <div {...b('error')}>{displayedError}</div>}
            </div>
        </FormFieldContext.Provider>
    );
};

interface CheckboxFieldProps {
    label?: string;
    name?: string;
    checked?: boolean;
    disabled?: boolean;
    onChange?: (event: React.KeyboardEvent<HTMLInputElement>) => void;
    caption?: React.ReactNode;
    documentationUrl?: string;
    multiline?: boolean;
}

const CheckboxField: FC<CheckboxFieldProps> = ({
    label,
    name,
    checked,
    disabled,
    onChange,
    children,
    caption,
    documentationUrl,
    multiline,
}) => {
    return (
        <Field
            label={label}
            caption={caption}
            documentationUrl={documentationUrl}
            name={name}
            theme={multiline ? 'self-centered' : undefined}
        >
            <label {...b('checkbox-wrapper', { multiline })}>
                <CheckBox
                    theme='normal'
                    size='m'
                    tone='default'
                    view='default'
                    name={name}
                    checked={checked}
                    disabled={disabled}
                    onChange={onChange}
                />
                {children}
            </label>
        </Field>
    );
};

export { Form, Header, Field, Subfield, ExtendedHeader, CheckboxField };
