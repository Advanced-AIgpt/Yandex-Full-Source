/* eslint-disable */
import { util, configure } from "protobufjs/minimal";
import * as Long from "long";

export const protobufPackage = "NAlice";

/**
 * Languages supported by Megamind.
 *
 * Languages from this list are specified
 * in scenario configs to filter out scenarios which don't support
 * the language of the phrase being processed.
 *
 * See https://a.yandex-team.ru/arc/trunk/arcadia/library/cpp/langs/langs.h
 * Not named as ELanguage to prevent clashing with ELanguage defined in the
 * above file. Same reason for distinct enum value name convention: L_{LANG_CODE}.
 */
export enum ELang {
  /** L_UNK - Zero value is required by protobuf */
  L_UNK = 0,
  /** L_RUS - Russian */
  L_RUS = 1,
  /** L_ENG - English */
  L_ENG = 2,
  /** L_TUR - Turkish */
  L_TUR = 44,
  /** L_ARA - Arabic */
  L_ARA = 55,
  UNRECOGNIZED = -1,
}

export function eLangFromJSON(object: any): ELang {
  switch (object) {
    case 0:
    case "L_UNK":
      return ELang.L_UNK;
    case 1:
    case "L_RUS":
      return ELang.L_RUS;
    case 2:
    case "L_ENG":
      return ELang.L_ENG;
    case 44:
    case "L_TUR":
      return ELang.L_TUR;
    case 55:
    case "L_ARA":
      return ELang.L_ARA;
    case -1:
    case "UNRECOGNIZED":
    default:
      return ELang.UNRECOGNIZED;
  }
}

export function eLangToJSON(object: ELang): string {
  switch (object) {
    case ELang.L_UNK:
      return "L_UNK";
    case ELang.L_RUS:
      return "L_RUS";
    case ELang.L_ENG:
      return "L_ENG";
    case ELang.L_TUR:
      return "L_TUR";
    case ELang.L_ARA:
      return "L_ARA";
    default:
      return "UNKNOWN";
  }
}

export enum EDialect {
  DIALECT_UNK = 0,
  DIALECT_MSA = 1,
  DIALECT_SAUDI = 2,
  UNRECOGNIZED = -1,
}

export function eDialectFromJSON(object: any): EDialect {
  switch (object) {
    case 0:
    case "DIALECT_UNK":
      return EDialect.DIALECT_UNK;
    case 1:
    case "DIALECT_MSA":
      return EDialect.DIALECT_MSA;
    case 2:
    case "DIALECT_SAUDI":
      return EDialect.DIALECT_SAUDI;
    case -1:
    case "UNRECOGNIZED":
    default:
      return EDialect.UNRECOGNIZED;
  }
}

export function eDialectToJSON(object: EDialect): string {
  switch (object) {
    case EDialect.DIALECT_UNK:
      return "DIALECT_UNK";
    case EDialect.DIALECT_MSA:
      return "DIALECT_MSA";
    case EDialect.DIALECT_SAUDI:
      return "DIALECT_SAUDI";
    default:
      return "UNKNOWN";
  }
}

// If you get a compile-error about 'Constructor<Long> and ... have no overlap',
// add '--ts_proto_opt=esModuleInterop=true' as a flag when calling 'protoc'.
if (util.Long !== Long) {
  util.Long = Long as any;
  configure();
}
