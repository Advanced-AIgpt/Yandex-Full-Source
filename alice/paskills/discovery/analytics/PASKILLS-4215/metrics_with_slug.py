import numpy


def print_metrics(file_positives, file_negatives, is_true_func):
    positive_count = 0
    negative_count = 0

    TP = 0
    FP = 0
    TN = 0
    FN = 0

    positives = numpy.loadtxt(file_positives, dtype="str", delimiter='\t')
    negatives = numpy.loadtxt(file_negatives, dtype="str", delimiter='\t')

    id_slug = numpy.loadtxt('id_slug.tsv', dtype="str", delimiter='\t')
    slug_map = {id: slug for id, slug in id_slug}

    for query, skill_id in positives:
        positive_count += 1

        if is_true_func(query, slug_map.get(skill_id, "")):
            TP += 1
            print("positive_count: ", positive_count, " TP: ", query)
        else:
            FN += 1
            print("positive_count: ", positive_count, " FN: ", query)

    for query, skill_id in negatives:
        negative_count += 1

        if is_true_func(query, slug_map.get(skill_id, "")):
            FP += 1
            print("negative_count: ", negative_count, " FP: ", query)
        else:
            TN += 1
            print("negative_count: ", negative_count, " TN: ", query)

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
