import json
import sys

from intent_renamer import IntentRenamer
from sklearn.metrics import precision_recall_fscore_support
from collections import defaultdict
from datetime import datetime


def rename_intents(classification_info, intent_renamer):
    for item in classification_info:
        item['true_intent'] = intent_renamer(item['true_intent'], IntentRenamer.By.TRUE_INTENT)
        for stage, pred_intent in item['stages'].iteritems():
            item['stages'][stage] = intent_renamer(pred_intent, IntentRenamer.By.PRED_INTENT)
        item['final_intent'] = intent_renamer(item['final_intent'], IntentRenamer.By.PRED_INTENT)


def group_by_stages(classification_info):
    grouped = defaultdict(lambda: defaultdict(list))
    for item in classification_info:
        true_intent = item['true_intent']
        for stage, pred_intent in item['stages'].iteritems():
            grouped[stage]['true_intent'].append(true_intent)
            grouped[stage]['pred_intent'].append(pred_intent)
        grouped['final']['true_intent'].append(true_intent)
        grouped['final']['pred_intent'].append(item['final_intent'])
    return grouped


def collect_intent_names(stage_info):
    return list(sorted(set(stage_info['true_intent'])))


def compute_metrics(grouped_info):
    metrics = []
    fielddate = datetime.now().strftime("%Y-%m-%d")
    for stage, stage_info in grouped_info.iteritems():
        stage_metrics = []

        stage_intent_names = collect_intent_names(stage_info)
        stage_metrics.extend(zip(*precision_recall_fscore_support(
            stage_info['true_intent'], stage_info['pred_intent'], labels=stage_intent_names
        )))

        support_index = 3
        total_support = sum([item[support_index] for item in stage_metrics])

        stage_intent_names.append('total')
        stage_metrics.append(list(precision_recall_fscore_support(
            stage_info['true_intent'], stage_info['pred_intent'], labels=stage_intent_names, average='weighted'
        )))
        stage_metrics[-1][support_index] = total_support

        for intent_name, (precision, recall, fscore, support) in zip(stage_intent_names, stage_metrics):
            metrics.append({
                'fielddate': fielddate,
                'stage': stage,
                'intent_name': intent_name,
                'precision': precision,
                'recall': recall,
                'f1_score': fscore,
                'support': support
            })

    return metrics


def main():
    rename_paths = sys.argv[1:]
    intent_renamer = IntentRenamer(rename_paths)
    classification_info = json.load(sys.stdin)
    rename_intents(classification_info, intent_renamer)
    json.dump(compute_metrics(group_by_stages(classification_info)), sys.stdout, indent=2)


if __name__ == '__main__':
    main()
