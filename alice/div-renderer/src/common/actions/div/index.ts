import { IDivAction } from 'divcard2';

export const setStateAction = (stateId: string | string[], temporary = false) => {
    if (Array.isArray(stateId)) {
        stateId = stateId.join('/');
    }
    return `div-action://set_state?state_id=${stateId}&temporary=${temporary}`;
};

export function setVariable(name: string, value: string | number): string {
    return `div-action://set_variable?name=${name}&value=${value}`;
}

export const setCurrentItem = (divId: string, itemIndex: number) => {
    return `div-action://set_current_item?id=${divId}&item=${itemIndex}`;
};

export interface IStatePlace {
    name: string;
    place: string[];
}

interface ISetStateActionInAllPlacesProps {
    places: IStatePlace[];
    logPrefix: string;
    state: [string, string];
    temporary?: boolean;
    payload?: {}
}

export const setStateActionInAllPlaces = ({
    places,
    logPrefix,
    state,
    temporary = false,
    payload = {},
}: ISetStateActionInAllPlacesProps): IDivAction[] => {
    return places.map((place): IDivAction => {
        return {
            log_id: `${logPrefix}${place.name}`,
            url: setStateAction([
                ...place.place,
                ...state,
            ], temporary),
            payload,
        };
    });
};
