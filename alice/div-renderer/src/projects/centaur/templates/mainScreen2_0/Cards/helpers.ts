export function getCardMainScreenId({ rowIndex, colIndex }: { rowIndex?: number; colIndex?: number }) {
    return `main_screen_card_${rowIndex || 0}_${colIndex || 0}`;
}
