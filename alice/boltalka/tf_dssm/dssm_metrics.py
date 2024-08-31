import tensorflow as tf


def hinge_loss(margin, threshold):
    return tf.maximum(0., threshold - margin)


def log_loss(margin, alpha, beta):
    return tf.log(1 + tf.exp(alpha*(-margin + beta)))


def make_loss_semihard(output_left, output_right, threshold, negatives_ids, is_anchor_left):
    batch_size = tf.shape(output_left)[0]

    dists = 1 - tf.matmul(output_left, tf.transpose(output_right))
    dists_mix = tf.where(is_anchor_left, dists, tf.transpose(dists))

    dists_to_pos = tf.diag_part(dists_mix)

    dists_temp = tf.where(tf.less(dists_mix, tf.expand_dims(dists_to_pos, -1)), tf.fill(tf.shape(dists_mix), 1e10), dists_mix)
    dists_temp += tf.diag(tf.tile([1e11], [batch_size]))

    dists_to_semihard_neg = tf.reduce_min(dists_temp, axis=1)

    batch_triplet_loss = tf.reduce_mean(hinge_loss(dists_to_semihard_neg - dists_to_pos, threshold))

    random_negative_ids = negatives_ids[:,0]
    indices = tf.stack([tf.range(batch_size), random_negative_ids], axis=1)

    random_negative_dists = tf.gather_nd(dists, indices)
    batch_recall = tf.reduce_mean(tf.cast(tf.less(dists_to_pos, random_negative_dists), dtype=tf.float32))

    return batch_triplet_loss, batch_recall


def make_loss_semihard_and_manual(output_left, output_right, threshold, negatives_ids, is_anchor_left):
    batch_size = tf.shape(output_left)[0]

    output_right_pos = output_right[:batch_size]
    output_right_neg = output_right[batch_size:]

    dists = 1 - tf.matmul(output_left, tf.transpose(output_right_pos))
    dists_mix = tf.where(is_anchor_left, dists, tf.transpose(dists))

    dists_to_pos = tf.diag_part(dists_mix)

    dists_temp = tf.where(tf.less(dists_mix, tf.expand_dims(dists_to_pos, -1)), tf.fill(tf.shape(dists_mix), 1e10), dists_mix)
    dists_temp += tf.diag(tf.tile([1e11], [batch_size]))

    dists_to_semihard_neg = tf.reduce_min(dists_temp, axis=1)
    dists_to_manual_neg = 1 - tf.reduce_sum(tf.mul(output_left, output_right_neg), axis=1)

    batch_triplet_loss = 0.5*tf.reduce_mean(hinge_loss(dists_to_semihard_neg - dists_to_pos, threshold)) +\
                         0.5*tf.reduce_mean(hinge_loss(dists_to_manual_neg - dists_to_pos, threshold))

    random_negative_ids = negatives_ids[:,0]
    indices = tf.stack([tf.range(batch_size), random_negative_ids], axis=1)

    random_negative_dists = tf.gather_nd(dists, indices)
    batch_recall = tf.reduce_mean(tf.cast(tf.less(dists_to_pos, random_negative_dists), dtype=tf.float32))

    return batch_triplet_loss, batch_recall


"""
def make_loss_semihard_free_anchor(output_left, output_right, threshold, negatives_ids, is_anchor_left):
    batch_size = tf.shape(output_left)[0]

    dists = 1 - tf.matmul(output_left, tf.transpose(output_right))
    dists = tf.where(is_anchor_left, dists, tf.transpose(dists))

    dists_to_pos = tf.diag_part(dists)

    dists_temp = tf.where(tf.less(dists, tf.expand_dims(dists_to_pos, -1)), tf.fill(tf.shape(dists), 1e10), dists)
    dists_temp += tf.diag(tf.tile([1e11], [batch_size]))

    dists_to_semihard_neg = tf.reduce_min(dists_temp, axis=1)

    batch_triplet_loss = tf.reduce_mean(hinge_loss(dists_to_semihard_neg - dists_to_pos, threshold))

    random_negative_ids = negatives_ids[:,0]
    indices = tf.stack([tf.range(batch_size), random_negative_ids], axis=1)

    random_negative_dists = tf.gather_nd(dists, indices)
    batch_recall = tf.reduce_mean(tf.cast(tf.less(dists_to_pos, random_negative_dists), dtype=tf.float32))

    return batch_triplet_loss, batch_recall
"""


def make_loss_softmax(output_left, output_right, threshold, negatives_ids, is_anchor_left):
    batch_size = tf.shape(output_left)[0]

    dists = 1 - tf.matmul(output_left, tf.transpose(output_right))
    dists = tf.where(is_anchor_left, dists, tf.transpose(dists))

    dists_to_pos = tf.diag_part(dists)
    dists_temp = dists + tf.diag(tf.tile([1e10], [batch_size]))

    soft_dists_to_hard_neg = -tf.log(tf.reduce_sum(tf.exp(-dists_temp), axis=1))

    batch_triplet_loss = tf.reduce_mean(hinge_loss(soft_dists_to_hard_neg - dists_to_pos, threshold))

    random_negative_ids = negatives_ids[:,0]
    indices = tf.stack([tf.range(batch_size), random_negative_ids], axis=1)

    random_negative_dists = tf.gather_nd(dists, indices)
    batch_recall = tf.reduce_mean(tf.cast(tf.less(dists_to_pos, random_negative_dists), dtype=tf.float32))

    return batch_triplet_loss, batch_recall


def make_loss_easy(output_left, output_right, threshold, negatives_ids, is_anchor_left):
    batch_size = tf.shape(output_left)[0]

    anchors = tf.where(is_anchor_left, output_left, output_right)

    params = tf.concat(0, [output_left, output_right])
    indices = tf.expand_dims(tf.cast(is_anchor_left, dtype=tf.int32) * batch_size, -1) + negatives_ids
    negatives = tf.gather(params, indices)

    anchor_negatives_dist = 1 - tf.squeeze(tf.batch_matmul(negatives, tf.expand_dims(anchors, -1)), axis=-1)

    dists_to_neg = tf.reduce_min(anchor_negatives_dist, axis=1)
    dists_to_pos = 1 - tf.reduce_sum(tf.mul(output_left, output_right), axis=1)

    batch_triplet_loss = tf.reduce_mean(hinge_loss(dists_to_neg - dists_to_pos, threshold))
    batch_recall = tf.reduce_mean(tf.cast(tf.less(dists_to_pos, anchor_negatives_dist[:,0]), dtype=tf.float32))

    return batch_triplet_loss, batch_recall
