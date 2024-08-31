import { ContainerBlock, TemplateCard, Templates, TextBlock } from 'divcard2';
import { NAlice } from '../../../../protos';
import { ServiceTab } from '../../components/serviceTab';
import { EmptyServiceTab } from '../../components/serviceTab/EmptyServiceTab';

const gap = 60;
const colWidth = 168;
const spaceBetweenRows = 24;
/*
Values of variables in the location of blocks:
      --------------       --------------
      |************|       |************|
←gap→ |*←colWidth→*| ←gap→ |*←colWidth→*| ←gap→
      |************|       |************|
      --------------       --------------
                       ↑
               spaceBetweenRows
                       ↓
      --------------       --------------
      |************|       |************|
←gap→ |*←colWidth→*| ←gap→ |*←colWidth→*| ←gap→
      |************|       |************|
      --------------       --------------
                       ↑
               spaceBetweenRows
                       ↓
      --------------       --------------
      |************|       |************|
←gap→ |*←colWidth→*| ←gap→ |*←colWidth→*| ←gap→
      |************|       |************|
      --------------       --------------
 */

const deviceWidth = 1280;
const colsInRow = Math.floor((deviceWidth - gap) / (colWidth + gap));

const FallBack = () => {
    return new TemplateCard(new Templates({}), {
        log_id: 'renderMainScreenServiceTab.FallBack',
        states: [
            {
                state_id: 0,
                div: new TextBlock({
                    text: '',
                }),
            },
        ],
    });
};

export const renderMainScreenServiceTab = ({
    CentaurMainScreenWebviewCardData,
}: NAlice.NData.ITCentaurMainScreenServicesTabData) => {
    if (!CentaurMainScreenWebviewCardData) {
        return FallBack();
    }

    const length = CentaurMainScreenWebviewCardData.length;
    const rows = Math.ceil(length / colsInRow);

    const rowItems = [];

    for (let i = 0; i < rows; i++) {
        const rowItem = [];

        const endOfRow = colsInRow * (i + 1);
        for (let j = i * colsInRow; j < endOfRow; j++) {
            rowItem.push(j < length ? ServiceTab(CentaurMainScreenWebviewCardData[j]) : EmptyServiceTab());
        }
        rowItems.push(new ContainerBlock({
            orientation: 'horizontal',
            alignment_horizontal: 'left',
            margins: {
                top: i && spaceBetweenRows,
            },
            items: rowItem,
        }));
    }

    const dataForTemplate: ConstructorParameters<typeof TemplateCard>[1] = {
        log_id: 'main_screen.service',
        states: [
            {
                state_id: 0,
                div: new ContainerBlock({
                    orientation: 'vertical',
                    items: rowItems,
                }),
            },
        ],
    };

    return new TemplateCard(new Templates({}), dataForTemplate);
};
