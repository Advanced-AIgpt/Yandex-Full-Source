import { DivCustomBlock, FixedSize, GifBlock, MatchParentSize, TemplateCard, Templates } from 'divcard2';
import { createShowViewClientAction, EnumLayer } from '../../actions/client';
import { getS3Asset } from '../../helpers/assets';
import { CloseButtonWrapper } from '../CloseButtonWrapper/CloseButtonWrapper';
import { Layer } from '../../common/layers';
import { TopLevelCard } from '../../helpers/helpers';
import { directivesAction } from '../../../../common/actions';
import { createRequestState } from '../../../../registries/common';

export function LoaderDiv(props?: Partial<ConstructorParameters<typeof GifBlock>[0]>): GifBlock {
    return new GifBlock({
        gif_url: getS3Asset('loaders/spinner.gif'),
        width: new FixedSize({ value: 250 }),
        height: new FixedSize({ value: 250 }),
        alignment_horizontal: 'center',
        alignment_vertical: 'center',
        ...props,
    });
}

export function CrutchLoader(layer = EnumLayer.content) {
    let layerToClose = Layer.CONTENT;

    switch (layer) {
        case EnumLayer.content:
            layerToClose = Layer.CONTENT;
            break;
        case EnumLayer.dialog:
            layerToClose = Layer.DIALOG;
            break;
    }

    const actionRequestState = createRequestState();

    return {
        log_id: 'loader_click',
        url: directivesAction(createShowViewClientAction(
            TopLevelCard({
                log_id: 'loader',
                states: [
                    {
                        state_id: 0,
                        div: CloseButtonWrapper({ div: LoaderDiv(), layer: layerToClose }),
                    },
                ],
            }, actionRequestState),
            null,
            layer,
        )),
    };
}

