/* eslint-disable */
import { util, configure, Writer, Reader } from "protobufjs/minimal";
import * as Long from "long";
import { TLatLon } from "../../../../../alice/protos/data/lat_lon";

export const protobufPackage = "NAlice.NOrder";

export enum ENotificationType {
  EN_UNDEFINED = 0,
  EN_COURIER_IS_COMMING = 1,
  EN_ORDER_BEING_LATE = 2,
  EN_ORDER_CANCELED = 3,
  EN_ORDER_UNDER_THE_DOOR = 4,
  UNRECOGNIZED = -1,
}

export function eNotificationTypeFromJSON(object: any): ENotificationType {
  switch (object) {
    case 0:
    case "EN_UNDEFINED":
      return ENotificationType.EN_UNDEFINED;
    case 1:
    case "EN_COURIER_IS_COMMING":
      return ENotificationType.EN_COURIER_IS_COMMING;
    case 2:
    case "EN_ORDER_BEING_LATE":
      return ENotificationType.EN_ORDER_BEING_LATE;
    case 3:
    case "EN_ORDER_CANCELED":
      return ENotificationType.EN_ORDER_CANCELED;
    case 4:
    case "EN_ORDER_UNDER_THE_DOOR":
      return ENotificationType.EN_ORDER_UNDER_THE_DOOR;
    case -1:
    case "UNRECOGNIZED":
    default:
      return ENotificationType.UNRECOGNIZED;
  }
}

export function eNotificationTypeToJSON(object: ENotificationType): string {
  switch (object) {
    case ENotificationType.EN_UNDEFINED:
      return "EN_UNDEFINED";
    case ENotificationType.EN_COURIER_IS_COMMING:
      return "EN_COURIER_IS_COMMING";
    case ENotificationType.EN_ORDER_BEING_LATE:
      return "EN_ORDER_BEING_LATE";
    case ENotificationType.EN_ORDER_CANCELED:
      return "EN_ORDER_CANCELED";
    case ENotificationType.EN_ORDER_UNDER_THE_DOOR:
      return "EN_ORDER_UNDER_THE_DOOR";
    default:
      return "UNKNOWN";
  }
}

export interface TOrder {
  Id: string;
  CreatedDate: string;
  EtaSeconds: number | undefined;
  EtaDateRange?: TOrder_TDateRange | undefined;
  Status: TOrder_EStatus;
  Price?: TOrder_TPrice;
  TargetAddress?: TOrder_TAddress;
  Items: TOrder_TItem[];
  DeliveryType?: TOrder_TDelivery;
}

export enum TOrder_EStatus {
  UnknownStatus = 0,
  Created = 1,
  Assembling = 2,
  Assembled = 3,
  PerformerFound = 4,
  Delivering = 5,
  DeliveryArrived = 6,
  Succeeded = 7,
  Canceled = 8,
  Failed = 9,
  UNRECOGNIZED = -1,
}

export function tOrder_EStatusFromJSON(object: any): TOrder_EStatus {
  switch (object) {
    case 0:
    case "UnknownStatus":
      return TOrder_EStatus.UnknownStatus;
    case 1:
    case "Created":
      return TOrder_EStatus.Created;
    case 2:
    case "Assembling":
      return TOrder_EStatus.Assembling;
    case 3:
    case "Assembled":
      return TOrder_EStatus.Assembled;
    case 4:
    case "PerformerFound":
      return TOrder_EStatus.PerformerFound;
    case 5:
    case "Delivering":
      return TOrder_EStatus.Delivering;
    case 6:
    case "DeliveryArrived":
      return TOrder_EStatus.DeliveryArrived;
    case 7:
    case "Succeeded":
      return TOrder_EStatus.Succeeded;
    case 8:
    case "Canceled":
      return TOrder_EStatus.Canceled;
    case 9:
    case "Failed":
      return TOrder_EStatus.Failed;
    case -1:
    case "UNRECOGNIZED":
    default:
      return TOrder_EStatus.UNRECOGNIZED;
  }
}

