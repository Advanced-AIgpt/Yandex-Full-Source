## Getting started

To run AppHost locally do `alice/hollywood/http_adapter/localhost/run-prod.sh`. Currently, the ports are hardcoded, so that `40004` is the `http_adapter` port.

Then, build and run the `alice/hollywood/scripts/run/run-hollywood` (use `--help` to show possible options). 
If you have any problems with access to vault with secrets, request a role in ABC service `bassdevelopers`.

## Logs

Use `apphost/tools/event_log_dump/event_log_dump` for reading AppHost logs.

## How to bundle data with the scenario

You may want to bundle some data with your scenario's package, e.g. a trained model. Do **not** use the `RESOURCE` command in `ya.make` for this purpose!
Instead, you should create a `resource` directory in your scenario's directory, put a `ya.inc` there which may include `FROM_SANDBOX` and `COPY_FILE` directives,
and then include this resource in `shards/all/prod/resources` and **ALL** the other shards your scenario is activated in, just like the examples that are already there.

To load the resources, create a class that inherts from `IResourceContainer` and register it in your scenario like this: `.SetResources<TScenarioResources>(args...)`.
Load the resources by overriding the `LoadFromPath` method, it will be called with your scenarios resources' directory at program initialization.
Then, you can use the `TContext::ScenarioResources` method to access your resources from any one of your scenario's handles.

Every shard's package that includes your scenario must then bundle that resource using the `BUILD_OUTPUT` directory, see `video_rater` and `sssss` scenarios as examples.
You may also choose to simplify your `hollywood_package.json` file by copying `shards/all/prod/resources` to your shard's directory and only leaving those scenarios that you need.

If you need data that must be updated while Hollywood is running, use FastData, as described in the next section.

## FastData
# All scenarios should use different messages.

FastData usage example can be found in ssss scenario.
For tests and local run purposes fast data can be stored in `alice/hollywood/shards/all/prod/fast_data`.

Please NOTE: for tests and local run you can use `.txt` files, but in production services only `.pb` (binary) files will be used.

## NLG usage for scenarios

Scenarios may depend on NLG libraries. Place scenario's templates in its `nlg` subdirectory.
Use `SetNlgRegistration(NAlice::NHollywood::NLibrary::NScenarios::NYourScenarioName::NNlg::RegisterAll` in `REGISTER_SCENARIO` macros to initialize NLG component when registering scenario. Also, you must include generated `register.h` of NLG library and must specify dependency in `PEERDIR` macros of `ya.make` file of the scenario.
Information about NLG libraries, see in [alice/nlg/README.md](../nlg/README.md).
