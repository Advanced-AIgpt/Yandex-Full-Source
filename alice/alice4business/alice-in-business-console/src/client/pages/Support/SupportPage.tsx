import React, { useCallback, useState, useEffect } from 'react';
import { useAlert } from 'react-alert';
import { cn } from '@bem-react/classname';
import { Button } from '@yandex-lego/components/Button/desktop/bundle';

import { ParamsInput } from '../../components/Support/ParamsInput'
import { useSupportApi } from '../../context/support-api';
import { ISupportApi } from '../../lib/support-api';
import Alert from '../../blocks/Alert/Alert';

import './SupportPage.scss';
import config, { CmdValueType, ParamNameType, RequiredInputType } from  './config';

const b = cn('SupportPage');

type ScenarioType = Record<string, CmdValueType>;
type InputType = 'selectRadio' | 'selectCheck' | RequiredInputType;
export type OptionsType = Array<{value: string | null, content: string}>;
export type ResponseType = Array<{value: string, name: string}>;
export type ParamsType = Record<ParamNameType, string | string[] | null>;

export interface OptionsObjectType {
    label: string;
    options: OptionsType;
    important: boolean;
    inputType: InputType;
    paramName?: ParamNameType;
    defaultValues?: ParamsType
}

const getOptions = async (cmd: string, api: ISupportApi, params: ParamsType): Promise<{options: OptionsType | [], inputType: InputType}> => {
    let res: ResponseType = [];
    let inputType: InputType = 'selectRadio';

    if(cmd.startsWith('organizationId')) {
        res = await api.getAllOrganizations();
    } else if(cmd.startsWith('deviceId')) {
        res = await api.getAllDevices();
    } else if(cmd.startsWith('connectOrgId')) {
        res = await api.getConnectOrganizations();
    } else if (cmd.startsWith('orgDeviceIds')) {
        res = await api.getOrganizationDevices(params.organizationId?.toString() || '')
    } else if (cmd.startsWith('userId')) {
        res = await api.getAllUsers()
    } else {
        res = []
    }

    if (cmd.startsWith('input')) {
        inputType = cmd as InputType;
    } else if (cmd.endsWith('Checkbox')) {
        inputType = 'selectCheck'
    }

    const options = res.map((el) => {
        return {
            value: el.value,
            content: el.name,
        }
    })

    return {
        options,
        inputType
    }
}

const getDefaultValues = async (chosenCmd: string, api: ISupportApi, value: string): Promise<ParamsType> => {
    switch (chosenCmd){
        case 'device:change':
            return await api.getDeviceDefaults(value)
        case 'organization:change':
            return await api.getOrganizationDefaults(value)
    }

    return {} as ParamsType
}

const defaultState = {
    optionsArr: [{
        label: 'Выберите операцию',
        options: config.operations,
        inputType: 'selectRadio',
        important: true
    }] as OptionsObjectType[],
    scenario: [] as ScenarioType[],
    params: {} as ParamsType,
    defaults: {} as ParamsType,
    ready: false,
    choseParamNextStep: false,
    hasAccess: true,
    chosenCommand: ''
}

