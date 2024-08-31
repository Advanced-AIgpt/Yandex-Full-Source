import { centaurTemplateHelper } from '../../../../index';

export function ChangeButton(
    imageUrl: string,
    text: string,
    options: Partial<Parameters<typeof centaurTemplateHelper.change_button>[0]>,
) {
    return centaurTemplateHelper.change_button({
        ...options,
        change_image: imageUrl,
        change_text: text,
    });
}
