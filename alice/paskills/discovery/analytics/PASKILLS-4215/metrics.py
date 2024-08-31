def print_metrics(file_positives, file_negatives, is_true_func):
    positive_count = 0
    negative_count = 0

    TP = 0
    FP = 0
    TN = 0
    FN = 0

    with open(file_positives) as file:
        for _, line in enumerate(file):
            positive_count += 1

            if is_true_func(line):
                TP += 1
                print("positive_count: ", positive_count, " TP: ", line)
            else:
                FN += 1
                print("positive_count: ", positive_count, " FN: ", line)

    with open(file_negatives) as file:
        for _, line in enumerate(file):
            negative_count += 1

            if is_true_func(line):
                FP += 1
                print("negative_count: ", negative_count, " FP: ", line)
            else:
                TN += 1
                print("negative_count: ", negative_count, " TN: ", line)

    all_count = positive_count + negative_count
    print('positive: ', positive_count)
    print('negative: ', negative_count)

    print('TP: ', TP)
    print('FN: ', FP)
    print('TP: ', TN)
    print('FN: ', FN)

    print('all: ', all_count)

    print('accuracy: ', 100.0 * (TP + TN) / (TP + FP + TN + FN), '%')
    print('precision: ', 100.0 * TP / (TP + FP), '%')
    print('recall: ', 100.0 * TP / (TP + FN), '%')
