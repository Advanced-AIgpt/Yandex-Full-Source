package buttons

import sdk "a.yandex-team.ru/alice/gamma/sdk/golang"

var YesButton = sdk.Button{
	Title: "Да",
	Hide:  true,
}

var ChooseButtons = []sdk.Button{
	YesButton,
	{
		Title: "Нет",
		Hide:  true,
	},
}
