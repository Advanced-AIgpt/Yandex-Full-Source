package ru.yandex.alice.paskill.dialogovo.domain;

public enum ImageAlias {
    ORIG("orig"),
    MOBILE_LOGO_X2("mobile-logo-x2"),

    ONE_X1("one-x1"),
    ONE_X1_5("one-x1.5"),
    ONE_X2("one-x2"),
    ONE_X3("one-x3"),
    ONE_X4("one-x4"),

    MENU_LIST_X1("menu-list-x1"),
    MENU_LIST_X1_5("menu-list-x1.5"),
    MENU_LIST_X2("menu-list-x2"),
    MENU_LIST_X3("menu-list-x3"),
    MENU_LIST_X4("menu-list-x4");


    private final String code;

    ImageAlias(String code) {
        this.code = code;
    }

    public String getCode() {
        return code;
    }
}

//  Orig = 'orig',
//  CatalogueIconX1 = 'catalogue-icon-x1',
//  CatalogueIconX1_5 = 'catalogue-icon-x1.5',
//  CatalogueIconX2 = 'catalogue-icon-x2',
//  CatalogueIconX3 = 'catalogue-icon-x3',
//  CatalogueIconX3_5 = 'catalogue-icon-x3.5',
//  CatalogueIconX4 = 'catalogue-icon-x4',
//  CataloguePromoX1 = 'catalogue-promo-x1',
//  CataloguePromoX1_5 = 'catalogue-promo-x1.5',
//  CataloguePromoX2 = 'catalogue-promo-x2',
//  CataloguePromoX3 = 'catalogue-promo-x3',
//  CataloguePromoX3_5 = 'catalogue-promo-x3.5',
//  CataloguePromoX4 = 'catalogue-promo-x4',
//  MobileLogoX1 = 'mobile-logo-x1',
//  MobileLogoX1_5 = 'mobile-logo-x1.5',
//  MobileLogoX2 = 'mobile-logo-x2',
//  MobileLogoX3 = 'mobile-logo-x3',
//  MobileLogoX3_5 = 'mobile-logo-x3.5',
//  MobileLogoX4 = 'mobile-logo-x4',

// menu-list-x1
// menu-list-x1.5
// menu-list-x2
// menu-list-x3
// menu-list-x4

// one-x1
// one-x1.5
// one-x2
// one-x3
// one-x4
