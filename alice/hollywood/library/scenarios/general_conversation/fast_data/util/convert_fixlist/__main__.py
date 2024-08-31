import argparse
import alice.hollywood.library.scenarios.general_conversation.proto.general_conversation_pb2 as general_conversation
import google.protobuf.text_format as text
import sys
import yaml


def parse_args():
    parser = argparse.ArgumentParser()
    parser.add_argument('response_banlist_path', nargs='?', default='/dev/stdin')
    parser.add_argument('request_banlist_path', nargs='?')
    parser.add_argument('request_tale_banlist_path', nargs='?')
    parser.add_argument('gif_banlist_path', nargs='?')
    parser.add_argument('facts_crosspromo_banlist_path', nargs='?')
    parser.add_argument('coefficients_dict_path', nargs='?')
    parser.add_argument('out_path', nargs='?', default='/dev/stdout')

    return parser.parse_args()


def main():
    args = parse_args()
    with open(args.response_banlist_path, 'r') as f:
        response_banlist = f.read()
    with open(args.request_banlist_path, 'r') as f:
        request_banlist = f.read()
    with open(args.request_tale_banlist_path, 'r') as f:
        request_tale_banlist = f.read()
    with open(args.gif_banlist_path, 'r') as f:
        gif_banlist = f.read()
    with open(args.facts_crosspromo_banlist_path, 'r') as f:
        facts_crosspromo_banlist = f.read()
    with open(args.coefficients_dict_path, 'rb') as f:
        coefficients_dict = yaml.safe_load(f)
    message = general_conversation.TGeneralConversationFastDataProto()
    message.ResponseBanlist = response_banlist
    message.RequestBanlist = request_banlist
    message.RequestTaleBanlist = request_tale_banlist
    message.GifResponseUrlBanlist = gif_banlist
    message.FactsCrosspromoResponseBanlist = facts_crosspromo_banlist
    for date in coefficients_dict.keys():
        dictCoefficientsPerDay = general_conversation.TCoefficientsSamplePerDay()
        for testid in coefficients_dict[date].keys():
            testidCoefficientsPerDay = general_conversation.TLineCombinationCoefficients()
            testidCoefficientsPerDay.Relev = coefficients_dict[date][testid]['relev']
            testidCoefficientsPerDay.Informativeness = coefficients_dict[date][testid]['informativeness']
            testidCoefficientsPerDay.Interest = coefficients_dict[date][testid]['interest']
            testidCoefficientsPerDay.NotRude = coefficients_dict[date][testid]['notRude']
            testidCoefficientsPerDay.NotMale = coefficients_dict[date][testid]['notMale']
            testidCoefficientsPerDay.Respect = coefficients_dict[date][testid]['respect']
            dictCoefficientsPerDay.DailySetOfLineCombinations[testid].CopyFrom(testidCoefficientsPerDay)
        message.LineCombinationCoefficientsDict[date].CopyFrom(dictCoefficientsPerDay)
    with open(args.out_path, 'w') as f:
        f.write(text.MessageToString(message, as_utf8=True))


if __name__ == '__main__':
    sys.exit(main() or 0)
