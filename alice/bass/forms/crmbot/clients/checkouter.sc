namespace NBassApi::Crmbot;

struct TPager {
    from: ui64;
    to: ui64;
    total: ui64;
    page: ui64;
    pageSize: ui64;
    pagesCount: ui64;
};

struct TCheckouterBuyer {
    uid: ui64;
    muid: ui64;
    regionId: ui64;
    firstName: string;
    lastName: string;
    middleName: string;
    phone: string;
    email: string;
};

struct TCheckouterTrackCheckpoint {
    status: string (required);
    deliveryStatus: i32 (required);
    date: string;
};

struct TCheckouterTrack {
    checkpoints: [TCheckouterTrackCheckpoint];
    deliveryServiceId: ui32 (required);
    deliveryServiceType: string;
};

struct TCheckouterParcel {
    delayedShipmentDate: string;
    deliveredAt: string;
    fromDate: string;
    shipmentDate: string;
    shipmentId: ui64;
    supplierShipmentDateTime: string;
    toDate: string;
    tracks: [TCheckouterTrack];
    status: string;
};

struct TCheckouterDeliveryDates {
    fromDate: string (required);
    toDate: string (required);
    fromTime: string;
    toTime: string;
};

struct TCheckouterOutletPhone {
    cityCode: string;
    countryCode: string;
    number: string;
    extNumber: string;
};

struct TCheckouterOutlet {
    name: string;
    notes: string;
    phones: [TCheckouterOutletPhone];
    postcode: string;
    city: string;
    street: string;
    block: string;
    building: string;
    house: string;
    estate: string;
};

struct TCheckouterAddress {
    country: string (required);
    postcode: string;
    city: string (required);
    subway: string;
    street: string;
    house: string;
    block: string;
    entrance: string;
    entryphone: string;
    floor: string;
    apartment: string;
    phone: string (required);
    recipientName: struct {
        firstName: string;
        secondName: string;
        middleName: string;
    };
};

struct TCheckouterDelivery {
    type: string (required);
    dates: TCheckouterDeliveryDates (required);
    parcels: [TCheckouterParcel] (required);
    regionId: ui64 (required);
    outlet: TCheckouterOutlet;
    buyerAddress: TCheckouterAddress;
};

struct TCheckouterCancellationRequest {
    notes: string;
    requestStatus: string;
    substatus: string;
    substatusText: string;
};

struct TCheckouterPaymentPartition {
    paymentAgent: string;
};

struct TCheckouterPayment {
    partitions: [TCheckouterPaymentPartition];
};

struct TCheckouterOrderInfo {
    id: ui64 (required);
    status: string (required);
    substatus: string;
    buyer: TCheckouterBuyer (required);
    delivery: TCheckouterDelivery (required);
    fulfilment: bool (required);
    creationDate: string (required);
    cancellationRequest: TCheckouterCancellationRequest;
    paymentMethod: string (required);
    payment: TCheckouterPayment;
};

struct TCheckouterOrderInfos {
    orders: [TCheckouterOrderInfo] (required);
    pager: TPager;
};

struct TCheckouterOrderEvent {
    tranDate: string;
    reason: string;
    type: string;
    orderBefore: TCheckouterOrderInfo;
    orderAfter: TCheckouterOrderInfo;
};

struct TCheckouterOrderEvents {
    events: [TCheckouterOrderEvent] (required);
    pager: TPager;
};

struct TCheckouterCancellationSubstatus {
    substatus: string (required);
    text: string (required);
};

struct TCheckouterCancellationRule {
    status: string (required);
    substatuses: [TCheckouterCancellationSubstatus] (required);
};

struct TCheckouterCancellationRules {
    cancellationRules: [TCheckouterCancellationRule] (required);
};
