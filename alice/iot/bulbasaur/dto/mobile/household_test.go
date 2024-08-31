package mobile

import (
	"context"
	"testing"

	"a.yandex-team.ru/alice/iot/bulbasaur/controller/sharing/model"
	"a.yandex-team.ru/alice/iot/bulbasaur/model"
	"a.yandex-team.ru/alice/library/go/geosuggest"
	"a.yandex-team.ru/library/go/ptr"
	"github.com/stretchr/testify/assert"
)

func TestHouseholdUpdateRequestToHousehold(t *testing.T) {
	req := HouseholdCreateRequest{
		HouseholdNameValidateRequest:     HouseholdNameValidateRequest{Name: "Мой дом"},
		HouseholdLocationValidateRequest: HouseholdLocationValidateRequest{ptr.String("Россия, Москва, проспект 60-летия октября")},
	}
	location := model.HouseholdLocation{
		Longitude:    45,
		Latitude:     46,
		Address:      "Россия, Москва, проспект 60-летия октября",
		ShortAddress: "Проспект 60-летия октября",
	}
	expected := model.Household{
		Name:     "Мой дом",
		Location: &location,
	}
	assert.Equal(t, expected, req.ToHousehold(&location))
}

func TestHouseholdViewFromHousehold(t *testing.T) {
	household := model.Household{
		ID:   "1",
		Name: "Картонная коробка",
		Location: &model.HouseholdLocation{
			Longitude:    45,
			Latitude:     46,
			Address:      "Помойка",
			ShortAddress: "Помойка",
		},
	}
	var view HouseholdView
	view.From(household.ID, household)
	expected := HouseholdView{
		ID:   "1",
		Name: "Картонная коробка",
		Location: &HouseholdLocationView{
			Address:      "Помойка",
			ShortAddress: "Помойка",
		},
		IsCurrent: true,
	}
	assert.Equal(t, expected, view)
}

func TestHouseholdCreateRequest(t *testing.T) {
	request := HouseholdCreateRequest{
		HouseholdNameValidateRequest:     HouseholdNameValidateRequest{Name: "Домишко"},
		HouseholdLocationValidateRequest: HouseholdLocationValidateRequest{},
	}
	expected := model.Household{
		Name:     "Домишко",
		Location: nil,
	}
	assert.Equal(t, expected, request.ToHousehold(nil))

	location := &model.HouseholdLocation{
		Longitude:    1.1,
		Latitude:     2.2,
		Address:      "Глушь",
		ShortAddress: "Глушка",
	}
	expected = model.Household{
		Name: "Домишко",
		Location: &model.HouseholdLocation{
			Longitude:    1.1,
			Latitude:     2.2,
			Address:      "Глушь",
			ShortAddress: "Глушка",
		},
	}
	assert.Equal(t, expected, request.ToHousehold(location))
}

func TestHouseholdLocationValidateRequest(t *testing.T) {
	geosuggestMock := geosuggest.NewMock()
	geosuggestMock.AddResponseToAddress("Улица Льва Толстого, 16, Москва, Россия", geosuggest.GeosuggestFromAddressResponse{
		Part: "Улица Льва Толстого, 16, Москва, Россия",
		Results: []geosuggest.Geosuggest{
			{
				RawCoordinates: "55.555555,66.666666",
				RawAddress:     "Россия, Москва, улица Льва Толстого, 16",
				Title:          geosuggest.GeosuggestTitle{RawShortAddress: "улица Льва Толстого, 16"},
			},
		},
	})
	geosuggestMock.AddResponseToAddress("Московская область, Балашиха, квартал Абрамцево, 53", geosuggest.GeosuggestFromAddressResponse{
		Part: "Московская область, Балашиха, квартал Абрамцево, 53",
		Results: []geosuggest.Geosuggest{
			{
				RawCoordinates: "55.555555,66.666666",
				RawAddress:     "Россия, Московская область, Балашиха, квартал Абрамцево, 53",
				Title:          geosuggest.GeosuggestTitle{RawShortAddress: "Квартал Абрамцево, 53"},
			},
		},
	})
	request := HouseholdLocationValidateRequest{
		Address: ptr.String("Улица Льва Толстого, 16, Москва, Россия"),
	}
	expected := &model.HouseholdLocation{
		Address:      "Россия, Москва, улица Льва Толстого, 16",
		ShortAddress: "Улица Льва Толстого, 16",
		Longitude:    55.555555,
		Latitude:     66.666666,
	}
	location, err := request.ValidateAddressByGeosuggest(context.Background(), geosuggestMock)
	assert.NoError(t, err)
	assert.NotNil(t, location)
	assert.Equal(t, expected, location)

	request = HouseholdLocationValidateRequest{Address: ptr.String("Московская область, Балашиха, квартал Абрамцево, 53")}
	expected.Address = "Россия, Московская область, Балашиха, квартал Абрамцево, 53"
	expected.ShortAddress = "Квартал Абрамцево, 53"

	location, err = request.ValidateAddressByGeosuggest(context.Background(), geosuggestMock)
	assert.NoError(t, err)
	assert.NotNil(t, location)
	assert.Equal(t, expected, location)
}