export function tOrder_EStatusToJSON(object: TOrder_EStatus): string {
  switch (object) {
    case TOrder_EStatus.UnknownStatus:
      return "UnknownStatus";
    case TOrder_EStatus.Created:
      return "Created";
    case TOrder_EStatus.Assembling:
      return "Assembling";
    case TOrder_EStatus.Assembled:
      return "Assembled";
    case TOrder_EStatus.PerformerFound:
      return "PerformerFound";
    case TOrder_EStatus.Delivering:
      return "Delivering";
    case TOrder_EStatus.DeliveryArrived:
      return "DeliveryArrived";
    case TOrder_EStatus.Succeeded:
      return "Succeeded";
    case TOrder_EStatus.Canceled:
      return "Canceled";
    case TOrder_EStatus.Failed:
      return "Failed";
    default:
      return "UNKNOWN";
  }
}

export enum TOrder_ECurrency {
  UnknownCurrency = 0,
  Rub = 1,
  UNRECOGNIZED = -1,
}

export function tOrder_ECurrencyFromJSON(object: any): TOrder_ECurrency {
  switch (object) {
    case 0:
    case "UnknownCurrency":
      return TOrder_ECurrency.UnknownCurrency;
    case 1:
    case "Rub":
      return TOrder_ECurrency.Rub;
    case -1:
    case "UNRECOGNIZED":
    default:
      return TOrder_ECurrency.UNRECOGNIZED;
  }
}

export function tOrder_ECurrencyToJSON(object: TOrder_ECurrency): string {
  switch (object) {
    case TOrder_ECurrency.UnknownCurrency:
      return "UnknownCurrency";
    case TOrder_ECurrency.Rub:
      return "Rub";
    default:
      return "UNKNOWN";
  }
}

export enum TOrder_EDeliveryType {
  UnknownDeliveryType = 0,
  FootCurier = 1,
  AutoCurier = 2,
  Curier = 7,
  Rover = 3,
  Self = 4,
  SelfPostamat = 5,
  /** SelfPickupPoint - next free value: 8 */
  SelfPickupPoint = 6,
  UNRECOGNIZED = -1,
}

export function tOrder_EDeliveryTypeFromJSON(
  object: any
): TOrder_EDeliveryType {
  switch (object) {
    case 0:
    case "UnknownDeliveryType":
      return TOrder_EDeliveryType.UnknownDeliveryType;
    case 1:
    case "FootCurier":
      return TOrder_EDeliveryType.FootCurier;
    case 2:
    case "AutoCurier":
      return TOrder_EDeliveryType.AutoCurier;
    case 7:
    case "Curier":
      return TOrder_EDeliveryType.Curier;
    case 3:
    case "Rover":
      return TOrder_EDeliveryType.Rover;
    case 4:
    case "Self":
      return TOrder_EDeliveryType.Self;
    case 5:
    case "SelfPostamat":
      return TOrder_EDeliveryType.SelfPostamat;
    case 6:
    case "SelfPickupPoint":
      return TOrder_EDeliveryType.SelfPickupPoint;
    case -1:
    case "UNRECOGNIZED":
    default:
      return TOrder_EDeliveryType.UNRECOGNIZED;
  }
}

export function tOrder_EDeliveryTypeToJSON(
  object: TOrder_EDeliveryType
): string {
  switch (object) {
    case TOrder_EDeliveryType.UnknownDeliveryType:
      return "UnknownDeliveryType";
    case TOrder_EDeliveryType.FootCurier:
      return "FootCurier";
    case TOrder_EDeliveryType.AutoCurier:
      return "AutoCurier";
    case TOrder_EDeliveryType.Curier:
      return "Curier";
    case TOrder_EDeliveryType.Rover:
      return "Rover";
    case TOrder_EDeliveryType.Self:
      return "Self";
    case TOrder_EDeliveryType.SelfPostamat:
      return "SelfPostamat";
    case TOrder_EDeliveryType.SelfPickupPoint:
      return "SelfPickupPoint";
    default:
      return "UNKNOWN";
  }
}

export interface TOrder_TContacts {
  Phone: string;
  Auto: string;
}

export interface TOrder_TAddress {
  LatLon?: TLatLon;
  Country: string;
  City: string;
  Street: string;
  House: string;
  Entrance: string;
}

export interface TOrder_TDelivery {
  DeliveryType: TOrder_EDeliveryType;
  CurierContacts?: TOrder_TContacts;
  CurierLatLon?: TLatLon;
}

