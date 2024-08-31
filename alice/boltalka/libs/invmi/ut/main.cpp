#include <alice/boltalka/libs/invmi/math_ops.h>

#include <library/cpp/testing/unittest/registar.h>

#include <util/random/fast.h>

Y_UNIT_TEST_SUITE(TMathOpsTest) {
    struct TMatrix {
        size_t N, M;
        TVector<float> Data;

        TMatrix(size_t N, size_t M)
            : N(N)
            , M(M)
            , Data(N * M)
        {
        }

        TMatrix(TFastRng<ui64>& rng, size_t N, size_t M)
        {
            InitRandom(rng, N, M);
        }

        void InitRandom(TFastRng<ui64>& rng, size_t N, size_t M) {
            this->N = N;
            this->M = M;
            Data.resize(N * M);
            float* a = Data.data();
            for (size_t i = 0; i < N; ++i) {
                for (size_t j = 0; j < M; ++j) {
                    a[i * M + j] = (float)rng.GenRandReal4();
                }
            }
        }

        TMatrix Transpose() const {
            TMatrix result(M, N);
            const float* a = Data.data();
            float* res = result.Data.data();
            for (size_t i = 0; i < N; ++i) {
                for (size_t j = 0; j < M; ++j) {
                    res[j * N + i] = a[i * M + j];
                }
            }
            return result;
        }

        TMatrix multiply(bool transposeMe, const TMatrix& other, bool transposeOther) const {
            const TMatrix A = transposeMe ? Transpose() : *this;
            const TMatrix B = transposeOther ? other.Transpose() : other;
            Y_VERIFY(A.M == B.N);
            size_t K = A.M;
            TMatrix result(A.N, B.M);
            const float* a = A.Data.data();
            const float* b = B.Data.data();
            float* res = result.Data.data();
            for (size_t i = 0; i < result.N; ++i) {
                for (size_t j = 0; j < result.M; ++j) {
                    for (size_t k = 0; k < K; ++k) {
                        res[i * result.M + j] += a[i * A.M + k] * b[k * B.M + j];
                    }
                }
            }
            return result;
        }

        void Print() const {
            const float* a = Data.data();
            Cout << N << '\t' << M << Endl;
            for (size_t i = 0; i < N; ++i) {
                for (size_t j = 0; j < M; ++j) {
                    Cout << a[i * M + j] << '\t';
                }
                Cout << Endl;
            }
        }
    };

    bool CheckEqual(const float* a, const float* b, size_t N, float eps = 1e-3) {
        for (size_t i = 0; i < N; ++i) {
            if (fabs(a[i] - b[i]) > eps) {
                return false;
            }
        }
        return true;
    }

    TVector<float> MklMultiply(const TMatrix& A, bool transposeA, const TMatrix& B, bool transposeB) {
        TVector<float> result;
        TMatrixInfo<float> C;
        C.NumRows = transposeA ? A.M : A.N;
        C.NumCols = transposeB ? B.N : B.M;
        result.resize(C.NumRows * C.NumCols);
        C.Data = result.data();

        MatrixMatrixMultiply({ A.Data.data(), A.N, A.M, transposeA ? CblasTrans : CblasNoTrans, 1.0 },
                             { B.Data.data(), B.N, B.M, transposeB ? CblasTrans : CblasNoTrans },
                             C);
        return result;
    }

    Y_UNIT_TEST(TestMatrixMatrixMultiply) {
        TFastRng<ui64> rng(0);
        for (size_t AN = 1; AN <= 10; ++AN) {
            for (size_t AMBN = 1; AMBN <= 10; ++AMBN) {
                for (size_t BM = 1; BM <= 10; ++BM) {
                    for (size_t test = 0; test < 1000; ++test) {
                        TMatrix a(rng, AN, AMBN);
                        bool ta = rng.GenRand() % 2 == 0;
                        if (ta) {
                            a = a.Transpose();
                        }
                        TMatrix b(rng, AMBN, BM);
                        bool tb = rng.GenRand() % 2 == 0;
                        if (tb) {
                            b = b.Transpose();
                        }
                        TMatrix ans = a.multiply(ta, b, tb);
                        TVector<float> res = MklMultiply(a, ta, b, tb);
                        UNIT_ASSERT_EQUAL(ans.N * ans.M, res.size());
                        if (!CheckEqual(res.data(), ans.Data.data(), res.size())) {
                            a.Print();
                            if (ta) Cout << "T" << Endl;
                            Cout << "x" << Endl;
                            b.Print();
                            if (tb) Cout << "T" << Endl;
                            Cout << "=" << Endl;
                            ans.Print();
                            Cout << "Got:" << Endl;
                            Cout << ans.N << '\t' << ans.M << Endl;
                            for (size_t i = 0; i < ans.N; ++i) {
                                for (size_t j = 0; j < ans.M; ++j) {
                                    Cout << res[i * ans.M + j] << '\t';
                                }
                                Cout << Endl;
                            }
                            UNIT_ASSERT(false);
                        }
                    }
                }
            }
        }
    }
}

