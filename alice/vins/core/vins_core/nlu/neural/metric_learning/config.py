import logging


logger = logging.getLogger(__name__)


class MetricLearningConfig(object):
    run_name = 'bow-words'
    checkpoint_file = 'checkpoint'

    model = None

    # Optimization
    loss_params = {'name': 'triplet'}
    embedding_learning_rate = 0  # sgd momentum
    learning_rate = 1e-3  # adam
    learning_rate_decay = False
    learning_rate_decay_steps = 1000
    learning_rate_decay_rate = 0.5
    epsilon = 1e-8
    grad_threshold = 1.
    batch_size = 512
    train_buffer_size = -1  # -1 if not needed
    l2norm = True

    # Encoder
    encoder_num_units = None
    encoder_num_layers = None
    encoder_pooling = None

    sparse_seq_input_size = None
    sparse_input_size = None
    sparse_seq_output_size = None
    sparse_output_size = None
    dense_seq_input_size = None
    dense_input_size = None
    finetune_embeddings = False

    encoder_output_dense_layers = [
        {
            'num_units': 300,
            'relu': False,
            'dropout': 0
        },
        {
            'num_units': 256,
            'relu': True,
            'dropout': 0.1
        },
        {
            'num_units': 100,
            'relu': False,
            'dropout': 0
        }
    ]

    # Info
    metrics_names = ['loss', 'recall']
    training_loss_freq = 50
    validation_loss_freq = 50
    checkpoint_freq_in_batches = 50
    max_num_checkpoints = 2
    save_weights_start = 0

    # Sampling
    negatives_from_same_label = False

    # Misc
    num_updates = None
    callbacks = []
    save_weights_mode = 'best'
    restore_weights_mode = 'last'

    batch_samples_per_class = 40
    num_classes_in_batch = 100
    class_sampling = 'sqrt'
    train_split = 0.95
    exclude_intents_from_validation = None
    valid_num_classes = None
    valid_samples_per_class = None
    transform_batch_size = 1000

    batch_generator_njobs = 1
    batch_generator_queue_maxsize = 1

    def __init__(self, **kwargs):
        for k, v in kwargs.items():
            setattr(self, k, v)

        logger.info('Config was built')