export interface TOrder_TPrice {
  Value: number;
  Currency: TOrder_ECurrency;
}

export interface TOrder_TItem {
  Id: string;
  Title: string;
  Quantity: number;
  ActualPrice?: TOrder_TPrice;
  FullPrice?: TOrder_TPrice;
}

export interface TOrder_TDateRange {
  FromDate: string;
  ToDate: string;
}

export interface TProviderOrderResponse {
  ProviderName: string;
  Orders: TOrder[];
}

function createBaseTOrder(): TOrder {
  return {
    Id: "",
    CreatedDate: "",
    EtaSeconds: undefined,
    EtaDateRange: undefined,
    Status: 0,
    Price: undefined,
    TargetAddress: undefined,
    Items: [],
    DeliveryType: undefined,
  };
}

export const TOrder = {
  encode(message: TOrder, writer: Writer = Writer.create()): Writer {
    if (message.Id !== "") {
      writer.uint32(10).string(message.Id);
    }
    if (message.CreatedDate !== "") {
      writer.uint32(18).string(message.CreatedDate);
    }
    if (message.EtaSeconds !== undefined) {
      writer.uint32(24).int32(message.EtaSeconds);
    }
    if (message.EtaDateRange !== undefined) {
      TOrder_TDateRange.encode(
        message.EtaDateRange,
        writer.uint32(34).fork()
      ).ldelim();
    }
    if (message.Status !== 0) {
      writer.uint32(40).int32(message.Status);
    }
    if (message.Price !== undefined) {
      TOrder_TPrice.encode(message.Price, writer.uint32(50).fork()).ldelim();
    }
    if (message.TargetAddress !== undefined) {
      TOrder_TAddress.encode(
        message.TargetAddress,
        writer.uint32(58).fork()
      ).ldelim();
    }
    for (const v of message.Items) {
      TOrder_TItem.encode(v!, writer.uint32(66).fork()).ldelim();
    }
    if (message.DeliveryType !== undefined) {
      TOrder_TDelivery.encode(
        message.DeliveryType,
        writer.uint32(74).fork()
      ).ldelim();
    }
    return writer;
  },

  decode(input: Reader | Uint8Array, length?: number): TOrder {
    const reader = input instanceof Reader ? input : new Reader(input);
    let end = length === undefined ? reader.len : reader.pos + length;
    const message = createBaseTOrder();
    while (reader.pos < end) {
      const tag = reader.uint32();
      switch (tag >>> 3) {
        case 1:
          message.Id = reader.string();
          break;
        case 2:
          message.CreatedDate = reader.string();
          break;
        case 3:
          message.EtaSeconds = reader.int32();
          break;
        case 4:
          message.EtaDateRange = TOrder_TDateRange.decode(
            reader,
            reader.uint32()
          );
          break;
        case 5:
          message.Status = reader.int32() as any;
          break;
        case 6:
          message.Price = TOrder_TPrice.decode(reader, reader.uint32());
          break;
        case 7:
          message.TargetAddress = TOrder_TAddress.decode(
            reader,
            reader.uint32()
          );
          break;
        case 8:
          message.Items.push(TOrder_TItem.decode(reader, reader.uint32()));
          break;
        case 9:
          message.DeliveryType = TOrder_TDelivery.decode(
            reader,
            reader.uint32()
          );
          break;
        default:
          reader.skipType(tag & 7);
          break;
      }
    }
    return message;
  },

  fromJSON(object: any): TOrder {
    return {
      Id: isSet(object.id) ? String(object.id) : "",
      CreatedDate: isSet(object.created_date)
        ? String(object.created_date)
        : "",
      EtaSeconds: isSet(object.eta_seconds)
        ? Number(object.eta_seconds)
        : undefined,
      EtaDateRange: isSet(object.eta_date_range)
        ? TOrder_TDateRange.fromJSON(object.eta_date_range)
        : undefined,
      Status: isSet(object.status) ? tOrder_EStatusFromJSON(object.status) : 0,
      Price: isSet(object.price)
        ? TOrder_TPrice.fromJSON(object.price)
        : undefined,
      TargetAddress: isSet(object.target_address)
        ? TOrder_TAddress.fromJSON(object.target_address)
        : undefined,
      Items: Array.isArray(object?.items)
        ? object.items.map((e: any) => TOrder_TItem.fromJSON(e))
        : [],
      DeliveryType: isSet(object.delivery_type)
        ? TOrder_TDelivery.fromJSON(object.delivery_type)
        : undefined,
    };
  },

  toJSON(message: TOrder): unknown {
    const obj: any = {};
    message.Id !== undefined && (obj.id = message.Id);
    message.CreatedDate !== undefined &&
      (obj.created_date = message.CreatedDate);
    message.EtaSeconds !== undefined &&
      (obj.eta_seconds = Math.round(message.EtaSeconds));
    message.EtaDateRange !== undefined &&
      (obj.eta_date_range = message.EtaDateRange
        ? TOrder_TDateRange.toJSON(message.EtaDateRange)
        : undefined);
    message.Status !== undefined &&
      (obj.status = tOrder_EStatusToJSON(message.Status));
    message.Price !== undefined &&
      (obj.price = message.Price
        ? TOrder_TPrice.toJSON(message.Price)
        : undefined);
    message.TargetAddress !== undefined &&
      (obj.target_address = message.TargetAddress
        ? TOrder_TAddress.toJSON(message.TargetAddress)
        : undefined);
    if (message.Items) {
      obj.items = message.Items.map((e) =>
        e ? TOrder_TItem.toJSON(e) : undefined
      );
    } else {
      obj.items = [];
    }
    message.DeliveryType !== undefined &&
      (obj.delivery_type = message.DeliveryType
        ? TOrder_TDelivery.toJSON(message.DeliveryType)
        : undefined);
    return obj;
  },
};

