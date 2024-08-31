import {
    TSemanticFrameRequestData,
    TTypedSemanticFrame,
} from '../../../protos/alice/megamind/protos/common/frame';
import { TAnalyticsTrackingModule_EOrigin } from '../../../protos/alice/megamind/protos/common/atm';
import { logger } from '../../logger';

export const createServerAction = (name: string, payload: unknown) => {
    return {
        type: 'server_action',
        name,
        payload,
    } as const;
};

export const createSemanticFrameAction = (
    typedSemanticFrame: unknown,
    productScenario: string,
    purpose: string,
    origin = 'Scenario',
) => {
    return createServerAction(
        '@@mm_semantic_frame',
        {
            typed_semantic_frame: typedSemanticFrame,
            analytics: {
                product_scenario: productScenario,
                origin,
                purpose,
            },
            params: {
                disable_output_speech: true,
                disable_should_listen: true,
            },
        },
    );
};

export const createSemanticFrameActionTypeSafe = (
    typedSemanticFrame: TTypedSemanticFrame,
    productScenario: string,
    purpose: string,
    origin = TAnalyticsTrackingModule_EOrigin.Scenario,
    origin_info = '',
) => {
    return createServerAction(
        '@@mm_semantic_frame',
        TSemanticFrameRequestData.toJSON(
            {
                TypedSemanticFrame: typedSemanticFrame,
                Analytics: {
                    ProductScenario: productScenario,
                    Origin: origin,
                    Purpose: purpose,
                    OriginInfo: origin_info,
                },
                Params: {
                    DisableOutputSpeech: true,
                    DisableShouldListen: true,
                },
            } as TSemanticFrameRequestData),
    );
};

interface CreateServerActionBasedOnTSFParams {
    binaryTsf: Uint8Array | null | undefined;
    productScenario: string;
    purpose: string;
}
export const createServerActionBasedOnTSF = ({
    binaryTsf,
    productScenario,
    purpose,
}: CreateServerActionBasedOnTSFParams) => {
    if (!binaryTsf) {
        logger.error({
            err: new Error(`Failed to decode TypedSemanticFrame, blank value. Scenario: ${productScenario}, purpose: ${purpose}`),
            binaryTsf,
            productScenario,
            purpose,
        });

        return undefined;
    }

    try {
        const tsf = TTypedSemanticFrame.decode(binaryTsf);

        return createSemanticFrameActionTypeSafe(tsf, productScenario, purpose);
    } catch (err) {
        if (err instanceof Error) {
            logger.error({
                err,
                binaryTsf,
                productScenario,
                purpose,
            }, `Failed to decode TypedSemanticFrame. Scenario: ${productScenario}, purpose: ${purpose}`);
        }

        return undefined;
    }
};

