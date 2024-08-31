OWNER(kudrinsky)

UNION()

FROM_SANDBOX(
        2332306995 
        OUT_NOAUTO how_much_dssm_test_neg.tsv
        OUT_NOAUTO how_much_dssm_test_pos.tsv
        OUT_NOAUTO how_much_dssm_train_neg.tsv
        OUT_NOAUTO how_much_dssm_train_pos.tsv
        OUT_NOAUTO how_much_dssm_val_neg.tsv
        OUT_NOAUTO how_much_dssm_val_pos.tsv
)

FROM_SANDBOX(
        2332293587
        OUT_NOAUTO how_much_alkapov_test_neg.tsv
        OUT_NOAUTO how_much_alkapov_test_pos.tsv
        OUT_NOAUTO how_much_alkapov_train_neg.tsv
        OUT_NOAUTO how_much_alkapov_train_pos.tsv
        OUT_NOAUTO how_much_alkapov_val_neg.tsv
        OUT_NOAUTO how_much_alkapov_val_pos.tsv
)

END()