function createBaseTOrder_TContacts(): TOrder_TContacts {
  return { Phone: "", Auto: "" };
}

export const TOrder_TContacts = {
  encode(message: TOrder_TContacts, writer: Writer = Writer.create()): Writer {
    if (message.Phone !== "") {
      writer.uint32(10).string(message.Phone);
    }
    if (message.Auto !== "") {
      writer.uint32(18).string(message.Auto);
    }
    return writer;
  },

  decode(input: Reader | Uint8Array, length?: number): TOrder_TContacts {
    const reader = input instanceof Reader ? input : new Reader(input);
    let end = length === undefined ? reader.len : reader.pos + length;
    const message = createBaseTOrder_TContacts();
    while (reader.pos < end) {
      const tag = reader.uint32();
      switch (tag >>> 3) {
        case 1:
          message.Phone = reader.string();
          break;
        case 2:
          message.Auto = reader.string();
          break;
        default:
          reader.skipType(tag & 7);
          break;
      }
    }
    return message;
  },

  fromJSON(object: any): TOrder_TContacts {
    return {
      Phone: isSet(object.phone) ? String(object.phone) : "",
      Auto: isSet(object.auto) ? String(object.auto) : "",
    };
  },

  toJSON(message: TOrder_TContacts): unknown {
    const obj: any = {};
    message.Phone !== undefined && (obj.phone = message.Phone);
    message.Auto !== undefined && (obj.auto = message.Auto);
    return obj;
  },
};

function createBaseTOrder_TAddress(): TOrder_TAddress {
  return {
    LatLon: undefined,
    Country: "",
    City: "",
    Street: "",
    House: "",
    Entrance: "",
  };
}