export const SupportPage = () => {
    const [optionsArr, setOptionsArr] = useState<OptionsObjectType[]>(defaultState.optionsArr);
    const [scenario, setScenario] = useState<ScenarioType[]>(defaultState.scenario);
    const [params, setParams] = useState<ParamsType>(Object.assign({}, defaultState.params));
    const [defaults, setDefaults] = useState<ParamsType>(Object.assign({}, defaultState.defaults));
    const [ready, setReady] = useState(defaultState.ready);
    const [choseParamNextStep, setChoseParamNextStep] = useState(defaultState.choseParamNextStep);
    const [hasAccess, setHasAccess] = useState(defaultState.hasAccess); // Делаем true, чтобы не дёргался UI, но API дополнительно чекает
    const [chosenCommand, setChosenCommand] = useState(defaultState.chosenCommand);

    const api = useSupportApi();
    const alert = useAlert();

    useEffect(() => {
        const getAccess = async () => {
            try {
                await api.getAccess()
                setHasAccess(true);
            } catch {
                setHasAccess(false);
            }
        }
        getAccess().then(
            () => {},
            () => {},
        );
    }, [])

    const onChangeCallback = useCallback(async (chosenOption: Partial<string | string[]>, chosenParamName: ParamNameType | undefined) => {
        const isScenarioChoice = !chosenParamName;

        if(isScenarioChoice && !Array.isArray(chosenOption)) {
            setScenario(config.scenarios[chosenOption]);
            setChosenCommand(chosenOption);

            setParams(Object.assign({}, defaultState.params));
            setDefaults(Object.assign({}, defaultState.defaults));
            setReady(defaultState.ready)
            setOptionsArr(defaultState.optionsArr)
        }

        return new Promise<void>(async (resolve) => {
            // Если изменен ранее выбранный параметр
            if(!isScenarioChoice && chosenParamName && params[chosenParamName]){
                if (scenario.length === 0) {
                    setReady(true)
                }
                resolve()
            } else if(!isScenarioChoice && scenario.length === 0) {
                // Если выбраны все параметры из сценария
                setReady(true)
                resolve();
            } else {
                // Необходимо и обновить state и использовать полученные данные,
                // поэтому при первом проходе используем переменную, затем state
                let curDefaults: ParamsType = {} as ParamsType;
                const curScenarioStep: ScenarioType = isScenarioChoice && !Array.isArray(chosenOption)
                    ? config.scenarios[chosenOption][0]
                    : scenario[0]!;

                // Если мы собираемся редактировать, то предупреждаем,
                // что следующее поле будет использоваться для получения дефолтных значений
                if (isScenarioChoice && chosenOption.includes('change')){
                    setChoseParamNextStep(true)
                }
                if( choseParamNextStep && !isScenarioChoice && !Array.isArray(chosenOption)) {
                    try{
                        curDefaults = await getDefaultValues(chosenCommand, api, chosenOption)
                        setDefaults(curDefaults)
                        setChoseParamNextStep(false);
                    } catch (e) {
                        alert.error('Ошибка. Возможно устройства/организации с таким id не существует.');
                        throw e;
                    }
                }

                const [label, {cmd, paramName, important}] = Object.entries(curScenarioStep)[0];
                let options: OptionsType = []
                let inputType: InputType = 'selectRadio'

                // Если это уже готовый массив значений
                if(Array.isArray(cmd)) {
                    options = cmd.map((option) => {
                        return {
                            value: option,
                            content: option
                        }
                    });
                } else {
                    // Определяем какие опции нам нужно достать с бэка, либо ввести значение самому
                    try {
                        ({options, inputType} = await getOptions(cmd, api, params));
                    } catch (e){
                        alert.error('Ошибка. Не удалось получить возможные варианты.')
                        throw e;
                    }
                }

                setOptionsArr(
                    [
                        ...(isScenarioChoice ? defaultState.optionsArr : optionsArr),
                        {
                            label,
                            options,
                            important,
                            inputType,
                            paramName,
                            defaultValues: Object.values(defaults).length > 0 ? defaults : curDefaults
                        }
                    ]);
                // Убираем уже выведенный шаг из сценария
                if(isScenarioChoice && !Array.isArray(chosenOption)) {
                    setScenario(config.scenarios[chosenOption].slice(1))
                } else {
                    setScenario(scenario.slice(1))
                }
                resolve()
            }
        })
    }, [scenario, optionsArr, params]);

    const onSubmit = useCallback(async () => {
        let res;
        try {
            switch (chosenCommand) {
                case 'device:create':
                    res = await api.createDevice(params)
                    break;
                case 'device:change':
                    res = await api.changeDevice(params)
                    break;
                case 'device:add-puid':
                    res = await api.addPuid(params);
                    break;
                case 'organization:create':
                    res = await api.createOrganization(params)
                    break;
                case 'organization:change':
                    res = await api.changeOrganization(params)
                    break;
                case 'organization:set-max-station-volume':
                    res = await api.setOrganizationMaxVolume(params)
                    break;
                case 'promocode:add':
                    res = await api.addPromocode(params)
                    break;
                case 'promocode:add-to-organization':
                    res = await api.addPromocodeToOrganization(params)
                    break;
                case 'room:create':
                    res = await api.createRoom(params)
                    break;
                case 'users:create':
                    res = await api.createUser(params)
                    break;
                case 'users:bind':
                    res = await api.bindUsers(params)
                    break;
            }
            if (res){
                if (res.message) {
                    if (res.status === 'ok') {
                        alert.info(res.message)
                    } else {
                        alert.error(res.message)
                    }
                } else if(res.status === 'not ok'){
                    alert.error('Ошибка')
                } else {
                    setScenario(Object.assign({}, defaultState.scenario));
                    setChosenCommand(defaultState.chosenCommand);
                    setParams(Object.assign({}, defaultState.params));
                    setDefaults(Object.assign({}, defaultState.defaults));
                    setReady(defaultState.ready)
                    setOptionsArr(defaultState.optionsArr)

                    alert.success('Успех!')
                }
            }
        } catch {
            alert.error('Ошибка. Что-то пошло не так.')
        }
    }, [params])

    return (
        <div className={b()}>
            <div className={b('Content')}>
                {hasAccess ? (
                    <>
                        <div className={b('Content-Input')}>
                            {optionsArr.map((options, index) => (
                                <ParamsInput setGlobalReady={setReady} setParams={setParams} optionsObject={options} onChangeCallback={onChangeCallback} index={index} key={options.label}/>
                            ))}
                        </div>
                        <Button className={b('Content-Button')} view="action" size="m"  disabled={!ready} onClick={onSubmit}>
                            Выполнить
                        </Button>
                    </>
                ) : (
                    <Alert size='l' type='error'>
                        У Вас недостаточно прав
                    </Alert>
                )}
            </div>
        </div>
    )
}
