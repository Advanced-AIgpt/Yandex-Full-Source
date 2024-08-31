#include "response_convertor.h"

#include <alice/hollywood/library/framework/core/request.h>

#include <alice/protos/data/lat_lon.pb.h>
#include <alice/protos/data/scenario/order/order.pb.h>

namespace NAlice::NHollywoodFw::NOrder {

namespace {

const THashMap<TString, NAlice::NOrder::TOrder::EStatus> statusMapper {
    {"created" , NAlice::NOrder::TOrder::Created},
    {"assembling" , NAlice::NOrder::TOrder::Assembling},
    {"assembled" , NAlice::NOrder::TOrder::Assembled},
    {"performer_found" , NAlice::NOrder::TOrder::PerformerFound},
    {"delivering" , NAlice::NOrder::TOrder::Delivering},
    {"delivery_arrived" , NAlice::NOrder::TOrder::DeliveryArrived},
    {"closed" , NAlice::NOrder::TOrder::Succeeded},
    {"cancelled" , NAlice::NOrder::TOrder::Canceled},
    {"failed" , NAlice::NOrder::TOrder::Failed},
};

const THashMap<TString, NAlice::NOrder::TOrder::EDeliveryType> deliveryTypeMappper {
    {"curier", NAlice::NOrder::TOrder::Curier},
    {"rover", NAlice::NOrder::TOrder::Rover},
};
}

void ConvertItem(const NJson::TJsonValue& itemJsonValue, TStringBuilder& errorMessage, NAlice::NOrder::TOrder::TItem* itemProtoVariable) {

    if (itemJsonValue.Has("id") && itemJsonValue["id"].IsString()) {
        itemProtoVariable->SetId(itemJsonValue["id"].GetString());
    } else {
        errorMessage << "No element \'id\' in item or not a string type." << Endl;
    }

    if (itemJsonValue.Has("title") && itemJsonValue["title"].IsString()) {
        itemProtoVariable->SetTitle(itemJsonValue["title"].GetString());
    } else {
        errorMessage << "No element \'title\' in item or not a string type." << Endl;
    }

    if (itemJsonValue.Has("actual_price") && itemJsonValue["actual_price"].IsString()) {
        itemProtoVariable->MutableActualPrice()->SetValue(std::stod(itemJsonValue["actual_price"].GetString()));
        itemProtoVariable->MutableActualPrice()->SetCurrency(NAlice::NOrder::TOrder::Rub);
    } else {
        errorMessage << "No element \'actual_price\' in item or not a double type." << Endl;
    }

    if (itemJsonValue.Has("full_price") && itemJsonValue["full_price"].IsString()) {
        itemProtoVariable->MutableFullPrice()->SetValue(std::stod(itemJsonValue["full_price"].GetString()));
        itemProtoVariable->MutableFullPrice()->SetCurrency(NAlice::NOrder::TOrder::Rub);
    } else {
        errorMessage << "No element \'full_price\' in item or not a double type." << Endl;
    }

    if (itemJsonValue.Has("quantity") && itemJsonValue["quantity"].IsString()) {
        itemProtoVariable->SetQuantity(std::stoi(itemJsonValue["quantity"].GetString()));
    } else {
        errorMessage << "No element \'quantity\' in item or not a double type." << Endl;
    }
}

void ConvertAddress(const NJson::TJsonValue& addressJsonValue, TStringBuilder& errorMessage, NAlice::NOrder::TOrder::TAddress* addressProtoValue) {

    if (addressJsonValue.Has("country") && addressJsonValue["country"].IsString()) {
        addressProtoValue->SetCountry(addressJsonValue["country"].GetString());
    } else {
        errorMessage << "No element \'country\' in element  \'address\' in response or not a string type." << Endl;
    }

    if (addressJsonValue.Has("city") && addressJsonValue["city"].IsString()) {
        addressProtoValue->SetCity(addressJsonValue["city"].GetString());
    } else {
        errorMessage << "No element \'city\' in element  \'address\' in response or not a string type." << Endl;
    }

    if (addressJsonValue.Has("street") && addressJsonValue["street"].IsString()) {
        addressProtoValue->SetStreet(addressJsonValue["street"].GetString());
    } else {
        errorMessage << "No element \'street\' in element  \'address\' in response or not a string type." << Endl;
    }

    if (addressJsonValue.Has("house") && addressJsonValue["house"].IsString()) {
        addressProtoValue->SetHouse(addressJsonValue["house"].GetString());
    } else {
        errorMessage << "No element \'house\' in element  \'address\' in response or not a string type." << Endl;
    }

    if (addressJsonValue.Has("entrance") && addressJsonValue["entrance"].IsString()) {
        addressProtoValue->SetEntrance(addressJsonValue["entrance"].GetString());
    } else {
        errorMessage << "No element \'entrance\' in element  \'address\' in response or not a string type." << Endl;
    }
}

void ConvertOrder(const NJson::TJsonValue& orderJsonValue, TStringBuilder& errorMessage ,NAlice::NOrder::TOrder* orderProtoVariable) {
    if (orderJsonValue.Has("id") && orderJsonValue["id"].IsString()) {
        orderProtoVariable->SetId(orderJsonValue["id"].GetString());
    } else {
        errorMessage << "No element \'id\' in response or not a string type." << Endl;
    }

    if (orderJsonValue.Has("created_at") && orderJsonValue["created_at"].IsString()) {
        orderProtoVariable->SetCreatedDate(orderJsonValue["created_at"].GetString());
    } else {
        errorMessage << "No element \'created_at\' in response or not a string type." << Endl;
    }

    if (orderJsonValue.Has("delivery_eta_min") && orderJsonValue["delivery_eta_min"].IsInteger()) {
        orderProtoVariable->SetEtaSeconds(orderJsonValue["delivery_eta_min"].GetInteger() * 60);
    } else {
        errorMessage << "No element \'delivery_eta_min\' in response or not an integer type." << Endl;
    }

    if (orderJsonValue.Has("status") && orderJsonValue["status"].IsString()) {
        if (auto status = statusMapper.FindPtr(orderJsonValue["status"].GetString())) {
            orderProtoVariable->SetStatus(*status);
        } else {
            errorMessage << "Unknown type of \'status\' variable in response. Using default value." << Endl;
            orderProtoVariable->SetStatus(NAlice::NOrder::TOrder::UnknownStatus);
        }
    } else {
        errorMessage << "No element \'status\' in response or not a string type." << Endl;
    }

    if (orderJsonValue.Has("price") && orderJsonValue["price"].IsDouble()) {
        orderProtoVariable->MutablePrice()->SetValue(orderJsonValue["price"].GetDouble());
        orderProtoVariable->MutablePrice()->SetCurrency(NAlice::NOrder::TOrder::Rub);
    } else {
        errorMessage << "No element \'price\' in response or not a double type." << Endl;
    }

    if (orderJsonValue.Has("address")) {

        ConvertAddress(orderJsonValue["address"], errorMessage, orderProtoVariable->MutableTargetAddress());

        if (orderJsonValue.Has("location") && orderJsonValue["location"]["lat"].IsDouble() && orderJsonValue["location"]["lon"].IsDouble()) {
            orderProtoVariable->MutableTargetAddress()->MutableLatLon()->SetLatitude(orderJsonValue["location"]["lat"].GetDouble());
            orderProtoVariable->MutableTargetAddress()->MutableLatLon()->SetLongitude(orderJsonValue["location"]["lon"].GetDouble());
        } else {
            errorMessage << "No element \'location\' in response or not a double type." << Endl;
        }
    } else {
        errorMessage << "No element \'address\' in response." << Endl;
    }

    for (const NJson::TJsonValue& itemJsonValue : orderJsonValue["items"].GetArray()) {
        NAlice::NOrder::TOrder::TItem* itemProtoVariable = orderProtoVariable->AddItems();
        ConvertItem(itemJsonValue, errorMessage, itemProtoVariable);
    }

    if (orderJsonValue.Has("delivery_type") && orderJsonValue["delivery_type"].IsString()) {
        if (auto deliveryType = deliveryTypeMappper.FindPtr(orderJsonValue["delivery_type"].GetString())) {
            orderProtoVariable->MutableDeliveryType()->SetDeliveryType(*deliveryType);
        } else {
            errorMessage << "Unknown type of \'delivery_type\' variable in response. Using default value." << Endl;
            orderProtoVariable->MutableDeliveryType()->SetDeliveryType(NAlice::NOrder::TOrder::Curier);
        }
    } else {
        errorMessage << "No element \'delivery_type\' in response or not a string type." << Endl;
    }
}

NAlice::NOrder::TProviderOrderResponse ParseLavkaResponse(const NJson::TJsonValue& jsonResponse, const TRunRequest& request) {

    NAlice::NOrder::TProviderOrderResponse protoResult;
    protoResult.SetProviderName("lavka");

    TStringBuilder errorMessage;

    NJson::TJsonValue::TArray ordersList = jsonResponse["orders"].GetArray();
    if(ordersList.size() == 0) {
        return protoResult;
    }

    for (const NJson::TJsonValue& orderJsonValue : ordersList) {
        NAlice::NOrder::TOrder* orderProtoVariable = protoResult.AddOrders();
        ConvertOrder(orderJsonValue, errorMessage, orderProtoVariable);
    }

    NMetrics::ISensors& sensors = request.System().GetSensors();
    NMonitoring::TLabels label{
        {"scenario_name", "order"},
        {"name", "lavka_response_convertor_result"},
    };

    if (!errorMessage.empty()) {
        label.Add("subname", "error_in_response");
        LOG_ERROR(request.Debug().Logger()) << errorMessage;
    } else {
        label.Add("subname", "correct_response");
    }
    sensors.IncRate(label);
    return protoResult;
}

}