export const TOrder_TAddress = {
  encode(message: TOrder_TAddress, writer: Writer = Writer.create()): Writer {
    if (message.LatLon !== undefined) {
      TLatLon.encode(message.LatLon, writer.uint32(10).fork()).ldelim();
    }
    if (message.Country !== "") {
      writer.uint32(18).string(message.Country);
    }
    if (message.City !== "") {
      writer.uint32(26).string(message.City);
    }
    if (message.Street !== "") {
      writer.uint32(34).string(message.Street);
    }
    if (message.House !== "") {
      writer.uint32(42).string(message.House);
    }
    if (message.Entrance !== "") {
      writer.uint32(50).string(message.Entrance);
    }
    return writer;
  },

  decode(input: Reader | Uint8Array, length?: number): TOrder_TAddress {
    const reader = input instanceof Reader ? input : new Reader(input);
    let end = length === undefined ? reader.len : reader.pos + length;
    const message = createBaseTOrder_TAddress();
    while (reader.pos < end) {
      const tag = reader.uint32();
      switch (tag >>> 3) {
        case 1:
          message.LatLon = TLatLon.decode(reader, reader.uint32());
          break;
        case 2:
          message.Country = reader.string();
          break;
        case 3:
          message.City = reader.string();
          break;
        case 4:
          message.Street = reader.string();
          break;
        case 5:
          message.House = reader.string();
          break;
        case 6:
          message.Entrance = reader.string();
          break;
        default:
          reader.skipType(tag & 7);
          break;
      }
    }
    return message;
  },

  fromJSON(object: any): TOrder_TAddress {
    return {
      LatLon: isSet(object.lat_lon)
        ? TLatLon.fromJSON(object.lat_lon)
        : undefined,
      Country: isSet(object.country) ? String(object.country) : "",
      City: isSet(object.city) ? String(object.city) : "",
      Street: isSet(object.street) ? String(object.street) : "",
      House: isSet(object.house) ? String(object.house) : "",
      Entrance: isSet(object.entrance) ? String(object.entrance) : "",
    };
  },

  toJSON(message: TOrder_TAddress): unknown {
    const obj: any = {};
    message.LatLon !== undefined &&
      (obj.lat_lon = message.LatLon
        ? TLatLon.toJSON(message.LatLon)
        : undefined);
    message.Country !== undefined && (obj.country = message.Country);
    message.City !== undefined && (obj.city = message.City);
    message.Street !== undefined && (obj.street = message.Street);
    message.House !== undefined && (obj.house = message.House);
    message.Entrance !== undefined && (obj.entrance = message.Entrance);
    return obj;
  },
};

function createBaseTOrder_TDelivery(): TOrder_TDelivery {
  return {
    DeliveryType: 0,
    CurierContacts: undefined,
    CurierLatLon: undefined,
  };
}

export const TOrder_TDelivery = {
  encode(message: TOrder_TDelivery, writer: Writer = Writer.create()): Writer {
    if (message.DeliveryType !== 0) {
      writer.uint32(8).int32(message.DeliveryType);
    }
    if (message.CurierContacts !== undefined) {
      TOrder_TContacts.encode(
        message.CurierContacts,
        writer.uint32(18).fork()
      ).ldelim();
    }
    if (message.CurierLatLon !== undefined) {
      TLatLon.encode(message.CurierLatLon, writer.uint32(26).fork()).ldelim();
    }
    return writer;
  },

  decode(input: Reader | Uint8Array, length?: number): TOrder_TDelivery {
    const reader = input instanceof Reader ? input : new Reader(input);
    let end = length === undefined ? reader.len : reader.pos + length;
    const message = createBaseTOrder_TDelivery();
    while (reader.pos < end) {
      const tag = reader.uint32();
      switch (tag >>> 3) {
        case 1:
          message.DeliveryType = reader.int32() as any;
          break;
        case 2:
          message.CurierContacts = TOrder_TContacts.decode(
            reader,
            reader.uint32()
          );
          break;
        case 3:
          message.CurierLatLon = TLatLon.decode(reader, reader.uint32());
          break;
        default:
          reader.skipType(tag & 7);
          break;
      }
    }
    return message;
  },

  fromJSON(object: any): TOrder_TDelivery {
    return {
      DeliveryType: isSet(object.delivery_type)
        ? tOrder_EDeliveryTypeFromJSON(object.delivery_type)
        : 0,
      CurierContacts: isSet(object.curier_contacts)
        ? TOrder_TContacts.fromJSON(object.curier_contacts)
        : undefined,
      CurierLatLon: isSet(object.curier_lat_lon)
        ? TLatLon.fromJSON(object.curier_lat_lon)
        : undefined,
    };
  },

  toJSON(message: TOrder_TDelivery): unknown {
    const obj: any = {};
    message.DeliveryType !== undefined &&
      (obj.delivery_type = tOrder_EDeliveryTypeToJSON(message.DeliveryType));
    message.CurierContacts !== undefined &&
      (obj.curier_contacts = message.CurierContacts
        ? TOrder_TContacts.toJSON(message.CurierContacts)
        : undefined);
    message.CurierLatLon !== undefined &&
      (obj.curier_lat_lon = message.CurierLatLon
        ? TLatLon.toJSON(message.CurierLatLon)
        : undefined);
    return obj;
  },
};