func TestGetValidLocationSuggests(t *testing.T) {
	geosuggestMock := geosuggest.NewMock()
	geosuggestMock.AddResponseToAddress("Улица Льва Толстого, 16, Москва, Россия", geosuggest.GeosuggestFromAddressResponse{
		Part: "Улица Льва Толстого, 16, Москва, Россия",
		Results: []geosuggest.Geosuggest{
			{
				RawCoordinates: "55.555555,66.666666",
				RawAddress:     "Россия, Москва, улица Льва Толстого, 16",
				Title:          geosuggest.GeosuggestTitle{RawShortAddress: "улица Льва Толстого, 16"},
			},
		},
	})
	locationSuggests := []model.HouseholdLocation{
		{
			Address: "Улица Льва Толстого, 16, Москва, Россия",
		},
		{
			Address: "Глушь",
		},
	}
	expected := []model.HouseholdLocation{
		{
			Address:      "Россия, Москва, улица Льва Толстого, 16",
			Longitude:    55.555555,
			Latitude:     66.666666,
			ShortAddress: "Улица Льва Толстого, 16",
		},
	}
	assert.Equal(t, expected, GetValidLocationSuggests(context.Background(), locationSuggests, geosuggestMock))
}

func TestHouseholdInfoViewFromHousehold(t *testing.T) {
	household := model.Household{
		ID:   "1",
		Name: "Конура",
		Location: &model.HouseholdLocation{
			Address:      "100",
			ShortAddress: "1",
			Latitude:     1,
			Longitude:    2,
		},
		SharingInfo: &model.SharingInfo{
			OwnerID:     1,
			HouseholdID: "1",
		},
	}
	var view HouseholdInfoView
	view.FromHousehold(household, "1")
	expected := HouseholdInfoView{
		ID:   "1",
		Name: "Конура",
		Location: &HouseholdLocationView{
			Address:      "100",
			ShortAddress: "1",
		},
		IsCurrent: true,
		SharingInfo: &SharingInfoView{
			OwnerID: 1,
		},
	}
	assert.Equal(t, expected, view)
}

func TestUserHouseholdViewFrom(t *testing.T) {
	household := model.Household{
		ID:   "household-id",
		Name: "Скамейка",
		Location: &model.HouseholdLocation{
			Address:      "Парк Горького",
			ShortAddress: "Gorky Park",
			Longitude:    1,
			Latitude:     2,
		},
	}
	locationSuggests := []model.HouseholdLocation{
		{
			Address:      "Улица Льва Толстого, 16, Москва, Россия",
			ShortAddress: "Львенок Толстый",
		},
		{
			Address:      "Глушь",
			ShortAddress: "Глу",
		},
	}
	residents := model.HouseholdResidents{
		{
			ID:   1,
			Role: model.OwnerHouseholdRole,
		},
		{
			ID:   2,
			Role: model.GuestHouseholdRole,
		},
		{
			ID:   3,
			Role: model.GuestHouseholdRole,
		},
	}
	expected := UserHouseholdView{
		Household: HouseholdWithLocationSuggestsView{
			HouseholdView: HouseholdView{
				ID:   "household-id",
				Name: "Скамейка",
				Location: &HouseholdLocationView{
					Address:      "Парк Горького",
					ShortAddress: "Gorky Park",
				},
				IsCurrent:   true,
				IsRemovable: ptr.Bool(false),
			},
			HouseholdLocationSuggestsView: HouseholdLocationSuggestsView{
				LocationSuggests: []HouseholdLocationView{
					{
						Address:      "Улица Льва Толстого, 16, Москва, Россия",
						ShortAddress: "Львенок Толстый",
					},
					{
						Address:      "Глушь",
						ShortAddress: "Глу",
					},
				},
			},
			HasGuests: true,
		},
	}
	var actual UserHouseholdView
	actual.From(household.ID, household, nil, model.Households{household}, locationSuggests, residents)
	assert.Equal(t, expected, actual)
}

func TestUserHouseholdsViewFrom(t *testing.T) {
	box := model.Household{
		ID:   "1",
		Name: "Картонная коробка",
		Location: &model.HouseholdLocation{
			Longitude:    45,
			Latitude:     46,
			Address:      "Помойка",
			ShortAddress: "Помойка",
		},
	}
	walls := model.Household{
		ID:   "2",
		Name: "Бетонные стены",
		Location: &model.HouseholdLocation{
			Longitude:    47,
			Latitude:     48,
			Address:      "Чадлэнд",
			ShortAddress: "Чадлэнд",
		},
	}
	user := sharingmodel.User{
		ID:          1,
		DisplayName: "Тимофей Ш.",
		Login:       "timoshka",
		Email:       "timofei@timofei.ru",
		AvatarURL:   "avatarka",
		PhoneNumber: "+7-***-***-**-**",
		YandexPlus:  true,
	}
	invitation := model.HouseholdInvitation{
		ID:          "invitation-id",
		SenderID:    1,
		HouseholdID: "household-id",
		GuestID:     2,
	}
	expected := UserHouseholdsView{
		Households: []HouseholdView{
			{
				ID:   "1",
				Name: "Картонная коробка",
				Location: &HouseholdLocationView{
					Address:      "Помойка",
					ShortAddress: "Помойка",
				},
				IsCurrent: true,
			},
			{
				ID:   "2",
				Name: "Бетонные стены",
				Location: &HouseholdLocationView{
					Address:      "Чадлэнд",
					ShortAddress: "Чадлэнд",
				},
			},
		},
		Invitations: []HouseholdInvitationShortView{
			{
				ID: "invitation-id",
				Sender: SharingUserView{
					ID:          1,
					DisplayName: "Тимофей Ш.",
					Login:       "timoshka",
					Email:       "timofei@timofei.ru",
					AvatarURL:   "avatarka",
					PhoneNumber: "+7-***-***-**-**",
					YandexPlus:  true,
				},
			},
		},
	}
	var actual UserHouseholdsView
	actual.FromHouseholds("1", model.Households{box, walls}, sharingmodel.Users{user}, model.HouseholdInvitations{invitation})
	assert.Equal(t, expected, actual)
}
