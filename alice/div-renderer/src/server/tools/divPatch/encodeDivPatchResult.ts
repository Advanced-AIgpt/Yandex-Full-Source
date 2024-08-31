import { IRequestState } from '../../../common/types/common';
import { IDivPatchElement, OneOfKey } from '../../../projects';
import { NAlice } from '../../../protos';
import { struct } from 'pb-util/build';

export interface DivPatchResultItemToEncode {
    requestState: IRequestState;
    templateId: OneOfKey;
    result: IDivPatchElement[];
    data: NAlice.NRenderer.TDivRenderData;
}

export const encodeDivPatchResult = (divPatchResultItem: DivPatchResultItemToEncode) => {
    const { data, result } = divPatchResultItem;

    const res = {
        changes: result,
        mode: 'transactional',
    } as {};

    return new NAlice.NRenderer.TRenderResponse({
        CardId: data.CardId,
        CardName: data.CardName,
        Div2PatchBody: {
            Div2PatchBody: struct.encode(res),
        },
    });
};