export function Loader() {
    return {
        log_id: 'loader_click',
        url: directivesAction(createShowViewClientAction(
            new TemplateCard(new Templates({}), {
                log_id: 'player_loader',
                states: [
                    {
                        state_id: 0,
                        div: new DivCustomBlock({
                            custom_type: 'lottie',
                            width: new MatchParentSize(),
                            height: new MatchParentSize(),
                            custom_props: {
                                steps: [
                                    {
                                        source: {
                                            type: 'json',
                                            cacheKey: '05898201-7f77-4b01-9f0b-4ec5b7ee0cfd',
                                            json_string: '{"v":"5.7.4","meta":{"g":"LottieFiles AE ","a":"","k":"","d":"","tc":""},"fr":25,"ip":0,"op":10,"w":166,"h":166,"nm":"spinner","ddd":0,"assets":[],"layers":[{"ddd":0,"ind":1,"ty":4,"nm":"spinner Outlines","sr":1,"ks":{"o":{"a":0,"k":100,"ix":11},"r":{"a":1,"k":[{"i":{"x":[0.833],"y":[0.833]},"o":{"x":[0.167],"y":[0.167]},"t":0,"s":[0]},{"t":10,"s":[360]}],"ix":10},"p":{"a":0,"k":[83,83,0],"ix":2},"a":{"a":0,"k":[83,83,0],"ix":1},"s":{"a":0,"k":[100,100,100],"ix":6}},"ao":0,"shapes":[{"ty":"gr","it":[{"ind":0,"ty":"sh","ix":1,"ks":{"a":0,"k":{"i":[[0,0],[-2.209,0],[0,2.209]],"o":[[0,2.209],[2.209,0],[0,0]],"v":[[-26,22],[-22,26],[-18,22]],"c":true},"ix":2},"nm":"Path 1","mn":"ADBE Vector Shape - Group","hd":false},{"ind":1,"ty":"sh","ix":2,"ks":{"a":0,"k":{"i":[[0,0],[0,2.209],[2.209,0]],"o":[[2.209,0],[0,-2.209],[0,0]],"v":[[22,-18],[26,-22],[22,-26]],"c":true},"ix":2},"nm":"Path 2","mn":"ADBE Vector Shape - Group","hd":false},{"ind":2,"ty":"sh","ix":3,"ks":{"a":0,"k":{"i":[[0,0],[-22.091,0],[0,0],[0,-26.51]],"o":[[0,-22.091],[0,0],[-26.51,0],[0,0]],"v":[[-18,22],[22,-18],[22,-26],[-26,22]],"c":true},"ix":2},"nm":"Path 3","mn":"ADBE Vector Shape - Group","hd":false},{"ty":"mm","mm":1,"nm":"Merge Paths 1","mn":"ADBE Vector Filter - Merge","hd":false},{"ty":"fl","c":{"a":0,"k":[1,1,1,1],"ix":4},"o":{"a":0,"k":100,"ix":5},"r":1,"bm":0,"nm":"Fill 1","mn":"ADBE Vector Graphic - Fill","hd":false},{"ty":"tr","p":{"a":0,"k":[61,61],"ix":2},"a":{"a":0,"k":[0,0],"ix":1},"s":{"a":0,"k":[100,100],"ix":3},"r":{"a":0,"k":0,"ix":6},"o":{"a":0,"k":100,"ix":7},"sk":{"a":0,"k":0,"ix":4},"sa":{"a":0,"k":0,"ix":5},"nm":"Transform"}],"nm":"Group 1","np":5,"cix":2,"bm":0,"ix":1,"mn":"ADBE Vector Group","hd":false},{"ty":"gr","it":[{"ind":0,"ty":"sh","ix":1,"ks":{"a":0,"k":{"i":[[0,0],[22.091,0],[0,0],[0,26.51]],"o":[[0,22.091],[0,0],[26.51,0],[0,0]],"v":[[40,0],[0,40],[0,48],[48,0]],"c":true},"ix":2},"nm":"Path 1","mn":"ADBE Vector Shape - Group","hd":false},{"ind":1,"ty":"sh","ix":2,"ks":{"a":0,"k":{"i":[[0,0],[0,22.091],[0,0],[-26.51,0]],"o":[[-22.091,0],[0,0],[0,26.51],[0,0]],"v":[[0,40],[-40,0],[-48,0],[0,48]],"c":true},"ix":2},"nm":"Path 2","mn":"ADBE Vector Shape - Group","hd":false},{"ind":2,"ty":"sh","ix":3,"ks":{"a":0,"k":{"i":[[0,0],[-22.091,0],[0,0],[0,-26.51]],"o":[[0,-22.091],[0,0],[-26.51,0],[0,0]],"v":[[-40,0],[0,-40],[0,-48],[-48,0]],"c":true},"ix":2},"nm":"Path 3","mn":"ADBE Vector Shape - Group","hd":false},{"ind":3,"ty":"sh","ix":4,"ks":{"a":0,"k":{"i":[[0,0],[0,-22.091],[0,0],[26.51,0]],"o":[[22.091,0],[0,0],[0,-26.51],[0,0]],"v":[[0,-40],[40,0],[48,0],[0,-48]],"c":true},"ix":2},"nm":"Path 4","mn":"ADBE Vector Shape - Group","hd":false},{"ty":"mm","mm":1,"nm":"Merge Paths 1","mn":"ADBE Vector Filter - Merge","hd":false},{"ty":"fl","c":{"a":0,"k":[1,1,1,1],"ix":4},"o":{"a":0,"k":10,"ix":5},"r":1,"bm":0,"nm":"Fill 1","mn":"ADBE Vector Graphic - Fill","hd":false},{"ty":"tr","p":{"a":0,"k":[83,83],"ix":2},"a":{"a":0,"k":[0,0],"ix":1},"s":{"a":0,"k":[100,100],"ix":3},"r":{"a":0,"k":0,"ix":6},"o":{"a":0,"k":100,"ix":7},"sk":{"a":0,"k":0,"ix":4},"sa":{"a":0,"k":0,"ix":5},"nm":"Transform"}],"nm":"Group 2","np":6,"cix":2,"bm":0,"ix":2,"mn":"ADBE Vector Group","hd":false}],"ip":0,"op":10,"st":0,"bm":0}],"markers":[]}',
                                        },
                                        count: {
                                            type: 'infinity',
                                        },
                                        scale: 'center_crop',
                                    },
                                ],
                            },
                        }),
                    },
                ],
            }),
            false,
        )),
    };
}
