import React, { createRef, Dispatch, RefObject, SetStateAction, useCallback } from 'react';
import { cn } from '@bem-react/classname'
import { Text } from '@yandex-lego/components/Text/touch-pad/bundle';

import { useCustomerPageCtx } from '../../../pages/Customer/CustomerPage';
import { INPUT_ERRORS } from '../../../utils/errors';

import './styles.scss';

const b = cn('CodeInput')

interface CellProps {
    innerRef: RefObject<HTMLInputElement>;
    index: number;
    setCode: Dispatch<SetStateAction<number[]>>;
    initialValue: string;
}

const inputRefs:any = Array.from(new Array(4)).map(()=> createRef());

const Cell = ({ innerRef, index, setCode, initialValue }: CellProps) => {
    const { setError } = useCustomerPageCtx();

    const handleChange = useCallback((e) => {
        const value = e.target.value
        setCode((prev: number[]) => [...prev.slice(0,index), value.length > 1 ? value[1] : value, ...prev.slice(index+1)]);

        if (value.length === 1) {
            inputRefs[index + 1]?.current.focus();
        } else if (value.length === 2) {
            e.target.value = value[1];
            inputRefs[index + 1]?.current.focus();
        } else if (value.length === 0) {
            inputRefs[index -1]?.current.focus();
        }
    }, []);

    const handleClick = useCallback(() => {
        inputRefs[index]?.current.select();
        setError(undefined)
    }, []);


    return (
        <input
            type="tel"
            value={initialValue}
            autoFocus={index===0}
            ref={innerRef}
            className={b('Content-Cell')}
            onChange={handleChange}
            onClick={handleClick}
        />
    )
}


export const CodeInput = ({setCode, initialCode} : any) => {
    const { error } = useCustomerPageCtx();

    const errorMsg = error && INPUT_ERRORS.includes(error.status) ? error.text : ''

    return (
        <div className={b()}>
            <div className={b('Content', {Error: !!errorMsg})}>
                {Array.from(new Array(4)).map((el, index) => (
                    <Cell setCode={setCode} innerRef={inputRefs[index]} index={index} key={index} initialValue={initialCode[index]}/>
                ))}
            </div>
            {errorMsg && (
                <Text typography="body-short-s" color="alert" className={b('ErrorText')}>
                    {errorMsg}
                </Text>
            )}
        </div>
    )
}