function createBaseTOrder_TPrice(): TOrder_TPrice {
  return { Value: 0, Currency: 0 };
}

export const TOrder_TPrice = {
  encode(message: TOrder_TPrice, writer: Writer = Writer.create()): Writer {
    if (message.Value !== 0) {
      writer.uint32(9).double(message.Value);
    }
    if (message.Currency !== 0) {
      writer.uint32(16).int32(message.Currency);
    }
    return writer;
  },

  decode(input: Reader | Uint8Array, length?: number): TOrder_TPrice {
    const reader = input instanceof Reader ? input : new Reader(input);
    let end = length === undefined ? reader.len : reader.pos + length;
    const message = createBaseTOrder_TPrice();
    while (reader.pos < end) {
      const tag = reader.uint32();
      switch (tag >>> 3) {
        case 1:
          message.Value = reader.double();
          break;
        case 2:
          message.Currency = reader.int32() as any;
          break;
        default:
          reader.skipType(tag & 7);
          break;
      }
    }
    return message;
  },

  fromJSON(object: any): TOrder_TPrice {
    return {
      Value: isSet(object.value) ? Number(object.value) : 0,
      Currency: isSet(object.currency)
        ? tOrder_ECurrencyFromJSON(object.currency)
        : 0,
    };
  },

  toJSON(message: TOrder_TPrice): unknown {
    const obj: any = {};
    message.Value !== undefined && (obj.value = message.Value);
    message.Currency !== undefined &&
      (obj.currency = tOrder_ECurrencyToJSON(message.Currency));
    return obj;
  },
};

function createBaseTOrder_TItem(): TOrder_TItem {
  return {
    Id: "",
    Title: "",
    Quantity: 0,
    ActualPrice: undefined,
    FullPrice: undefined,
  };
}

export const TOrder_TItem = {
  encode(message: TOrder_TItem, writer: Writer = Writer.create()): Writer {
    if (message.Id !== "") {
      writer.uint32(10).string(message.Id);
    }
    if (message.Title !== "") {
      writer.uint32(18).string(message.Title);
    }
    if (message.Quantity !== 0) {
      writer.uint32(33).double(message.Quantity);
    }
    if (message.ActualPrice !== undefined) {
      TOrder_TPrice.encode(
        message.ActualPrice,
        writer.uint32(42).fork()
      ).ldelim();
    }
    if (message.FullPrice !== undefined) {
      TOrder_TPrice.encode(
        message.FullPrice,
        writer.uint32(50).fork()
      ).ldelim();
    }
    return writer;
  },

  decode(input: Reader | Uint8Array, length?: number): TOrder_TItem {
    const reader = input instanceof Reader ? input : new Reader(input);
    let end = length === undefined ? reader.len : reader.pos + length;
    const message = createBaseTOrder_TItem();
    while (reader.pos < end) {
      const tag = reader.uint32();
      switch (tag >>> 3) {
        case 1:
          message.Id = reader.string();
          break;
        case 2:
          message.Title = reader.string();
          break;
        case 4:
          message.Quantity = reader.double();
          break;
        case 5:
          message.ActualPrice = TOrder_TPrice.decode(reader, reader.uint32());
          break;
        case 6:
          message.FullPrice = TOrder_TPrice.decode(reader, reader.uint32());
          break;
        default:
          reader.skipType(tag & 7);
          break;
      }
    }
    return message;
  },

  fromJSON(object: any): TOrder_TItem {
    return {
      Id: isSet(object.id) ? String(object.id) : "",
      Title: isSet(object.title) ? String(object.title) : "",
      Quantity: isSet(object.quantity) ? Number(object.quantity) : 0,
      ActualPrice: isSet(object.actual_price)
        ? TOrder_TPrice.fromJSON(object.actual_price)
        : undefined,
      FullPrice: isSet(object.full_price)
        ? TOrder_TPrice.fromJSON(object.full_price)
        : undefined,
    };
  },

  toJSON(message: TOrder_TItem): unknown {
    const obj: any = {};
    message.Id !== undefined && (obj.id = message.Id);
    message.Title !== undefined && (obj.title = message.Title);
    message.Quantity !== undefined && (obj.quantity = message.Quantity);
    message.ActualPrice !== undefined &&
      (obj.actual_price = message.ActualPrice
        ? TOrder_TPrice.toJSON(message.ActualPrice)
        : undefined);
    message.FullPrice !== undefined &&
      (obj.full_price = message.FullPrice
        ? TOrder_TPrice.toJSON(message.FullPrice)
        : undefined);
    return obj;
  },
};

