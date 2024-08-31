import Ajv, { AnySchema, JSONSchemaType } from 'ajv';
import { DeepNonNullable, DeepRequired } from 'ts-essentials';
import { IRequestState } from '../../../common/types/common';
import { ExpFlags } from '../expFlags';

export const ajv = new Ajv();

type DataAdapter<D, R> = (data: D, requestState: IRequestState) => R;

export const createDataAdapter = <D, R>(schema: AnySchema, dataAdapter: DataAdapter<D, R>) => {
    const validate = ajv.compile(schema);

    const resultDataAdapter: DataAdapter<D, R> = (data, requestState) => {
        const isDataValid = validate(data);

        if (isDataValid === false) {
            // TODO: CENTAUR-1243: log errors

            if (requestState.hasExperiment(ExpFlags.errorOnAssert)) {
                const [firstError] = validate.errors ?? [];

                throw new Error(`DataValidationError: "${firstError.instancePath}" ${firstError.message}`);
            }
        }

        return dataAdapter(data, requestState);
    };

    return resultDataAdapter;
};

export type MakeClear<T> = DeepRequired<DeepNonNullable<T>>;
export type MakeTestSchemaType<T> = JSONSchemaType<MakeClear<T>>;
