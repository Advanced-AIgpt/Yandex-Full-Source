from sklearn.preprocessing import normalize
import argparse
import numpy as np
import sys


def reconstructPointsCKmeans(encodings, vocabs):
    M = encodings.shape[1]
    vocabDim = vocabs.shape[2]
    dim = M * vocabDim
    pointsCount = encodings.shape[0]
    points = np.zeros((pointsCount, dim), dtype='float32')
    for i in xrange(M):
        points[:, vocabDim * i:vocabDim * (i + 1)] = vocabs[i, encodings[:, i], :]
    return points

def assignToCentroids(points, vocabs, numBlocks):
    pointsCount = points.shape[0]
    M = vocabs.shape[0]
    vocabDim = vocabs.shape[2]

    assigns = np.zeros((pointsCount, M), dtype='int32')
    vocabNorms = np.sum(vocabs ** 2, axis=2)
    for block in xrange(numBlocks):
        block_start = block * pointsCount // numBlocks
        block_end = (block + 1) * pointsCount // numBlocks
        for m in xrange(M):
            distances = (-2) * np.dot(
                points[block_start:block_end, vocabDim * m:vocabDim * (m + 1)],
                vocabs[m, :, :].T) + vocabNorms[m, :]
            closestIdx = distances.argmin(axis=1)
            assigns[block_start:block_end, m] = closestIdx.flatten()
        print >> sys.stderr, (block + 1) * 100 // numBlocks, '%\r',
    return assigns

def trainModel(points, M, K, niter, block_size):
    dim = points.shape[1]
    pointsCount = points.shape[0]
    numBlocks = (pointsCount + block_size - 1) // block_size
    A = np.random.randn(dim, dim)
    R, r = np.linalg.qr(A)
    rotatedPoints = np.dot(points, R.T).astype('float32')
    vocabDim = dim / M
    vocabs = np.zeros((M, K, vocabDim), dtype='float32')
    # init vocabs
    print >> sys.stderr, 'init'
    for i in xrange(M):
        perm = np.random.permutation(pointsCount)
        vocabs[i, :, :] = rotatedPoints[perm[:K], vocabDim * i:vocabDim * (i + 1)].copy()
    # init assignments
    print >> sys.stderr, 'assign'
    assigns = assignToCentroids(rotatedPoints, vocabs, numBlocks)
    for it in xrange(niter):
        print >> sys.stderr, 'error'
        approximations = reconstructPointsCKmeans(assigns, vocabs)
        errors = rotatedPoints - approximations
        error = np.sum(errors.ravel() ** 2)
        sys.stderr.write('Iteration: %d; error: %f\n' % (it, error / rotatedPoints.shape[0], ))

        print >> sys.stderr, 'svd'
        U, s, V = np.linalg.svd(np.dot(approximations.T, points), full_matrices=False)
        R = np.dot(U, V)
        print >> sys.stderr, 'rotate'
        rotatedPoints = np.dot(points, R.T).astype('float32')
        print >> sys.stderr, 'vocab'
        for m in xrange(M):
            counts = np.bincount(assigns[:, m], minlength=K)
            for k in xrange(K):
                if counts[k]:
                    vocabs[m, k, :] = np.sum(rotatedPoints[assigns[:, m] == k, vocabDim * m:vocabDim * (m + 1)], axis=0) / counts[k]
                else:
                    vocabs[m, k, :] = 0.0
        print >> sys.stderr, 'assign'
        assigns = assignToCentroids(rotatedPoints, vocabs, numBlocks)

    return vocabs, R


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--vectors', type=argparse.FileType('rb'), required=True)
    parser.add_argument('--dim', type=int, required=True)
    parser.add_argument('-K', type=np.int32, required=True)
    parser.add_argument('--niter', type=int, default=20)
    parser.add_argument('--out', type=argparse.FileType('wb'), default='-')
    parser.add_argument('--head', type=int, default=1000*1000*1000)
    parser.add_argument('--dont-normalize', action='store_true', default=False)
    parser.add_argument('--block-size', type=int, default=5000)

    args = parser.parse_args()

    data = np.memmap(args.vectors, mode='r', dtype="float32").reshape((-1, args.dim))[:args.head, :]
    sys.stderr.write("%d vectors\n" % (data.shape[0], ))
    if not args.dont_normalize:
        data = normalize(data)
        sys.stderr.write("Normalized\n")

    args.K.astype('uint32').tofile(args.out)

    vocabs, R = trainModel(data, 2, args.K, args.niter, args.block_size)
    R.astype('float32').tofile(args.out)

    centroids1 = vocabs[0, :, :]
    centroids2 = vocabs[1, :, :]

    centroids1.tofile(args.out)
    centroids2.tofile(args.out)

if __name__ == '__main__':
    main()