function createBaseTOrder_TDateRange(): TOrder_TDateRange {
  return { FromDate: "", ToDate: "" };
}

export const TOrder_TDateRange = {
  encode(message: TOrder_TDateRange, writer: Writer = Writer.create()): Writer {
    if (message.FromDate !== "") {
      writer.uint32(18).string(message.FromDate);
    }
    if (message.ToDate !== "") {
      writer.uint32(26).string(message.ToDate);
    }
    return writer;
  },

  decode(input: Reader | Uint8Array, length?: number): TOrder_TDateRange {
    const reader = input instanceof Reader ? input : new Reader(input);
    let end = length === undefined ? reader.len : reader.pos + length;
    const message = createBaseTOrder_TDateRange();
    while (reader.pos < end) {
      const tag = reader.uint32();
      switch (tag >>> 3) {
        case 2:
          message.FromDate = reader.string();
          break;
        case 3:
          message.ToDate = reader.string();
          break;
        default:
          reader.skipType(tag & 7);
          break;
      }
    }
    return message;
  },

  fromJSON(object: any): TOrder_TDateRange {
    return {
      FromDate: isSet(object.from_date) ? String(object.from_date) : "",
      ToDate: isSet(object.to_date) ? String(object.to_date) : "",
    };
  },

  toJSON(message: TOrder_TDateRange): unknown {
    const obj: any = {};
    message.FromDate !== undefined && (obj.from_date = message.FromDate);
    message.ToDate !== undefined && (obj.to_date = message.ToDate);
    return obj;
  },
};

function createBaseTProviderOrderResponse(): TProviderOrderResponse {
  return { ProviderName: "", Orders: [] };
}

export const TProviderOrderResponse = {
  encode(
    message: TProviderOrderResponse,
    writer: Writer = Writer.create()
  ): Writer {
    if (message.ProviderName !== "") {
      writer.uint32(10).string(message.ProviderName);
    }
    for (const v of message.Orders) {
      TOrder.encode(v!, writer.uint32(18).fork()).ldelim();
    }
    return writer;
  },

  decode(input: Reader | Uint8Array, length?: number): TProviderOrderResponse {
    const reader = input instanceof Reader ? input : new Reader(input);
    let end = length === undefined ? reader.len : reader.pos + length;
    const message = createBaseTProviderOrderResponse();
    while (reader.pos < end) {
      const tag = reader.uint32();
      switch (tag >>> 3) {
        case 1:
          message.ProviderName = reader.string();
          break;
        case 2:
          message.Orders.push(TOrder.decode(reader, reader.uint32()));
          break;
        default:
          reader.skipType(tag & 7);
          break;
      }
    }
    return message;
  },

  fromJSON(object: any): TProviderOrderResponse {
    return {
      ProviderName: isSet(object.provider_name)
        ? String(object.provider_name)
        : "",
      Orders: Array.isArray(object?.orders)
        ? object.orders.map((e: any) => TOrder.fromJSON(e))
        : [],
    };
  },

  toJSON(message: TProviderOrderResponse): unknown {
    const obj: any = {};
    message.ProviderName !== undefined &&
      (obj.provider_name = message.ProviderName);
    if (message.Orders) {
      obj.orders = message.Orders.map((e) =>
        e ? TOrder.toJSON(e) : undefined
      );
    } else {
      obj.orders = [];
    }
    return obj;
  },
};

// If you get a compile-error about 'Constructor<Long> and ... have no overlap',
// add '--ts_proto_opt=esModuleInterop=true' as a flag when calling 'protoc'.
if (util.Long !== Long) {
  util.Long = Long as any;
  configure();
}

function isSet(value: any): boolean {
  return value !== null && value !== undefined;
}
