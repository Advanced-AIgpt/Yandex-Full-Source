import React, { useEffect, useState } from 'react';
import { Field, FieldProps } from '../Form/Form';
import { Select, Spin } from 'lego-on-react';
import block from 'propmods';
import './LazySelect.scss';

const b = block('LazySelect');

interface Props<T> {
    value: string;
    fetch: () => Promise<T[]>;
    idSelector: (item: T) => string;
    nameSelector: (item: T) => string;
    onChange: (value: string) => void;
    disabled?: boolean;
    onLoad?: (data: T[]) => void;
    onLoadError?: () => void;
    defaultValue?: {
        label: string;
        value: string;
    };
}

export function LazySelect<T>({
    fetch,
    value,
    onChange,
    disabled,
    idSelector,
    nameSelector,
    onLoad,
    onLoadError,
    defaultValue,
}: Props<T>) {
    const [items, setItems] = useState<T[]>([]);
    const [isLoading, setIsLoading] = useState(false);

    const fetchData = async () => {
        if (isLoading) {
            return;
        }

        setIsLoading(true);
        try {
            const data = await fetch();
            setItems(data);
            onLoad && onLoad(data);
        } catch (e) {
            console.error(e);
            onLoadError && onLoadError();
        } finally {
            setIsLoading(false);
        }
    };

    useEffect(() => {
        fetchData().catch(console.error);
    }, []);

    return (
        <div {...b('select-wrapper')} onClick={() => fetchData()}>
            {isLoading && (
                <div {...b('spinner')}>
                    <Spin size='xs' progress />
                </div>
            )}
            <Select
                theme='normal'
                placeholder='Не выбрано'
                size='s'
                type='radio'
                val={value}
                onChange={(val) => {
                    [val] = Array<string>().concat(val); // TODO remove after lego typings fix

                    onChange(val);
                }}
                disabled={disabled}
            >
                <Select.Item key={'default'} val={defaultValue ? defaultValue.value : undefined}>
                    {defaultValue ? defaultValue.label : 'Не выбрано'}
                </Select.Item>
                {items.map((item, i) => (
                    <Select.Item key={i} val={idSelector(item)}>
                        {nameSelector(item)}
                    </Select.Item>
                ))}
            </Select>
        </div>
    );
}

function LazySelectField<T>(props: Props<T> & FieldProps) {
    const {
        fetch,
        value,
        onChange,
        disabled,
        idSelector,
        nameSelector,
        onLoad,
        onLoadError,
        defaultValue,
        ...fieldProps
    } = props;

    return (
        <Field {...fieldProps}>
            <LazySelect {...props} />
        </Field>
    );
}

export default LazySelectField;
