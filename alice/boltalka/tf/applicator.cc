#include <library/cpp/getopt/last_getopt.h>
#include <tensorflow/core/public/session.h>
#include <util/generic/string.h>

namespace tf = tensorflow;

#define Die(str) Die_(__LINE__, str)
static inline void Die_(int lineno, const TString& str) {
    Cerr << "ERROR:" << lineno << ": " << str << Endl;
    exit(1);
}

template <typename T>
static void MyLoad(tf::Tensor &tensor, std::initializer_list<T> list) {
    auto flat = tensor.flat<T>();
    Y_VERIFY(size_t(tensor.shape().num_elements()) == list.size(),
             "Incompatible size: tensor %llu, initializer %llu",
             tensor.shape().num_elements(), static_cast<unsigned long long>(list.size()));
    std::copy(list.begin(), list.end(), &flat(0));
}

template <typename Indexable>
static void MyShow(const Indexable &data) {
    Cout << "[";
    decltype(data.size()) i;
    // Print first three items
    for (i = 0; i < data.size() && i < 3; ++i) {
        Cout << data(i) << ", ";
    }
    // Print last three items
    if (i < data.size() - 3) {
        Cout << "..., ";
        i = data.size() - 3;
    }
    for (; i < data.size(); ++i) {
        Cout << data(i);
        if (i + 1 < data.size()) Cout << ", ";
    }
    Cout << "]";
}

int main(int argc, char **argv) {

    using namespace NLastGetopt;
    TOpts opts = NLastGetopt::TOpts::Default();
    opts.AddHelpOption()
        ;
    opts.AddLongOption('m', "mode")
        .RequiredArgument("MODE")
        .Help("One of [evaluate, predict]")
        ;
    opts.AddLongOption('g', "graph")
        .RequiredArgument("GRAPH")
        .Help("Model graph file")
        ;
    opts.AddLongOption('v', "verbose")
        .NoArgument()
        .Help("Talk as you go")
        ;

    TOptsParseResult args(&opts, argc, argv);

    // Must have an interface to OS
    auto env = tf::Env::Default();

    // Read the model graph file -----------------------------------------------
    auto graphdef = std::make_shared<tf::GraphDef>();
    {
        auto status = tf::ReadBinaryProto(env, args.Get("graph"), &*graphdef);
        if (!status.ok()) Die(status.ToString());
    }

    // Setup a configuration before opening a session --------------------------
    tf::SessionOptions options;
    if (false) {
        auto& config = options.config;
        config.set_intra_op_parallelism_threads(2); // e.g. do matmuls on 2 threads,
        config.set_inter_op_parallelism_threads(4); // e.g. do graph on 4 threads
        config.set_allow_soft_placement(true);
        config.set_log_device_placement(true);
    }

    // Open a session and import graph into it ---------------------------------
    std::shared_ptr<tf::Session> sess;
    {
        tf::Session *session;
        auto status = tf::NewSession(options, &session);
        if (!status.ok()) Die(status.ToString());
        status = session->Create(*graphdef);
        if (!status.ok()) Die(status.ToString());
        sess.reset(session);
    }

    // Setup inputs, etc -------------------------------------------------------
    std::vector<std::pair<TString, tf::Tensor>> inputs;

    inputs.emplace_back("model/semi_cudnn_seq2seq/encoder/sequences_lengths:0",
        tf::Tensor(tf::DT_INT32, {2}));
    MyLoad(inputs.back().second, {4, 5});

    inputs.emplace_back("model/semi_cudnn_seq2seq/encoder/inputs:0",
        tf::Tensor(tf::DT_INT32, {5, 2}));
    MyLoad(inputs.back().second, {3, 3, 3, 3, 3, 3, 2, 3, 0, 2});

    inputs.emplace_back("model/semi_cudnn_seq2seq/decoder/target_weights:0",
        tf::Tensor(tf::DT_FLOAT, {6, 2}));
    MyLoad(inputs.back().second, {1.f, 1.f, 1.f, 1.f, 0.f, 1.f, 0.f, 1.f, 0.f, 1.f, 0.f, 0.f});

    inputs.emplace_back("model/semi_cudnn_seq2seq/decoder/inputs:0",
        tf::Tensor(tf::DT_INT32, {7, 2}));
    MyLoad(inputs.back().second, {1, 1, 3, 3, 2, 3, 0, 3, 0, 3, 0, 2, 0, 0});

    std::vector<TString> fetch_names {
        "model/semi_cudnn_seq2seq/encoder/cudnn_lstm_params:0",
        "model/semi_cudnn_seq2seq/encoder/one_hot:0",
        "model/semi_cudnn_seq2seq/encoder/Sum:0",
        "model/semi_cudnn_seq2seq/decoder/sampled_outputs:0",
        "model/semi_cudnn_seq2seq/loss/Mean:0",
    };

    std::vector<tf::Tensor> outputs;

    // Run the session ---------------------------------------------------------
    {
        auto status = sess->Run(inputs, fetch_names, {}, &outputs);
        if (!status.ok()) Die(status.ToString());
    }

    // Show results ------------------------------------------------------------
    for (size_t i = 0; i < fetch_names.size(); ++i) {
        Cout << fetch_names[i] << ": ";
        auto& o = outputs[i];
        switch (o.dtype()) {
            case tf::DT_FLOAT: MyShow(o.flat<float>()); break;
            case tf::DT_INT32: MyShow(o.flat<int>()); break;
            case tf::DT_INT64: MyShow(o.flat<tf::int64>()); break;
            default:
                Y_VERIFY(0, "Unknown fetch dtype: %s", DataTypeString(o.dtype()).c_str());
        }
        Cout << Endl;
    }
    Y_VERIFY(sess->Close().ok());
    return 0;
}
