import ym from 'react-yandex-metrika';

export const METRIKA_ID = 88101768;

const { NODE_ENV } = process.env;

type ParameterType = 'action' | 'puid'

const params = (parameters: Partial<Record<ParameterType, string>>  | Array<Partial<Record<ParameterType, string>>>) => {
    if (NODE_ENV !== 'production') {
        parameters = Array.isArray(parameters) ? parameters : [parameters];
        parameters.map((parameter) => {
            console.log(`Metrica parameter ${JSON.stringify(parameter)}`);
        });
    } else {
        ym('params', parameters);
    }
};

const action = (label: string, isLogin = false) => {
    const key = label + (isLogin ? '_login' : '_notlogin');
    params({ action: key });
};

const puid = (label: string) => {
    params({ puid: label });
};

const error = (label: string, status: number) => {
    const key = label + '_' + status;
    params({ action: key });
}

export const metrica = {
    action,
    puid,
    error
};
