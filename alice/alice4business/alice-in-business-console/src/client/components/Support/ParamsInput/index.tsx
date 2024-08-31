import React, {
    useState,
    useCallback,
    Dispatch,
    SetStateAction,
    useContext,
    useRef,
    useLayoutEffect,
    createContext
} from 'react';
import { cn } from '@bem-react/classname';
import { Select } from '@yandex-lego/components/Select/desktop/bundle';
import { Textinput } from '@yandex-lego/components/Textinput/desktop/bundle';
import { Spacer } from '@yandex-lego/components/Spacer/desktop';
import { Button } from '@yandex-lego/components/Button/desktop/bundle';

import { ParamNameType } from '../../../pages/Support/config';
import { OptionsObjectType, OptionsType, ParamsType } from '../../../pages/Support/SupportPage';

import './styles.scss'

const b = cn('ParamsInput');

const SearchContext = createContext<any>({});

const validateInput = (type: string, value: string | string[] | undefined, important: boolean) => {
    if ((Array.isArray(value) ? value.length === 0 : !value) && important) {
        return 'Необходимо ввести значение'
    }
    if(type.split('input')[1] === 'Number'){
        if(isNaN(Number(value))){
            return 'Необходимо ввести числовое значение';
        } else {
            return 'ok';
        }
    }
    return 'ok';
}

interface SelectInputProps {
    optionsObject: OptionsObjectType;
    onChangeCallback: (chosenCmd: string | string[], chosenParamName: ParamNameType | undefined) => Promise<void>;
    setParams: Dispatch<SetStateAction<ParamsType>>;
    setGlobalReady: Dispatch<SetStateAction<boolean>>;
    index: number;
}

export const ParamsInput = ({optionsObject, onChangeCallback, setParams, setGlobalReady, index}: SelectInputProps) => {
    const [value, setValue] = useState();
    const [validationError, setValidationError] = useState('');
    const [filteredOptions, setFilteredOptions] = useState([] as OptionsType);
    const [searchValue, setSearchValue] = useState();
    const [ready, setReady] = useState(false);

    const {label, options, inputType, paramName, defaultValues, important} = optionsObject;

    // Если значение уже ранее выбрано ИЛИ после смены типа операции нам нужно дождаться сброса, то берём value, иначе default
    let inputValue: string | string[] = value !== undefined || index === 1 ?
        value || '' :
        defaultValues && paramName && defaultValues[paramName] || '';

    // Если у нас мультивыбор, то оборачиваем в массив, а так же убираем пустое первоначальное значение
    if(inputType === 'selectCheck'){
        inputValue = Array.isArray(inputValue) ? inputValue : (inputValue ? [inputValue] : [])
    }

    const onChange = useCallback(async (e) => {
        const chosenCmd = e.target.value;
        setValue(chosenCmd);
        const isScenarioChoice = index === 0;

        // Save parameters on select change
        if (inputType.includes('select')) {
            await onChangeCallback(chosenCmd, paramName);
            if(!isScenarioChoice && paramName) {
                setParams((prev) => {
                    prev[paramName] = chosenCmd ? chosenCmd : null;
                    return Object.assign({}, prev);
                })
            }
        } else {
            // Очищаем ошибки и сбрасываем готовность, если поменяли инпут
            setGlobalReady(false)
            setReady(false)
            setValidationError('')
        }
    }, [inputType, setParams, index, onChangeCallback, value, inputValue, setReady]);

    // Save parameters on input submit
    const onClick = useCallback(async () => {
        const validationStatus = validateInput(inputType, inputValue, important);
        if(validationStatus === 'ok') {
            await onChangeCallback(inputValue, paramName);
            if (paramName) {
                setParams((prev) => {
                    prev[paramName] = inputValue ? inputValue : null;
                    return Object.assign({}, prev)
                })
            }
            // disable save button for input until next edit
            setReady(true)
        } else {
            setValidationError(validationStatus)
        }
    }, [value, setParams, onChangeCallback, inputValue])

    // Select search input onChange handler
    const onSearchInputChange = useCallback((event) => {
        setSearchValue(event.target.value);
        setFilteredOptions(options.filter((option) => option.content.toLowerCase().includes(event.target.value.toLowerCase())));
    }, []);

    let selectOptions = filteredOptions.length ? filteredOptions : options;
    selectOptions = important ? selectOptions : [{content: '-', value: null}, ...selectOptions]
    return (
        <div className={b()}>
            <p>
                {label}{important && <span className={b('Important')}>{' *'}</span>}
            </p>
            {
                inputType.includes('select')  ? (
                    <SearchContext.Provider value={{ onChange: onSearchInputChange, value: searchValue, className: b('RenderMenu') }}>
                        <Select
                            view="default"
                            size="m"
                            value={inputValue}
                            onChange={onChange}
                            options={selectOptions}
                            iconProps={{
                                type: 'arrow',
                            }}
                            renderMenu={RenderMenu}
                        />
                    </SearchContext.Provider>
                ) : (
                    <div className={b('TextInput')}>
                        <Textinput
                            state={validationError ? 'error' : undefined}
                            hint={validationError ? validationError : undefined}
                            size="m"
                            view="default"
                            value={inputValue as string}
                            onChange={onChange}
                        />
                        <Button
                            className={b('TextInput-Button')}
                            size="s"
                            view="action"
                            disabled={ready}
                            onClick={onClick}
                        >
                            Сохранить
                        </Button>
                    </div>
                )
            }
        </div>

    )
}

const RenderMenu = (props: any, Menu: any) => {
    const { onChange, value, className } = useContext(SearchContext);
    const controlRef = useRef<HTMLInputElement | null>(null);
    const prevActiveNode = useRef<HTMLElement | null>(null);

    useLayoutEffect(() => {
        if (props.focused) {
            prevActiveNode.current = document.activeElement as HTMLElement;
            controlRef.current && controlRef.current.focus();
        } else if (controlRef.current === document.activeElement && prevActiveNode.current !== null) {
            prevActiveNode.current.focus();
        }
    }, [props.focused]);

    return (
        <div className="MenuWrapper">
            <Spacer all="8px">
                <Textinput onChange={onChange} value={value} controlRef={controlRef} view="default" size="m" />
            </Spacer>
            <Menu style={{ backgroundColor: 'var(--color-bg-default)' }} {...props} className={className} />
        </div>
    );
};

