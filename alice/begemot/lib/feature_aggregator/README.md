# Alice Feature Aggregator

Alice Feature Aggregator is aimed to combine features produced by different
begemot rules.

Examples of valid configs can be found in *config_ut.cpp*.
Currently supported begemot rules can be identified from config's proto specification.

## Config semantics

Output of Alice Feature Aggregator is float array.
Each feature is placed in array with index, specified with *index* field of feature config.
So array size is maximum index + 1. Any gaps are filled with zeros.

Each feature must have unique name and index.
Name is used to generate enums purposed to access output array.

If feature is marked with *is_disabled* flag, it is always set to zero.

*rules* field must be non-empty array of feature extraction rules.
The order of rules matters.
When aggregator extracts feature, it iterates over array of rules and looks for first suitable one satisfying request experiments.
If applicable rule is not found, feature slot is set to zero.

*experiments* field specifies experiment flags which must be present in request in order to apply rule.
If no experiment flags (empty array) are specified, rule applies to any request.
Thus narrow experiments rules must precede general ones.
Check out tests to see valid / invalid configs.

Some rules contain optional *thresholds* field.
If it is empty, feature will be placed as is.
If it has values, feature will be discretized according to thresholds.

## How to add feature

New features are placed at the end of the config.
New feature index must be equal to max(existing indexes) + 1.

## How to change feature

Just add new rule, change or delete existing feature rules.
Experiments are here to test rule changes.

## How to delete feature

Just put *is_disabled* flag. Do not remove config's entry.

## How to access feature

In order to get named access to aggregated features one can use codegen tool to generate protobuf enumeration. Each enumerator corresponds to one feature and has the same name. Its value is feature index in output array.
Enum name and value are reserved in protobuf for features marked with *is_disabled* flag.

Codegen tool is available [here](../../../../alice/begemot/tools/gen_feature_enum).
