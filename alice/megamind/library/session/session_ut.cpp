#include "session.h"

#include <alice/megamind/library/response/builder.h>
#include <alice/megamind/library/stack_engine/stack_engine.h>
#include <alice/megamind/library/testing/mock_guid_generator.h>
#include <alice/megamind/library/testing/speechkit.h>
#include <alice/megamind/library/testing/test.pb.h>
#include <alice/protos/data/language/language.pb.h>

#include <alice/library/frame/builder.h>
#include <alice/library/unittest/message_diff.h>

#include <library/cpp/testing/unittest/registar.h>

#include <google/protobuf/util/message_differencer.h>

using namespace NAlice;
using namespace testing;

namespace {

const TString SpeechKitOnlySession = "eJztHGtz27jxr3g4k36yFZKSbCkzN63jxKlv4svD6aXXyMOBSEhCTQI8ApTiuPrv3QVAiqRlR0os2"
                                     "7koHyhyCS72vYsF4itHDP9LQyWdZztXTi5pFIzDIKNpzKiBRXRE8ljBPc/jeHfHURnhklGOoBGJJQ"
                                     "VYSjPJpKoDpyTOqf1uDs8RI7EYBxMYKLLLO0J+5ag840jqp3MAkzQNWARPzpQBFCB5bp6DAO+eRZR"
                                     "EQ0pHe/u9fm+vE7pkj4z67b0D0uv5HiFDv9Nx5kgu43bKuyGTkwRv9DDBSRwQicMJVy0ZUk4yJmRr"
                                     "TFUwo0RNaGaIIJwLRRQT/K60ceXM2BeSRUZ2l6kmyoJqw7I8tiZwGLOQvs1oKJI0VzQ6BgrzzLxEI"
                                     "k9zycIm8CVXTF2eUZKFkwL2CoilRqQfxAU1artyXtMkIUjGIHc77ZG+Un1tV+47+uoikc/pmHH4wN"
                                     "XzICueD7cf6Ge1MhqgpzmzX8XttRfIu03k/rLPzYw9vHZ8fd/V9zWSvf0Srd9bTvMSDObemaOJn3B"
                                     "QgpaucVEQ4BJ5IGqr3DAHh0taI8Kyy0CRmCI9v1s9O2lMLmOwlqcH7V6/6x90O888z+/jmDORZ6Ee"
                                     "hE/HMRnL4uG1GL/NxJAMWQxaBuheB6DvcmIfXS2fgi6/pKu9hK4kyEjERCCNodeoQ8a9G8Wyc3x6D"
                                     "3S+ouIwirLWEX7SJK7jGXvS10hfiSF0r6LS7g0q1R932hVIe22GujWGvBpD3q0M4YVKWePpagAfSw"
                                     "VDTqKB88zfNc8nHEISA98voSgNuB1sRgoDB6Z4KyBEDmOKU+l5P53P71Q8y/1FXsrWiMWJDDzXDQ6"
                                     "6bl3pTiKmjA6ctSlp32J5C0X5D0zJch94jJR03QBuFM1o9KDUSDFSDQLkhHHGxzsjAZPvMHZvtIyp"
                                     "aLpzaBwV7ljhu1iLlL7r3+673jf47nx9N12f3ZFUfx12MbEfxvE2rW/T+jatb9P6Nq1v0/o2rf/47"
                                     "GJaPxZZUjQ6rn+Aseo32xoi2Gdp0c9g+9ggkhcsjoOIyVBMaaZz1Acz+9LaoF3FFCo21Wk3SCcZkd"
                                     "SZn5uOxTUKPLfV6/pu2z3oXiOl2o6qzb6YsarMF0QR8/6mHLtc9RvOvJrxx0XReUXUM5Cu1g8C12l"
                                     "h7egff6cy9pbWESA/0pVZpca8MoxapVZeX1bhpWeuV2lai/sq1u+rE63c3oRhnmWUh5avRXtx5aai"
                                     "kWZ59zWB4qzvCR/fUqzfFO2AZlTHC5apy9eAwrHtUd1nPYPSDcVQhaGs3pJM0uwDS0x39j2Vpg98L"
                                     "7zOS1IOOUknIiNn+VCCSnMlMk1E+Wxo/4QUzqC0VJS/p3+C1jdj1kDWiSxncmy7W5te4eqavOciur"
                                     "SZ5HeSMcKVxMLz6l4K3n8S+WbGde2Ns6ospwD9SNl4ouDZa7mua0vvkxeaLh+tuizZlxXsFqxT1L9"
                                     "4UTAdcjmjWVHwA88mZT5Olldk8fp3Zg2Bj3D3mvKxmtiBv0GuhdT1hUYYSDfMI3qlnsLIU6vC6vBM"
                                     "aSVoD5tp4vFRUw//EFqndKPLTpCVnt2ULDMj8Nd0SmMEvxahqVc0UaiydznV23NrbMdolCXHTS2WL"
                                     "4yuysf3eUzLOOZoWousoNPz3NQU1KltKR0zHlHj1aeE8Y+Mc2PyqwSMgfIHqj0Ae5x14MFt9d3eQE"
                                     "G9OVCj4bOhEBctvAQy19ui/wMgmZEsaulrMNOTITQGsWGsa2FhqsSM14DFTQ1IogSWD1Jl8DilUNh"
                                     "NGUb6geoh54aPI6F3HQsxiKL0+qtweUpUOKHy5edUZFrrRxP31+g/p388P/7jzcGH3tnzf//qRV/e"
                                     "ea8OX46fM3H8buK+OjwTs6PD3d0bbGZhBUulN9c54jcyLc3IFtinJLvIU5NNG1hx2EdidzOdI6IkF"
                                     "P5Kp1izLTxQqzjHQGmXL3/wTquj38ef2ivzk/MLDmK2z8id1tpqSq+jMiZgJ6vT4bX2u91rk50vES"
                                     "8K4piJWjVShi7thLo0SBaiPBVZOrElXlGY3FQdmQ3UoiBcZ/d2V28mJwk1SyvnbIeLZEfC0nyHccI"
                                     "1L1ha5WSsTSHLi3p0CVGVLs2tRPnXpn37fr2ZqkXg19m/YSu5wTgZxsh4chvjCH1BY5ZARM7MhJpE"
                                     "oOVoQlCPB5XFx45jyHx+qWOvV67zCoBfAOzH+w1k/VuQ7TeRdRvIeohMU7wo4JYbk1eX5jGjcSRvH"
                                     "N2QfS3TYIjAN1B4VVZmm8nImrVG89WS97EoFbxFVjUvzeKwcmZiIez9NZeNDX34DX24DXW4Dd1eO7"
                                     "TgNxB2mwpuNzAeNDBeP16xQtW/mFAfo6hNuN+YsG+WiBkDEIk3vBx5w5VYNfzZholZDBtFoyxOuPp"
                                     "o7MCtGoUO5fCvDRZlQzUyYfKUnpla0zkpji45NsRXSyt7rml+U4z6Oo37NRq9puGuRBhLsAIwLd46"
                                     "ZcGQ3UJdw4uXUPe9AuzDKLrndr8mR2xSzrVlvc1ECKt2Gm3StJBbhfIIIPeOeWII+wQMgmB9s0Knu"
                                     "kCW5Zmr8nxX7cVu9ZQXre040rIDxisx8M57q0iCss3EMgAfKqgZw7IFxOIYUxjKVyqii0VUrVr08Z"
                                     "5Wly7mkJ8ueE2ENSwHo8qxNAfqXfzezFt8/Q9hsqaEUBwwk0CcGbtgKY0YsSUYoMcxEyKDBE+7BYy"
                                     "P0M2L03ULtLjeOV/GX8FRnVVQ3FzrbgRVlXNXR/wsstUPG6IEZUDjmKUwsIJTTtA7AhkLJWFhoCZB"
                                     "mtEpE7kM7CTFyAyMn2U00mODcSbytDyWqT83Joa3Ttn23MVWcyxmINVYsVQ7ZIHQhIigFCTSq1hCg"
                                     "6D8FkERrBAbIE5nsgDh9LyoWAMdm61cLX5rKGkmuMirfbRoYcrtsOiVdaKKmdP6u9oX/fILv+I9YR"
                                     "1q3cI3BoiirrNr1VPlhX5OYckGYp4WzVTLTig4LMFAubYFh0YwZDrARkxClUijql71tkagapl85SQ"
                                     "3rTZo1/iuLvLrTqP3LWjTpIPC6WFVyfh4CSIrM0QhUuSfYG9Dd51AYlMYVgS4qQ3oRF442qF5FBcl"
                                     "cfGE7yE2xEMS6kEpgYrbRBCzm2KPGAOOiuHp0G9EjxPPK0GUx+OgRGjL8QXzC64g0AQ20iS4UAZwy"
                                     "Tv9DMIxexkVF+LreNC3uMIKhsk3Z5cWRfUk+c1W9HUDWv79isZTVxpGHR2MMtwDgOtMM7SAzlZWY0"
                                     "RA5JjhHliVJR0/pTorvK+iNOCahkSqoGh8PbT2rhN0X2q80pvzWBekIW4119PC3W/Ml9kkNFvYm6t"
                                     "RNV/Ikw3+wKGAMtFKbLH6rbzQxrahEwkl41wkjBPrGptjf0w52+gkBp0NnJueqGO6mZhjwwkG7Whh"
                                     "yqbCt/2/b45Hjj23chcBaWwropXqABsDihDw0KHoGj33F4nUF8TwMs9ESp+eChkKnZcVTWAppFeCd"
                                     "h2ZZwxHTpRK5bOnTy+h8qOfW1n+FMYMxd8R/y/65ZP2YfV1KsYiIk/axyRNzXo6mFAS0exJ+4X3xN"
                                     "8fZSKBW31+phBESLIIXsVEwZtuv9XHgxI4OBYcIG231fa6++3u33KVBIbBXzQCDQhJkhJY8f8S2v+"
                                     "iFZktMcd3fXfPbe95+05lYVwI35yH0LYAEo7qUYsuHKU9XPha56DicVSH745XKe6tO+mvOwbiXVv4"
                                     "RJVlgHHU0ECc73OuqnXfhYfVrHMdN+NiphMtgaXCA/jaT1id2RXSqoqCdByylJnzRBiFgocrsX9Cb"
                                     "fE8+UZVmbwcTIBRuVXVo1YVB9a2+vpx9FUpB7ZaerRa2uapH6Wq0PFvq67Hqy6J+0B5MjSbW1vtPN"
                                     "qYpwduVfS4VFSKxpR4W/3cg36aMl/HnfT7gMgLc6Jmq6sN66qU+bcWenSYj7eaenxlwyWVimZY2D2"
                                     "WbvoSin5CNa7drS33Q7FSf2gd1onZqm9N9T0+HW4Vuaoi9W7VNtE9ukSnvmy18ji0cl77vHJGUebD"
                                     "hKmVjykWpw1zDqxlksT1M4fmmPHiD6Sm+mxz88+j9of94agT7XX6+3SvE9H+Xt8/iPYiEnl9vzfqt"
                                     "f0DZ/5/orYzQQ==";

const TString VinsSession = "eJylVEt3qzYQ/i/ZdlEkwm3pLnbCKyAfY0CgTQ4StgFLXFpjXvf0v3cgaU/acxduu2ABaL7HzDf69vD2po7nXF"
                            "VN8fb28MvD9tyW3JY1Oxg4p8YcP21+rZ/Or1tlXXNKpN+wljfB4ce9eP3hLF7NQbx6aWgIO74xbPYMjzJLN62Y"
                            "jJpjrY8uH3UypCueHpaFHR9mqNf3yU+uMkpO42pXeQNLvZljeLfMUuikFXCWN/ubwERyZU05NWdfJY+FnUzwfk"
                            "118ltOx6tbDZXAiZZiQxbIVFz3OrduVUbHmR2Wf2NfOEG1k91nvjnH8gpcLdu6V1eZQ2HLfsGFWk00ifS3nnZM"
                            "N9KtvwJG13J7qOAbBlzEDu4XV7lGVluSPL8gP3pCgf3SEdurdltUEbWfffrySCImd8+xxp4Dg1TA05C+SL2aJa"
                            "bG0lIDrlsBHk6H8bvauGL94o/rGw383zI6VLsmnAoaA9ba9++eL8BHsWApA4Huu3lZSuaF5x7Oz2f/I9/EUgsB"
                            "jnYPX+F44E/8bQa8Sa58u8zYREIRmeqhPDr7Dx2j6TvXCmbarfNas0tOnEKW9GS4g3PdgRSjUqgOiSUnDSu5k8"
                            "g//YCWmevJlOHkBLl8LLYfmhztepw8wEXv+VHJLGwTshwv+nvhhO2adTgPvtCS08KRw7tOTwp904NW+T96iwon"
                            "lEJZt0UbV+YEvq952sq78q6HsG9G86/4E2S+lG4P2iWj5CtkUvOxNYitOWSHn3N0tj7rgzkt+tZ9f8/vqsuamJ"
                            "50jBqamNwv0MNLDro5Lk453S911Y66I4v2IzloA4k2pR+FVVafu6xml2BCJewbZtEZEXs/76KsOkFvWcok3CUw"
                            "4095UOt98Rc+3CGzwNCTdU835ZGOfUbfa5gt4V40mxTLy9KfrJZVQGMtiOIueI7Bn6Zlyn30qYth17ts4Vcx7H"
                            "94CeYLdhvtH7vjYXhOGSVr/twGmQ+//wG6Ne10";

const TString VinsAndProtoSession = "eJzEu017qzC0pflfalqDQmByj3t2MAiMLRGEPkCT+wByB4PAxCE2pv98b597q0Y970GeJE4MSNp7rXcZ8f"
                                    "/8j//8z/HyVY/Xyfznf/6P/+t/HL7mroltrwvfrZW/ne5R4JzD/Hd/2DlVkv8cwvyxPyCnCvPFHF7OucwX"
                                    "etgdaRzA11+n+mRfJob33PPHctic8+7Lqf5Xvuzj8/u1f+83h+eRHp7O2ct/9of/cM59/vvf7/mlBx/+hp"
                                    "zzJ7w//gs/f7zPBa87/34+P/If+j7v36//PsfHf/38v6+xh+v6977dv+NUU74U4fs9//G+1t/lfR74/n5t"
                                    "OfzP97HhGKtTbf/Gc/olgZOW7+uCv/07B/uCYznn/N/54Bxw3ed/4/h9v/+/x/Rfx+/Z9d853+/7gq+SXc"
                                    "0B/ddYH+/jf7znEL6/xwhj+I/3seEYCev/jfn4dS5C1pv48J63z4Q+tcJ3XR6/zLifK1fsj+P+3rr4tyqO"
                                    "X1rpUSvqfBap0474t3XXzsTy/27cZauU35Wu/D1egx/4eYL/sc31+NV6gdUq3x/7v79ZcfyfFyWnxjt+VV"
                                    "76fv2rfh9jjL4qd99XRfCuha2O5esY4199eP8ud7oc3tfzatzUb7zqO/w3p/8L5uD9vc9/zOF/nvotv71g"
                                    "bv+Ny2M9/Lz8m4fm33os/+axh1r5P+v7H/9q5T0n1/A9L/B3/q6BwKnO+e/BP/3kyf+Z56//qqH/9Z7jr3"
                                    "/nOL/n1fk371UEX3CM/fu9yX+vE9QTHLffHzynerxfe79//79rB2pye6/Ru/7++z2w5sN7Df9/qoX09P81"
                                    "r/Xtcvib3yI52t92Pe+/8ui030XB4ZEF8PpsEvuE3p0adz/BWnUtrN95pI+m2P/3a+K3TdKHifdXo6pfOe"
                                    "Ifo0SxHqMg/jkEf5U8XSY26oT5NOxm2bNzFu8/+dB9KLWsbFi8Jvz74s5+Eo78YeMqa6lJLY1q1VrlAw4a"
                                    "iW+5xGEFP0sxD8qaG98in9ho1VPnViPGlQ1szdsXdWDqy/aeCfbd2MEzMarkRkcSfbl1XL3y114Lbq0Qhu"
                                    "XIBgpTJaxe6ZR21O10bucT5eZUb9rWW/d9mQInn3BUldZylR50QnxZ4pH1+pGp/TXj9qmmm2/KIGiw3dUj"
                                    "G3nkbESZSsfoSZ3BIxMOc5flGWenVqU2E/RFkzQUqH2awT8b3HntZogZ04Oy2raD/1L4y4G5P1coyM6eXu"
                                    "Sk2SUc7tSh5xYxLZV+5ogOIrT3oredcfVgXn823utXPcpXwXVc4O5u+uBs4lVrxbaqDwR151OLdax6fTIe"
                                    "QZdek0zqSYfdTx1h1wjyqHv60SJtSSQLGi0fZFgeBB/vYpq5FliRnv3SaL01w3qjG5NM7HzhLt/SXfo6MR"
                                    "O//nlWfeqdXEOZaF/5SMNaUXEZnPvFypKP+asI9YkO9tQMhurDEpEBn/hGNyW+7rnAqBmpVyQ2l2r3KAbh"
                                    "FNe9Q6ZuanrtaMFGUuwR6dNaDjO5xDIVsfkpBpnTeNi0I4+tu0/5RA80kb8nx9iKt5uOW9QWPy89IlXj7k"
                                    "Rd/StD6da22lXq+KAjKjM5e3T76yi1e4oy2omYkSyer4rLT5bINZMV6JU+tMXy2fCOXMb2RcaOZsr0OsaC"
                                    "J/rVhNHKNkxoLEWNzCe57n8KLhONuw3+L8sS6RRR1+nkuKl4GWBtVBbnni6rV+sgp9nsKg77gLs3dOHHrd"
                                    "jscBGz27z2WyaiHZP2KnD6Wdi/W7vJJFf5q3JWTAYGdYyXrITaD6UisS5F36F2XD8rsf62alhNEr3IJtZq"
                                    "8Ce6pb9cVc7Z/eNr15+NtQPl6b0Iu7CJjS2gt4x7fMH1P2mEPoWIEFN6zUc/Ie5cttAnxqWPIpEP3Qu3jf"
                                    "SncVfWSHqsBkvIaAYSU03Vz51y8TT479Zg3MveDM1w29UKajpCkx72axH5NQ9xZcIvWJ9UZ2G3a6b5WQ8m"
                                    "MoksM+j72tHl2YkePE4TibELxx9pb24U7ICiHFUlu5HRcfTorNBnXlUefbGlXYulbbFZjf3yyUSrdvu6S1"
                                    "FtEgVJFlrZ2mA13pfLNinNsOyM1KwdusnE5A69dtNI5k1vRevsNqHoXYSBTxFxLwmd83EHPWl7VeK4caUs"
                                    "YnbPlRz0Fvy2jgV9Izsu57JS9lmV5kM7zxd1LVIqFVwZIjk+sog5lUi/jbS1GcWOjZGbTwzRKJ1rrKs6Tg"
                                    "l3MOEbw2zKn5cx7XnS2bZsdxoH49nDPxpRXPBhx/ruBTojzw51lNjPVOArDc3GhI2IZ3wzdg/9WlKCiV+N"
                                    "BM61IhFrosc94645X6Te9GREJZgnMU3qUidS4IIoeSMI/8AcO6bHZavwQTupJgAXepzVpfjZiv64Fr25Vh"
                                    "vxVSiP1dg6dOt+6fWPp7k5CMfcYb36IkY/KjKbDOm1eu15i2yqBudFEnrV6rbRSK6546/cWW/Gre51aIpa"
                                    "iSc/LB/Gtc5F2W+m6CqR5ievk4VKk9bi8FLsA/raF9XYnUGfVKa0lw/yDtednZz0UGw0zCKU1GLnVQMbma"
                                    "df0u0sgV6o8fwUyqrWSX3irL0uoa4i65pxYdA7kkb7Qol8FUoX0pMu3dq7Cu2oezpe4h+kp2Csxh2Ca+jJ"
                                    "hvMsMjmM279c94MKMczBz0Oo2/3sttvZ7XZEMdAZiaqx2kDDM+kd0cXiV3bYX7XtKiqDqwqDXMS7XXH98y"
                                    "DuEoOeBuBLeQGeQF9LwGCeYZV+paLXrDRdLnyX9AFho0Vt1PVMHl9ssLmYmENU7rAYF1QMXqZmR4r1xZW/"
                                    "owldL8OaqtL4F57uamhxgVLeDniVHj6KqfVkpKempEWrno48LIz10Z1Hs65AZ+nYLdwGD1ayrZAp9MpMay"
                                    "kReMYN6u6uI/wpVPTiKHqInj7bPvWVirza2T0vIQ0Kd+fo6+IzMUc8maNKzYtUqKzDDko390HHGRPUqd3u"
                                    "lHM4D0qdegqCs4PTBsPaD+mjdeR3682lcP1BHBa3SIzHuRyq6yIaGTxM+NfPx30GWnWr3PSXlsOO9tbVo1"
                                    "lzGGvj4EiHOLpgtmg3XcElXpcw/TReUNejgTqnLtTNA+a90u6fVUj9XYAew7gcg/AZag4UitYQP1BjU9Ba"
                                    "6N1Yny8DjbjUuyasdmwwK3WQaJUcqVh2FCqoei2lsqDJyJ6JNK7ieidFFwELl+dNnymeX2Y7ei0PXk0M8x"
                                    "jt/NZ+bTXqBqLog0FPmhBLHYO9A5/TYXg2CVzTNOyKkFYEOIjF+xKqdSLWIhGtTn3dAyf9dYoe4lUE9edo"
                                    "6Gl6rWN9z12/B72s2egHOe9sUc6fRfz10Bv1eJRSI9JIboAvEdkID3bF6IccsQ4qS3CRTjJCgXbmX5Z0v7"
                                    "VnAtDwpxKYNNflM+d0Or2We8apJBN4yGt5tFNQgV97l4F5F9CINvqzE5g5JzTfZZg6l5jaNhY7NWpXjfRX"
                                    "Y5jFEDyVy+iEOivi7pSJNdPF3lWKlVrkDzLuxcXagg6L0snNBZ0UWQTsudn0Yqu7svPu5K6g5XTXDD87qd"
                                    "iHsPZeu9gTYec0rlHG02kRr9i4pq/BFXjc5Tx2NgF+QNXXneH5Jka7iZGeWQ89GC8f2pUflbvoE9BJjfPH"
                                    "JR783EGRic0zK/aFLpaUCr/WEw10n8bVBOw37s9V3z5qDucubSpV5DdDyiU2WCW2B28gdTIg8BQmSvYLjf"
                                    "mreAfz8/eRj9I2IndksVyz4ud+RsNalORFHHSEPkfa0+LtT8SltHFuSAz+w3gyB+6uCwXzGOEQOKIkw58n"
                                    "T+zcKPMLfgq1ACNR6zl39ScHZgLO1a2zr6QlT+Cy9aK6M4zvCVyU1BP7uSh6arxA1coE0KsJ3+ysHQTr0V"
                                    "3bkri5pH7Gccc2dioGG7YesPPhz8OM9qqSrhaepnqYqcH0CYo8gce9lLVJDrVRj8iCh510BH58+HGyECNW"
                                    "4oGMtlLJTDhc0xk8FvSJ0SHFOkp9KfVicFq2Ub6SEJh/3OtmhFwRfyHzWo4sZDN4Zn1ytW8UCqlKOz7NR9"
                                    "PbB4uPW152NyXYUDiLVlHnwFh29WaRcVnSTEF8wXNGXXPMhXQlYo+szJ/S0T7oNZSlrUmEgeQ62W7HlxLU"
                                    "z0UKng7zHbOt4cCuGPttyc5Ngn+BbJ4EpzBnWALfEHi9k1aDbuMQ+KOTrrMVYYUu43zMUfBJ1Rqz/q+jpf"
                                    "TaeCZ15LxYsQAj0adUJq8jJnPXXMWANwHLoBM61NyeToimzbh8UGXg9XxT5fwtIl1l5Xxk6Gsjrv7WUbtq"
                                    "0DDQqP7db3Lwv+vB/4EcD17rPNuEzfXw9Ksy+GajeJydL9cUy3beyMM46ECHP/dGmgW8cazk4ILOVFx1vf"
                                    "Hs2JT4o46ILwaQ9ojeGql74y6MCrjWEIu6tMcCRb4Y/VhZzInwb1VvLeRJj1wX0GMN/QWc5emHsX89OrAM"
                                    "dDTUarEFaFYRLbSxplfx8ODAU43yI8qrd96DHLYMzE1zGOtHE+U+T9LN8HTKJOsoJy5klkBPZNNJEIp4Br"
                                    "1gaTVRbQYNdUap5JpTl6XAhZWZIBPF86GJoo06Jr4MpjATrlVpzzQG3hztrgE+EqgD33AeRmnwmllmpYYZ"
                                    "xQI8f4Rc55+dlIv4dgc2mjQP7uK1p9SVwSWec8iRWS3QsxjxAnluVwgJLDGfLuHxAeN5gAZqyCjvPkubkP"
                                    "6C6n3wHs47QC31egONe7FoLS/TEeYr6EFjgVawhqp6Nqp9nFz7Dd78AjZ51DaITw72YSwcavtFXPaqeLo0"
                                    "Y3cE7vuE9T1UsJ5N37rnbfCzkH5CRs9PyECty48TeKUcv54XtQ+yRI8tp+oi5DcHzgFvIdT5QtWWTsRxXu"
                                    "CR51oddxdL+7PTPYVMl9yZceMsM7fAmZJmxP3jkJB+Q/63cjMMGM4rBjMbyAi5EKsWsK4J6JTTbrWzPxYj"
                                    "pGJFoa7ERlEXsikFvd0DT1MXQr3giF451ISRLNSH/Vjj4BPGnGZcfrQ9VeT6g6A+SuN23yym3fn1x6sTGx"
                                    "s8V2+WK4DH8ik9XmLtQO/OdQ9zFeJUvn1LYFeF7CWuyw5yPgaPeOjSXkH/Ay12roIKr632Lm9W3qJdPkGG"
                                    "jEyp1R+oj8qB+QlzqOnLaMcsCVLmdNFFRJChIuA0/0fi/CXtDAqPyov92jUx5PUBZ8DuLzXe7qC3eV4OTx"
                                    "ZDNh4xcEl6MgJFqky7Iu6g82WVgTZDVs7h72XtVE6Nv56gubH00uslXhba47lO0pnLrsoH6B0xe0paKxPx"
                                    "4BEmJoKxOeudRWgkMrDC/t0xLBXMk6un6KHiaqcOS5bF+mWu+9vJy+/wPTu51YNt5ufkQb0fQBd66ENOHn"
                                    "lPWT1KF/LOiY63B3ktlVDmpTlwxbiPLtyAZ6UP7ui1iAEJEe0KnP5eOKYFhmMM7IO78tQ4NoN+p6qkkPdw"
                                    "QAY76BjUNDKvTPk5Vftv8E9NBuBpgQ7GMb+XfkB5iW+5Z7wzModGYQ/eoxovpe2GAZKFT63+raPFbfsK/G"
                                    "tBxSafFYy0EGwEZgZO01thuylLGFKJeTYhdhqYDxhrLUf/ZVCADMefxJqPAs8nWULPYWwzheHvqCxG+tOq"
                                    "fHeRZJWDWMXIYm7TWnHMLsrUb49vXZS2I1Yt/E/jMshPzG3C9JsqWgBvEaDH1TjQo948AudTAgRaR1/v+r"
                                    "/xngaXUoJPyTh38QT5qYbMFHJXe9DX/DLRqBnnnQjJXQKHgoYiPdBa4eCUQ69ClvnNhPNsIqq4gFzTpwWN"
                                    "gOcsgyy3f8lRJsClFDwrI0O7u5TplA+7zUj6Y2LQcCnubRw9C9XpArEC5u+3QpHHwdcqL9pRGF8DHAT6fj"
                                    "UOhl4BfU/0vYUqF7F/aPvWz3Dq5A6e28N+MmG0ZaGOwB8/68hK0DF1RvPY2i5t+k7xiME1DE4NxwDfyc7u"
                                    "Ki+SQeMfH2LSQ3NYSGthjst0qLa/TyY1EWLBxVA5ZFyXSjEu3J0P48urXtvcXW98pLxWjmdKdpI9uzfDfq"
                                    "cGs8tV6rTKL4RjQ6F+3GawWSVNACyPZLF/tAl+EsfeIAPZqpczXAulEzBnbB6Ve3tpD3KNY1iNjOCDVlzt"
                                    "OzV8PQvHeX/OcZKRfxAJXsH/vi9KoCzSfRZJT4Dms5FG5J1rh8iH+T9CnblQ2+slYQPUARy/elbl8KgGyO"
                                    "8R/pDD7gmeqIBzf4DZkhoyC4lSyNTkQSbcw3xZpoZdO+kbcATKsM4IBq6z6Xbhg8c4jeu+C5qeeSd3EbqU"
                                    "sobc2CaWtIM8MRieGmRdQ64worvXoCWKQ7326ZU7+asdUcU33YNPRDKZ3/V3kji4Gq4hK1Om+ZfXKMvqXh"
                                    "ZcoO9GDM8i6T6kZz3IYt8sYZEa2EZLsrG4mxicn6q5BwYRamTPFmpbD7DiW3c4IeFzYBqQiGvVp4jH5rOF"
                                    "3m0V/uW9XM7ufGevJaMo+DHJX5/DeICFT3QwFXN/EJPy0FpZ6q3aXXBXN1ZO9fjzuETShfp7mViGkP1FJd"
                                    "mVCrpTQr6ApV7AqmkD+khk6oMnQfqwSTXSq45/INviXCl0Law8qkR+SmUfOgbldJY7Uf4PaI6bI9Cx8uhA"
                                    "NseQvzfpADNgeaqQdRlPe6i5PlNyhgz90WATA5vcAHj7S3z0iyS6Q834VKCeRuxTIuBvTjaoO0y2rhM46F"
                                    "rIYVB/r5N3dC7XPTo7upax3vGyXYU7+8YD3hqeO1LsgQG0BcwAPafj2QHvc3/uMNfvzyKPGZwX+EOwvsua"
                                    "zQbg0z/1REtjoSYdf2xDs4OcdAY/di7C/xDjn52SdEc4Dhthv43Vp9oTT3NYIGdDf0YdqbgGLmePGt028G"
                                    "Ov6OmLRPtPMy4LCXEO63rP1dMhMeT1YXGBFVVhgTWwvNbJ30fGvx7yugCT0Cgr/749JwCfeVDevYg6AseK"
                                    "B3Fvvubm+/TaAyVAyo19t52qB+RAcXaiHWQyGBse6i39UNH+xEUElfvzgvzBche5jfzaTp6ehfJ98NzcjD"
                                    "grVI6YN1sJOtH28ke6z62a2JCj9LOQZldZW2clu4lYFiZkodikWwk6FImBa6nurAc/iwYvt8Mjd1Y49/EO"
                                    "Ye+RxfsDCYP1Eq8K+ovKaIZ222+C44JICWPXTAzO+/PpA1XszLFEJ/R1lwn4Rmlr8+9zICOUMh3rJRVAGA"
                                    "wBi4WyghzOma3WM9Lh2Tk+ebH/zcflyoo9YwNkKEmTLFpg7RkykryElBvUSq/fLNrjSY7QmzYlrAzALeF3"
                                    "jsvCC8p8k+Bfx2fjmTuZuvP771ISt3Ao5AEzQuZgwD64Un92LJHX3E3HNjGRVgiRoVPgRcnJSz0mu5LKuW"
                                    "qReOqRCRp3A3joUQzEVVbXzUbXZmpRa6kHPPujYP0hwzlFhLEZDaegk3BuyK8W/LBF0FeJdixUsgCP7s56"
                                    "0A/Q7RtofdI6zAHdlHCegvc4LSa9QI2ll3HnMfv3qYCTGncti7ALgIvAt3CgQxpwhXyJugEy95bzFrzgxy"
                                    "dYF5J3z0vJvvUI5MDZjQ70lYXpq4hp9fZDYI07ZEwH1p1raW/MQc8W0SlTnde61CnK9KohqElbPYvSCP3a"
                                    "J6CxNC8p0dOctJJ9cycFJjR+4abPBtibDh1XCmsDemhidKohdwhuvUtZ7SD3L0atnrDvez/mmTvLmcu/jw"
                                    "ryp4jwr3DlN/i/o0MgJTu7Jmo3gW4rcMpUi6PLpxR6uUuKsQsu0mxKBlBzFitx9BuEH5eSOI1DCXHSEnTR"
                                    "g1RTQM1S0CZRR7iTsb8R0MvCS6GAMbmo7qP17Ar1EZEyuivFnlosfu6YXEmT0D56FEm7SW+GL9Pl474+u+"
                                    "nGNv1BnDm79MLPEc4viVnV2A007M7c0zvq+OzkyRwcgEH2eVJHPGoF6XAKJoEsLG4HrGUAuuYUfNevBGTx"
                                    "UdaXaV7ad12GVFfvzztj/OJl99vG3UJVWuVl9BAC1lfITjrrYlyNK6j4Ipb5BQgzd3aQwfId+GNGXj/gt9"
                                    "2JWgweD8eIJCVlJ4BXQuIZKWIDNWDyRjEGWWqFn7cm/tlEGByM2B+58F3IoB8iYmclMYOMCG6bSjrAfIzm"
                                    "Vgmdibj1uNfuWsjTZJgfDdSVKOdb7bQ7YPXRjMCQYomLyEFayhyu7Z47P5AH5E0A9wE758alfW5nRq/7I+"
                                    "lh/ZVBwJm0hn66FPsOfPr9mbJrFHE4Tm0TSVuU7NGMK6aS1ZU7H0TZbQ3WOzLOcx3SnZZsV22yKw4/99ra"
                                    "w0UY2k6QK3F+B8+hdFw0iZeCW/mjIqATd6ZNqHc6gk6A2mwseUrwA+OxpI7xs1XW10kKpDczqPWxQfIMuf"
                                    "ClRPSizm2F4/SyxH0T2gyu6ckVhQzMgHtTXDm+lJINbFy/qfxCdJOAPPo7E8AYaFiJozGFmmZwrfnIOjZ2"
                                    "BLLPb5GkmA6MnTfJGcIf7chWOJ6Ujp9y15xEGYQt116BQEtl4OalvhoFa6pkRdHskcHYrFjqTOzpRazfBT"
                                    "LAZalfjc8Hi/ZSbF+vOv7aFWXutZxNVcnCYsMrnbQHGd4ad94gRN+qnh0hO4jcNUtrg5PgaSJgzmhvJmCd"
                                    "sdhSzEdNaERzNQ6IlmlagU8JF0NOe2csuVyAEegwgw6agg3VU0L0gRwBefkI3mJ3LbcOjaOtcoRfD3+AY6"
                                    "tNi+6nxWLL+9QjDvFpdNu4/PLqXn/mpR2qPtCXsr3noBkMzqHH+beyZsfVTPLh6RNgh8ZNAzPNkFkI4mEq"
                                    "qDefDHBeveW+eS0R1GB6hl5vyplX7tfd2A7UevDNlGojJMvHI2Ts9KiiLoE+9RQwxiW2BPz5xbbAF9PXvR"
                                    "HIY8gu1IXM73Uql19OC5p04Z3HhuWXSyOlkP/uadPIRpDzgTkxyV16F45+VY6paTl/6Gj/20zBmbj4ARlO"
                                    "mCFdwClEA5maebQQMg2JnAlkq5Rz+9Fw6rFST2RclPECqcSyFtO8O7vOnVrIRpN5NJIuatBXyFC8vu6v2j"
                                    "NZ0Yu7Au7hvNplkS+ZZBCpflyS4A8uLDBZUIoSQ3/YtCl1nKk/j9qDeeeBX8f2u4BpAS+phdoHcH0jaA2X"
                                    "zkwFMBd/f6YIrM15xyskIXtQTqLns02GV9PLk/RarxA7yJzdtwKWaEJ5Npy4YpDfYpiLClHLJsMrb4aMIx"
                                    "1eSsE8Ar6CHqCzHjCILYb5o06CDyHMGShPc+DUGs29QHqBPodkMke5TK9F/Od+RnDdajldxqMPIbLmyqaq"
                                    "7CRkv5EplLCQevVoukx9bZeRhvnGPgvRzYUM+hwyr0S5Vwm74y55AecKXoqX3vKVQAY6wfpzLt9+7OhoQd"
                                    "qznMDaAmcfmAtaypkvLNRBjysmJBbYnAq1xsWYps2WRpAF+jo2OxkCM8Yr1HoQg8b+srJ61oM8t54cePTj"
                                    "tfEyG2e+N17wSxxwYgTa3v/dScimJ8/mRZiuGjgI9MDXbu6JMM3ksOwaMb+59gFi81vE9iV6DPnoC5h1Zs"
                                    "BJ7/0roxbobIZlojE7mdG5855V+n2Pafy5g99cCzlDHoocc9iXxGpbjEt1ERTq/O8GzE2JR1Xjgb8lNoJ8"
                                    "KGBeKVH0myv4PskfquyByzS6RKxm8raDGnkC+4Av/NmR5OuVO7Q2PfFrQe+XcVV1KXaXeD+anv3WU/oB/Z"
                                    "ASl52gBz2NZp3FcgT0esjI8lZQKhVklpFsJo529eD7JgmA8xdZlDORPKgNFqixZgA+7iGbF3yS3WVcyku0"
                                    "ApcsWtnhJQ978KE5lbE8NrDmJzf121DOWZSeW7dLoQKjCphcKmRJjPXFSi8riU/e+s7pFfKYB/WOgKNOwH"
                                    "HOJUa+cNk5H9u75OCzgoWV1fjs+nC9BLUwPsgBBHhwIQK8BJiOTulNxuuziWEM0UphvNdW7e+1+7OBjgOX"
                                    "tnexYT93u5BiVgCJ1Wagn5C7MzLeXgR1O52wnKrdsx063Tg/DzbBzOH5kyQ2gZyDL0r+Kvm1gxwTkL71qq"
                                    "07VpylkDMZjXcP4x39931Y8LtQu7gSvHOq63JTKrpDb9k6wbOKpGahYXwLpmbDREc4bd0lhjx8yErpqOuP"
                                    "CwwQUhcTMqUdYIoyjh8VaueDhn4A356AnVHj+j8SWBmy1xHqHkaIe4ogz/K/O+CNxwnyAqjKSEq56XB41o"
                                    "k8Fj2s/UR2VYkpSW4bAw6rxd6qRGvw/aodl7yyN7eOgPEc9muiZcsk7ehWeWTDtpUSPHTtspi5kI8+M/AO"
                                    "0utvivKdfjNyaLyL8j/UICvw+lw67UbUehAi7WWIeSXNJ/jxAzLbSY6rrqx8ypAFRuVruwXvz78RR+YHWP"
                                    "xRuZCxIvQ0LjDa6Mf6/RmqSl+X/u+mxvVYOX98WMeBJt3nJdoPjTp6fJzP3LFJG6dcC/9sXLHCOtyAq1Y1"
                                    "AZ8oa5s+8MjoX+uJpdL94xEv7drt68nG+cNslSsc4l7431cWa6UdtlyS9EY5fjWR8CSePb6lMd2wkq+9mw"
                                    "tWsGg5wLh5k0TrBfi43rpnE355eSkDfVgU5MsH29jL4C9P9izjtvukLvCXlRPbuqFw/rxYOfcc1pl6cL22"
                                    "cyDlv8QW9ETh0wl1z0rRvlXpB6zF76WkwESY8JjaynZKR2LHnZ1LY+yIUQpash/Iie7JFZCpZ9WM+tqWw6"
                                    "NQ5t6M/nc1/PEvZb4q2y1nj2mqnnfDo90ZdSvdoPfR18rK9t89PbjeoBHmVPfBD7O3F/Teq0Iasukcw/xk"
                                    "ZhNek7RPtg2+gHxZYOrxvnJOSPoXIW0WE5e4xqvGbgffD5loEXAM53HaafFmJ8Q5Tx/Vhh0V5y+oadAkVj"
                                    "Vck6KUv2IaHsydi3acX5DpiHZlZhBGbYSGmpMn9dJ7NaRQk3QS8WJz8XwZCTzV5w9IoR8i1klhrc+SwYd6"
                                    "vhXXxeFS9s24jI1aotbORZN0z4ITxED3L7LLWrF4OdToRXTbpU8PlxjmXGJwUf8T9ClrIExzt0VcpeMFs1"
                                    "/irhXobAh1/VtH+74ZhrWO/WPxnvvIXFXfrqrUH7nwC2YDxnr7A5n7fnbyXQN1TpN5KZLgznD6WyBgFzzv"
                                    "WCifQNF5Y1ufx/PpYm+Qc+nYKmDYqRtJ3+naAYYcc0TK9pXx1G1Qeq5dfH7vFeDgN42HX61jZiH0JKculm"
                                    "7n5mh+aCsx+HfOY+MVIeigZ9V560DP9zF571uLOgfqZ3fhNC+in625LhR6v8qlLGsF2SnWR6X2oU4CRUPo"
                                    "pcEyIHNcOPKghsEhUt+Esg8G2SR3nm7O5YmJymu8yCGeDI2z1rms1jqEDlf7V600Lzx91M4NgTbfWRSthR"
                                    "cQ4PMfHlHbohYR2ZV6NEXNMQYvqC/lbSVl5fE3G6rUq51ualFQ5P1xg/U8QB/iyl1YjmgB170Af59NGaQ8"
                                    "ku97lptwZFqMNDWQLYwHvuhRLJylB/bxW6Sz2jMHJvyTEFAfETtA9igL1xQNB0a09CrFHFZiTrkCHhnZN3"
                                    "hed4nAB2MMbOk8lWUr5PReu/sC5qrQMn1Ahspq9YWYzXe5F8BMtB7waM1in+u46ygmbqFsBsf8BB96Mnft"
                                    "GxdnhEMfvD/LjmRWjNayjSDwYl1tkM/BV8GrE24DpBF0ZiLT88bAv1gOGeZ6SSwlI2Tw2MzcXQ5kiIDLIR"
                                    "sPwqvlzZfOv/1RDgHWPnnzk8SMXzDU1/gHiaQ7kgGlBacJi0Df0SxlhAszDltdMs7RzIib+8zrwA86lyML"
                                    "+ZTOJ9ef8qna6KDTuqTvvanPkzvchQv8CnosUeoJYWNgJHb2DC2c+SUdW0CCWxQQQj61WwHzzl/LRw4cx0"
                                    "sbNMBbrAy2E2IxwexJR6SFpzs92VEjer6MXVh7VIthzSEjzFr5rI2Nzt09eAV+QT2fxKAj6B9ZObNbOEww"
                                    "jFUB/S2n4AT/Qy7jvs5ddOUClMN9e5b5bqX1Kge8RIq19tgHeFTHRReAp2nI8dsZ0WeujFURk2SawatxAj"
                                    "6AgDVnXXZeAfMuLeTYZLZK6rKYqFBD2lWlBR5rnRqbWxPvwQOD6aJopN5bnxHdETEP1F14EwE3X38eucRP"
                                    "znMXsojSSHfShV4cn36uOlK7XdCilDQDxBCEs7bvdjw57oDRCmC8KPdsX0fyWztLQRVoGPiXHMxaeWnJRn"
                                    "MGpv0shq6DOYD53/+0sdTCBr8igr4q7Q60QjfDWhbIfra8O3IslxPkaXj9Xgz7DPTtWcngt0XR1kBbwfp+"
                                    "A/OBbvlL0xvQgNwF/Y7g+Gs9BZ85snFtZahxqnSZHutNvNQkXKiXtz6k2jUBdyBfl0cnd/SuGexWFMs3c/"
                                    "Sh3cx7P7DQ5cwa6F9Ynw85zMda7Lb3XhiCqW6EDlkS7PhAo3/7omX7KiKpVGwH5gV+8b6nr/DnCc24HlMG"
                                    "87YVvbWKs6p6733j6U5Z/BSJzhnUbmHxsSpnJ7f6B853v3A8QH2FgBK8cI4PmKtCKyS5ZC+mKIZarirXzk"
                                    "o5iPbASXEnGtnxk7M+hTRUuvsr85jPoZ6ly3aQrY7NsIAfABsotPLyL+QOXNaWPvnEikssuyKqXMhHu0sC"
                                    "HQscLLlYc3f1siQFReuq9zXD/6BCQH0MgMTgK7Waee6xGxz/VQngF4ch6mnVCvN5mWQs4j1kK8khj9xNeX"
                                    "Mht9bMEsjv1lMK/ciwA7mXURvSsAX9emtO29O56Om5cm+oGNdCKjy3FvfNaw/zJFw1/iAdP713HhBD9AIu"
                                    "9AuxCHhPfxloV6k0rd2ZQ/6t9PTXy0dzasW6g0wGtehsuvzrAutV7WuxQsiP3OJPHkMP4+GRc3vio1nMsB"
                                    "/BB0DLCII8EOkpPYnxa6t7WtKN7ZgzhyxiXut054sC1leRCzS95OrpFlv0lJP5UEn3K0s70GGWeR88To6u"
                                    "im24Z4rVmVpzM6Uz9N43+Mj6/gxFFftHswVVPj5f6roceWimS0QnvnU5RQyR2Cd1L17VFrhZom9FWT0a5Q"
                                    "CHVzs9GW1KRi/gk43Cik8mu8i5V/F+yId0hIwwXKT2gJnP1MGTOiy/uZ0rs5kestogbHQ/e7MP+rODYJ5C"
                                    "nUIGgKLjdmEulZViB/XOHB7ZGW94MdBlg+yiJotJf3xdoL6g7td20mvdpz7k1VeO2FZxCnmk8pVsXVEypD"
                                    "f2OHv4CjkdXYT+IXK2wP+WYENYrM9UmDtFumYjqireqRoB81obN8kM3rgX4t/9ZRvw6QaM8GdtZbBqPCOh"
                                    "dJUdlhrGdNYqpXWY/pzdNGytSXhv+2IKFtnLDDJapB0D2ofv2sFPqJNJlTfPJHZ36VkBnPeplPkokhyqpc"
                                    "uAFxHU6iai3aNGeNE8PWWhtuDhThE5DpzbZb2BecDX4rVHTFBXRWtK4/mqhzWGvPTBnGGX97SCLq3JsCQm"
                                    "XmoW7a1UPpc9rVvw/qrHwBjol+Eu4ZOFXmCeiNMzvf65a8hQ3IUSgVxTjfM3XA+H2v/Um/FIMpN2QPNFgJ"
                                    "ag20563bcSoOHh1wpa6nEvCE2SVtobYAzoA/KxCx7/Om1/EfEw8LGmdQQJTnWLDm3FJ31krp8DqyjRBy4b"
                                    "yJ1EGmpMP05vf5uOTz2AO7yZRdhVx8QpLM2q1/5Uh383vXUvieichakkPaMnx6TAOlJF1UsN7C5GxwVvqo"
                                    "ETIL+zuZbmqR2/kI58SLXeCuAzyO6dQYNXuN0IvpGBVrsVkqESq6dK+8+jL9E6MpmCU68ql8f3fWOPROvj"
                                    "YoNdIy3kka+1icAjNs1p7H/KEp9B75CI6JA7aaGHmysSNoPvDmJMKYugbksTGRdqr9jPkLU97epUTeZB3H"
                                    "ZXhNEqovdeSOjx0S+ZMlVdQL31GtdW/+ptAJZfo1zhA42jtREsNaPzeO+jhITUc9B6YOxejb7KkuN73wrw"
                                    "RnrP0derGMy9kimMLQU2ZKl4754YZsh/NswhhzHQUvCHuVZ+Ur/v6djBb3tI94lwaAjM5lZPknTxyZkZ8F"
                                    "peC/xJvHklqCvP7p+XEF8uZG9SuDblidRnT/dFvK6NSi2L0YMgS2lMv8EDEGQfIqweW6FXvuGxsqljxmXm"
                                    "SNxBjw6X8u8TtCySY34vIPPWoXk14r3fduYcar3e8qfBOK69/AkZ9kYk2+Uus6qUPsE4upTMAb7YFX0wsX"
                                    "Gv2ZYehWOjFjjpwrvore0NZFhyBa4YxYO/90pY+1mhvw7jfxG1+Ld2KTLb3xfb6FOj7pFbO1YTnXS0e0FN"
                                    "3Axk2cbOpzcf1wJFVDw9wd89b8/MWSGPI2Da9HaRBjxDnqW8PYHTB4EtacPgl0mbE4upSfCnVJB7ojm+hN"
                                    "2B8wB6AzkNrDvkx50OU1xYXTXArNSTE+1BpxxT5mp5VuN6Ak/65kO1nr0vBHMOOUJC9sdVofa/dFiocYan"
                                    "khiy4u313gPDR23NQPwa2UOBvp5yM4pMQNIq/eEo0MajqQjZb4PaO/SYhhpU+YBudIMEkphnK6V9s1ut5F"
                                    "NiKatB/+S9TIp4L+CYcf5+Oijyz5BdXuDrVDoVghx6JvGP3w7RDrT+WMczLeLl10S2bxy8tUji1vHnNmYl"
                                    "zPWWqedWQOqBLH1j73u7A5ZZzD5y2XVV312VTGU9ypMeZMYwQ60jHpBhuvq1zNW483RJl2JgkLfsh3G/nH"
                                    "r62oHe5oX39YB8vdQhZpArknwDdhyRo9TKLjEcUxnNgY85HJ9gqRqp5zaCNZV/VxLdkBhNmjuRA7lvuURy"
                                    "12zpibnpVDnQ83IABkmr1qMlGW8emyKXDKkGzX8/H7Br3P1HhSDnv/cBYVrVsT4Bw99zGWDgj0qM+0EMc5"
                                    "b39kFGH9WqAn/rZhIbDIyzQS7RWRR5JB4edbROrdclbTK7SkJfj+xTvu+7ck2k0CfIoufLABw52KJVjseT"
                                    "7pqPaG1KQ+jQIRg7Bl22J49+aMe+9z4MdLS5waloIomKGCoSm3s+YL+GXmjj2a9K/H12ug89shE0/BP03z"
                                    "F2vkM2vual3JRKH43juCIMCtAsyPVmrFyfVx7MtZR9Gx8d1QufHP48iwT0wwZzG5qKRvtPGGN/KeeT5jar"
                                    "3Pklwk6yzfjvWuFT+2iGm58JzKjX7s7uGuq+4wznTxWDqIwsk6Ucms2A3QZbs1nGxR+3mUxsJC0gWyUyYT"
                                    "q/LjVow0xld8/FSvQGrjTOQSvs4TL8eRrHIBo9HchUV/i7W8n8CX1xFs7t/dzDwoAJs4RsuVhGM3QnPphc"
                                    "x62XRfoK+nNvkvxh0M2rUPXIEbmLEg+Xw/54Qt33+zE05nQvyglw1V9Y7FS1HLwc2BE49JUlMm7Daifirt"
                                    "CJTun7uktqIaOV7Wgm8j5fwoBlU8jVDDpjHrnVa4OOdz06SI2dU/P0ZTgGH2AJkZUr1Jy0JTsCjwzSsw/Q"
                                    "Iw7nvRlMFZuA3bHERKBrG9ukPkC2t+TFy+hllBHv/butmI8mwjG3dOCjvb/5qon2Rzqun0Wof7hTuWenfV"
                                    "ww2zWl3NFxeMmkW4w8uqCr4hIjJK5LZ0bdtwnUghd4730MapzvGZ5fsE5VxsXuIrtTltjqffOV9/9+zzIO"
                                    "+TEENne+Xqa3I+QL74w6K0cdtEn7kP9+l0/z2st/v29sKKIvD/x1MHLe4HyVKt57t/HNKNTl7rCdXbkjr/"
                                    "0n63UE41E519+0bJFxfT9774URkMsH6wiIPzTBRWW1LjzIyNj8gCrNzZAKHlngH1kLZ31ABt1ODv0sYp2f"
                                    "nC6/hPRb/rt/lVaUs5sM3/fW5AAa/VN5nVDJ8AIm3sj72TcMHi3QTibpZxbtOZmoPbmpKhDzIFsVF5FGTO"
                                    "AeKvK32IKTQSaqnXVs3eUM/fL+DKsk4fu+MMtzJH3wlGM1yaDZjm4NfSiTQBLgZCLp/eSiO0nS6ezgA8Pz"
                                    "pBMWAZ0WKp4fagQFxKBYkQ/MrVHN9blR6ye4zJW68wdkmVxI/c4omrqO3zpf98Kbb8VbZzD+ptNcCXtD0p"
                                    "vB32B1xB70XiqTVA4d9qkq5wDySNGMfzbohwJyxsqB7dl1ASYeHkz5Q37dF6DpLhV0bXrtqwgyvYSs4C5P"
                                    "yIdWQWQEX//USXdTyXHjE4acbiXw6I+Z5hq626mGVdfWrKzvSi3W79azz0rcUKv2hEu5NKPhkGXurOwCqK"
                                    "OwjdETuC0Tbz4p2Ua4waAR4/tJQdD1Vbj7pwnt9t6ncxFPyD+0uGA9sIlSOh132pOwdvjDCORIYa/vumvx"
                                    "fGbxcwf6ccuK/WriOZelxmZMg/dOTeNSp8FH9+TqOC8WClyeNS5Ddez/VKq7g1d/1mXrNXh4qQgNkFafF4"
                                    "VOzXunicPenzvAPAQ30PKxhpwv0Xv/BLszYAA1/Gy1mCPFgV+mYFWY3kkpZYGGe7PRtBLrXUY/DyWYgIXb"
                                    "alhH6el7PYj3vWpeD+zzkhz9ZoPzIEZbRXMe4anAXawgd9WoesJ1s8alL3rYxxKouIZqrL2Agj+vXM3nWh"
                                    "G/5e9nZ9m75lbuLJI75P38wrNScieU/WzK9x4muV7Krm/7Lz9LaNWqrhbRkunIQZf4z8NYSXgPWdpjUN/d"
                                    "opL5RKWOitBA3+2HavAhq7VeXtqwie303pNUlFgwKe5ZRBCw+K7iOeTp9YeFbKCcPk0PlRxhF3REa5G63K"
                                    "GxjpcxL7WqJfipSHvGu2cOvF5gs5Nj9NTRE/pq/q5k9MpHvYOsG7bT7IAffjPweebNMIfWAdrxYJ0RUWlS"
                                    "JBiYnEZ0xO/PD3dFop+1c3s1r+UEXvNsPfHM1LAVg49zmw5sAFIR89mMdgTGcSuoXi3MN1HskxZ7x4jupb"
                                    "hRmRKvGpMdQfSQRc9Noy/nMspnNaU1kx0HHd41zn6CHL5r3884jZhARqNQJy5Ut2Om7pZFP3fu/PELK6+a"
                                    "t3fofEHK4JPH5JEl8Dv6e9ebxGfXcSCT8QoYxvSD/35eQXpfO+Y6rxr+F4xjOXsBo1z/gk4khSteWgYb+D"
                                    "CDjHlqBn0VCa4I7oLKEU8q5E0MKy9gJSo1WxIGsYqhvkI61km6g073LgPaFNAagVBbS5wV+OYSIbdCzXdh"
                                    "O3C+dpdhA4xhfrLYedal/Mnd7iCxcIyaPw02FDQ5hyy4y9F8h/6GGkght5KdsQZyHxPMS0Fz0p5uX+/nAM"
                                    "/NYdmxEb/qMr03E8ur8ninYina2OfMSfNqy8HT6CZ55XLI2BWyDHL/LzDXj3SN1k6K8/F9L0BTpmSfhXhW"
                                    "fQ45NUdNIoGZF1bESPDRz0BhfenuPInNk5edzF8Lr6BPL8OCKrEwHmIhy1SaaD9BHqxlebszlCZw/p/357"
                                    "oXLq/gAxnk2YnZ2W3cPzsh2QdV+ATzlb/zkhjMBH2Y1wphPuieHJaYeBq8HPfgL3fudfYCuVuUHSu83CtE"
                                    "J7RrhUGpoFvnQcqb67CbGO4IH/dH8KiycloHkuFmeDDRUMD8S1xIU1ZwDoM7GNOyKPzOGpCX4hblahUSsU"
                                    "6pvcy3lIE22VYGMUlwyjbWaVhP4KauGpa+8uhW9MxtotsdMsmJOYuiUjhyizbg4B+N0hOwRWaUdFvwOsh3"
                                    "q+m1ZNZ+FFJDytj59VC9DJ5DyFgnES+f7WAPsvh5SglzNKWMDwQ1HtSfWu3ZgeMkBFIduxZeikAXPi8yGM"
                                    "Ug68uQHgjXkMz1txJHGC/0vNOlBdSzLufQyPROIBtqzDB15S/k27OK16iYunsT0vj9vBH8ryi4ed835ZBv"
                                    "VIFZTJybK1/79KLoSyCov5IduCQrzMFLR37NoH9F7MeNWr71uL51AQkE5TQYUAJ019f9JN3Uo3Z+73VZm2"
                                    "m2ctJYju97zsspd/ddNfqrUssHGWfRQJ4oEvbbehHkYGDTUAPDv+/Bvj9ia30jBFwHuwmBfoHjutzBnQz1"
                                    "owm/IG/MkMf04YyCW7UF8uThjyyZObBGX7g0Y6W4y7G71tESaawxsMhy8sxOg9/J65KRPpBNSTFFGhXj4I"
                                    "nRd2n8tUo1PCtHJ60zfxLPPMiwOqxYUqi3GxnbO7FvpqccOKE3UQVsx1KRROB43V0m8lX3cmEj5EFvVjKx"
                                    "LznZ31bdns0k3nvdh1Z+bZXSxzoxtrr+7LKyci+2+zxtuiDghllSvS5KZnKgV9Wbbw79VOCAEI9eRd+uF/"
                                    "EFvtcx0qddnYBWY1CMyB9MnMN/7IHJbuBXzPKNrmKjFpzjquzRp8XyCRqLMs5qFcqtlgHolw+ZN10JzBOz"
                                    "cgX2VcUUADvtP+vQbHBsVSeMNziY8k2895GyZoA53WQnSwvn/3qBD/nNQMuqWHqY19N5w8jYQOgYSZhFj7"
                                    "hdRtwZUyBVAlrQbikF354qtTpFnz/b4ehBFlwLbBfQMnoBjaBAZizGgxF/nnWMv80onbacP4Q0uYjYq43e"
                                    "z0msopq+fOngz2owkAH98r1TDebiN+dGq0QrOR7BY3HGBHqwnjoFMsAq8tBK5jfyvZ+DOHlPkAqru3bTGf"
                                    "LICDoxkRD31MsRjVeeq3RH4j+ektDbXjeoRHiyt5a4+qUGYF8L9dhL2yTB7lIGkCfxWQ+LTwcZABO8n8kZ"
                                    "ZB8Is+VbO3YrERUSsfOSKg3Pm/DagWbSzX1TpiSLbShcaflEPOL8bBL0CGpkrUdU0d5UFzhvO2nIp+koJY"
                                    "saJyXv56VyZHUjA0BH/ats+qGmTjYeu6vka/dvL4nsIqGOGzD4ql1Gck7fdfrOBwX1YB5GGeWTPEKODoow"
                                    "uMIaO0XJnDZkYyVMXZQzrPVcEpVD7dqhcPeZBm5v3cHVUQfX/d7jJz+NtYERR5jVFNxy2UHvjNATSEz0yD"
                                    "3mA0cWFLim4jLLYv+9B63ifNgptX4XEmrcpZ9M7VySdAX0QHFyMYP3/TDUJTVkyPfzDGaYezLuXwD6n/Vg"
                                    "7cUGuoi0r4f5oGz0pP17P016N5uOaq9zzo7+1KNUwPobj/efGsQE9O1bAjtD1s+AS2sT+64c38/nRStwmZ"
                                    "/Lr5culkUmNId67Opx/gQvWYTwkwq8rLXpL5PBUqOuP7nsShJJm8H/JYp+NpFFzQQjwZVvhuVOQuYXLmTb"
                                    "aVgzST9JtBZQ/cBHDCpDq8ptn1D/W+7htOXB/9vely25jWWJ/UoGX81UYyG4VEyPgxuYZCaQIgmAS6WCAQ"
                                    "JIEiQIQACYXMqK6CqHwzPRm2c8fukHjx1+8Gv1TNeMeqnqX8j8E3+Cz8UOrpkSU6pFqlCJBC/Ofs89y8UF"
                                    "xI6taavS18Up22A77AXkG1wbk7mrtWzC/CHlLluWLsZWX9MERWdf92v9O5bodzieakNOOL+uaBCDsK/56R"
                                    "R4yi/kdaPB833w9g7EcGaDrdAdkabNIYb2MrMMO63i7alQgzm/ZCss3iRlpjOlO0xnRIAN30kQ6/GYQ7Rr"
                                    "8iXkPIaIja/aJL1q42xP6LKXrD5eSVp/2gE/BSuvwPN0G/VLWh18plz0kT1fKLxmXpcdk78YwwLSwpuYVk"
                                    "Z7YriOVpaq/Fpe0zUeb65YYVyDuNJqQ0gAueSYx0wd8ivIF7QJeB8aYpNxG2tVxY52Jwi0LnbGVy2StkQM"
                                    "klBCux5C3AJ413zHgQClig9xGnxE/3KI968gj3RanQZ6sgZyFgHyUVhXOoUs39Ugk6yvZFyzJfBf7Sr4w2"
                                    "oL1rSWDfoGvvoVCZ0dU5PvOA7RKy1FkhavOyNsqDXsa64kiJB7XBIYztK0JhCg245ZFiottkW0VkNCWrJC"
                                    "gxhCXCuBP2+R7MurtZCRMPkOfF5Jmhbo9qRf4maN1zxpkspMFiFGqF4RNCbhktWu4iWwbxx84mtu3UQxXx"
                                    "nssSxWNJrXeiu+0yr19HEd8pYVM0F9HArig3EW8uweI8gziWuuWzR63gRySmGc6UAcJxJjuN4ft4Q+wRCa"
                                    "yfBUHfK9a7bmLHpdluQqmtokHEy+KHF817xsdzIZCSIoyIfb13RL48sFtbkuYcOaCRmLKShVp9YXGldtQq"
                                    "6CH9Q5Gp2TUQKvyFKM3rrsQe7L8jLX6mA4+INWa93ISLXGy6H7/Hw/C3OX4IhxS4F4mrmQ75qQ417T/Y7C"
                                    "ywxHs9ywW2KHF61lD5dIiGWvmMk4A/GeyVw0bEnorWWydClq0wXkEX1+0ii1Z2xl2GmBLcg4xxvL9mxJwr"
                                    "qwhnyDlvQxK2ijBYeZV51KH/I5QRQg97uGSTOcVpfomRqRZKc8PtYgF1jwGM739T7Z1MYOe0GbQqUhAjyx"
                                    "M8Prwswm+AuINHmThLi/1yYosDt5wnXrGYC34jjW4XWTbWIOrJHLjFxrjHmCtcFO6xDL2z1Ye5sQ+whVtK"
                                    "e8AfFsf94ilpkrvCXIU7wxnMjzPqnNe91xX6G1OUfLVx2+sLwGnyt0MIsV2L5Scy65GcTxnTzFo/4gkcfR"
                                    "Hjx+OlpLRIFkNHkJuoaMojHtC32a78ovwS4FyA+mHNHA5GmPuK4WLoRpv8ZflOpNgl+LhEZKYM0twqT6VV"
                                    "gXdGbdxksd+YJ9Cbkr2GUfF1dO73KN+sDLK6G6fCkQ7N2QaGI9soT28ahsrYDx68ZrpUONe5DLQDxZYnSQ"
                                    "oa7BOiys3HOYIL6TCYdulfMYd0F3euuG2VxrkKvQS5ZvqAxttqSLBiPjtNCZjdYy2JOA9V83dfqC1xszUa"
                                    "fbkCNrfLVvt4mWMezgbWGGC5d4q9SiGwvmgl+h/b38tNEbEjLEGBC/0X3wj+C/+UKnVdPWfK26FDRaZWot"
                                    "QaotRZkYmyL4YZnX7piaA7FqYdFeN3RuLZdYyDUZdMYEyZAiXSd6RKHbEliRqy4ySkeutHCzI08XkH9hGN"
                                    "DJSURjKnXGL0UN1RCaVrvLorNpahBXiU0c5ReFqsgvKI6sZ4ZT8zXbNXuc1iIhd+L7sPbw3dIK7SWH9Uoc"
                                    "EoU1j2vNTgdi/GnhtVSVSyDbNcQz3Z4mr3qcPBfW4wmsWxi7LpnKrLUAvwGx1SLTm8kaU9Mgnxu/Bh88Ft"
                                    "dymWsXsmI5D76gQLLkiJQ5rcNMjbVSgZy1k1lBXkLJXL/fx0tCmwD94H3jEmcJsN+LDthiR4CZ2DXZNjr/"
                                    "qlzAuFWh1SMFZiiY2X67QHEY/vqKEMY9HVZi1JeegeyqrYaIchy6t+6TZmMIsZ3ULlw3yRHYF231eVaA/E"
                                    "u47gri5VpmJbDx/qSBnr2xZd6gWB3iNo3u9jWTl2qtzHCGQ26CC0NcY4VqHhfR8zQTgWvOTAige2tGqOMd"
                                    "rXQBecDlJUapsjZdX2J4tV/VugzOTgSwOG46vuitBZOdtNY8Lc/5Gl0b8j3IRPNov5Q4rDQIyGFrIllcwf"
                                    "o5YTrasgWxqCLQZpNvTa5Ig+Bm9Gt2RnV7M6fOtAvNoUCjs5IcZapNpRrkfGR1CTnWrFNlxx2I6YRay+iv"
                                    "W2WJaLXaU63UxmFdrsos8AyZGWPB7CeFGt7guiMS9aIFJK9OfcVVW9n+BHLaGYWJnf7Loc4aEItkOWzchT"
                                    "VAYzv4eNjVCBbtS5rQlMIXGsyEpZRafSWRY5Ov0lOmO56Db0TnEojouZzhbHnJ1JZTZka3ZFyiOrp5KV1A"
                                    "nlEzJ31YW6+rzZXSFYgrgppcd02IVYSaCLpkK9Kq1bHvlFnh5XUV4nbw42DfOsxtB7LddYdrlC7J0ozTWI"
                                    "fjW0u+24BEvcXKF2YW/GKzN9HKyoSxIMex21qpwkCm2J46XYU3TbbaaF936WyHH18InRajaPRsWKUnELP2"
                                    "+SoOvqaKo/MIOMiZhqrj9AhtwXRNfVidgv8eO5DLaq0LDetMHbjX5MH+syytmQrdb/fQ2SFYod0iIL/F+h"
                                    "arNaosB2vXjHaaHF3rCKzB4zI9JKg5r5dWYEtrie9RbZqHHJuhFJq+7JP9sjg1SzyBZ1vdZoafSmh+CM3p"
                                    "iGjhrSrfFV7D/LrozFB9d9wV3H0/JUbsMDgPcVaHZ7DrqrFoQ5YEcfDd5bpxKWO9ZUuHtVeQl1yVbjYnmi"
                                    "WUHWZINCrNydiAHOs1ZEVkXxsL11WGaJJaH/x4n8caGlcRrHatAOGJrLUqzB3ogW3OCpBjNrrogZJLop4R"
                                    "u6bYxhokw7OWzFMZsIGe0u1DzItXBF1otWf9vkjQbak6LkPen23zGVzkZETHJQ8zDmKcLoOx1XZV0MF/8q"
                                    "JWxVsY2nfYo5pT6hpyOqs56bPXvNmHFRnk08LkKthxhc1KHMSKq8Ido9F1VhsTXNUx2/wS77QLRA8b21IH"
                                    "V4E3Z7hm+xzXmqD+J8f3lwzdcHiIXeTuCPIVeczNFhm0Z20oNOawLnfkDv4SnabHCQbe0mhGmbAdaV1dSZ"
                                    "WqBXJbCYREcBhVh/ha5GpCFXyqBuNNYdqoMFOKgFye7FSnmR7mtIfd8cV2/53t8Dv77/W7NuqLEHKTIzIr"
                                    "yFXpa36ZhXily15o02annmGr5kueMFlY1zSugzMc5pBtfrGEsHQtYabdU52mPNWsK3Jck7AxLQFekcDH7Z"
                                    "lGQb4DMW+h1+k47Q5E5jImLESyX+9BFjMkS1VRkydyRXbatVaPmS1NyM+vWzR9xc7Q2QEaJpG0w5D0QlpD"
                                    "nIzBGFj3waNRUkdTh2thKuhaW5nyS0mTM1JlzIK9QN7K3LEXRUKkWfIlhxnFmlnfPFu6OjEuL5uGXlYFDH"
                                    "Jd7Womw5ov2/0OvRC7zBzWQ+cKo0r+Pbx75i3OYr1uS6sWi71iuVi8ZCt5s0++pP7DtKyOquXyqCwWm2Wj"
                                    "2Cz+PJVOGcOJIjl26rMvUnNbkQeWYmqq4n6XlVtxrjmpz/S5pqVTpmLZqu0oOly5FTVbSafuRG2upD77PE"
                                    "Vfl/VW6lU65ViibqvRmDfplKp79zwB4BcpXZzBv+4QQxe1gWijoaLuvBiLuixZ4q2jyC+AYgsoFiVHNXR7"
                                    "YBq26qh3yuBWUeShKE1Tb3ZSJKuiZowGY4BoWKsnUubMLR3E8/kXKUuxTUCrBHfJMBqnCjhBUVgeh1sMVV"
                                    "IGjrKE66n7/3H/r/ffPfzi7P5PD7+4f3v/B/hw/+f7r+//5f5r9PE7+AsfvoZrbx/+7v4bNPLtGfz79/ff"
                                    "eJe/gTu/hP//8QzuAUhwy+8ffgUX/uXs/Ozh1w9f3X939je2qYhTxTpzcf/8JmVMbVEXb1Jnyu0t6BmuuP"
                                    "LQREcZeL8NFB1+1sAFwI/uZ4ChyPAFe1G4Sf3tShkZ1plkwbWzqSbeiWeSIYlnoqbaY0AE2oDPylJZAU7L"
                                    "GK6MO7CqQDQflvt3InXuOAqIRHKVq+rm3BnYxtxC3z0NwpiAC4++LxGpf0VEIvTw9w9A68OXZw+/ePjPD1"
                                    "/Cz4gfYPfs4bf3//7wS4+1bz3+voUfAMb9H91hbxE73zyfaJD9i7puOKI7QRCHC3UtWjL6FFm0MVX0AUhp"
                                    "pM9ck//8HE9jaTxNpMl0Jk2ls+lcOp8upHG4iKdxIo2TaTwDs92aa56rYOa2KtGKCJPDvQB4K6rlrK7ArL"
                                    "yvNcUoyrKFBvP6raqB0BW5qNsLBa6lvrhJlQx5dQM/36QE0VJhpts3aJbdpNoOKNSBLzepXQK8SaVvUhei"
                                    "fb3QAUddRrd5c/Ym1VHU0Rjdir3A87k3r+BSSbEdGAfXgEnva10H61DBn3jXcTyfyZHbd8OVlwaCTsGnK0"
                                    "UfOWP4QsAX1rBmIL61InNgKIcp5Vam4o4I2HqDlFTUwNBCMJYvMjBLxXWcRU2r6g74NsXzPAgISE1HUww5"
                                    "oHSqOYc7nRWitVAoZDEKxwtEGkTp6jgFI2hNHNnexytj9BLsXxyq3j3nVBoEMVJ1uD2dqupgHuC+2sEsAA"
                                    "JDlNIcfObsxa2oWquBA7MpgTtCKGrD+exnJJXBSCxzBHvmydhnA0uUVWNge4a9h4b7/xZOV5o5NQ0z0Zoq"
                                    "zuBO0WXD2kNA+ONJMevinToIPNMutNYcTWb3548i9v8Fxg8uERxg+9EkED4J5GES0DI1F0f7OYcwQdQfjZ"
                                    "X0sWYOY71TZcWAOEMyZugxeZf5gWTMdQeih71C+M5dC94+/PbU5IyAGFFGYRvEOnvwo1XgufA/2g5iyyEs"
                                    "VaFZnJoe0zDnmmgNRoYh23uI8cc8GTV1GLWtOI6qj/ZhjZnr49BmHof2yDSABf/8qVMh62POnX4q/COEIm"
                                    "gh/JMbmECMA8EaWh6fYpqPJO8xU8MN2T4QPbZkGZo20CEsGHhZyh6idg58HCl4YKv4vnmioNhhNbAV0ZLG"
                                    "L8bzWTJgyMEajf4U8LgnXRAZPE8UqMfTERgvnn03OiiSKKA/RCZBB5XPZ/Jk/tF05HwyCnuosFf2Czc82a"
                                    "OKGy94uTkWNJEhxvw7YYRALZPHCmQ+LvenIz+ufhe95UBCu59j9+cnYH2UkE+MsxAwui9OeQ6kx60aYYWE"
                                    "ZmYPcAwb5ChsL/KZcacqT8B9LC57PszHVt7nw/yYufQ8mEPzwj446sfN4WfCTT3ewClsEKTuH8HUngP7Y8"
                                    "zNNm4POBQZsjxU9jgzbs/ctOtsYeiyYkGMJp94rjsWqmnuo8T99dT2vo3yBYZlCjhOkVmSwt8H/2ON3njh"
                                    "FoP3sR3Uz95h8crvwXtrOx8F7+P4DauDp9T141h+FtSP4zpW/3yKjeEBduK9GH8u7I/nPSzyvlPssi+ffD"
                                    "z3z4M/4N+eW4dJ2Chsn2B9e5VOcajw7VVUr5TZTEzFS/ypmN1GPprb7AS43IR3x7oCD19CWvk2gELkElYR"
                                    "gNnoIiSAPXwZ3ExGJBD58Gb4PTE86j68vf9jcGsmwktGeDc7FUlAftciAEHlo/JEDHvY20hK4NtIctlICV"
                                    "Qh4vnbzVt2lMoDEPlYxp2JS3/7jgRI3zcHYAqxBZ+KwASjErf6XZhI/UTkymIkBMOS9wb+MbwbjxkiRsZu"
                                    "D0duAHDbPeHtBBEzmzh2f9yGHEMnFQIgo9I+TpCb9uuNTQB5G92ajy3V2ejWtxuiRo2o8KZMDB8ZU7o3ao"
                                    "vc0KuEALJYzH1kkwRHoxOAQt+QAJSLzbhsbMpttcjAEdCGNfM7K65LgQ+hywhgpFOs15NG5Zk7r/Blji3R"
                                    "VlwQwY8oAnwBqBQLta3tqappA1m1JeNOcStkW54Kp3IvsgUSJJd3HWOCACoeJvsobi1jNggrgOieoBIV1a"
                                    "b8oY6xY2CMqVw4cpuTHa13W1J00VINNyz0msjhJ69itc0dib8oECSBEZTrcX/AjVRaNVAvrjXXlJZiu7sF"
                                    "UmTKv75lV2dbFus1+doKiBNk5Xb46jYr3vntUhCm96nslg6jft8XqWtJmluWokve13DZSq5WW4tJ2ltCtp"
                                    "x9Ou6804HL3uOGY6405hoTni7mtTZ8kEtE6CG2ZvyWlMFCWmCvSnwKRqtvaMGRTyXCa7HSfXiNOngts2vq"
                                    "RLMkXmB98+qNH414lMVUtIpd3tf8DIKtqGmK0OxrUwajN7ss8XvCzqI/ONmNjI+MNQODrnHUP0R8HeVmo5"
                                    "+3l7zHwIq6Ej6YhHva0zzzhx5sPcRv3+h9+bcnWwNJdPsbVe/FbdR08sEkO1WPghFr9QQxc9BzetT9yV7Q"
                                    "Qakf7tM8VQtH2iy7tfIYjna2SgLZbP8GMNHs9cDRKirNuDtWRFXvqLrubj854v1vHDwDf7M3TtQWuXHcxs"
                                    "mNM57Pbpzb4WezFxi1Ns31f0Kf0XYYvxSNvpuKYWrKC29JvXFQL4MRHWms2NWlaVhoJSmP7cbgUl9iU1zl"
                                    "K/l2eSbM5XV9wXLFBVupo618JaY4uqw19fzr24s212k0Xy7rdUkXSIabLtkJg8ZcumMWGj5djqrCKDeGMZ"
                                    "doDMuNMmxltL4ujUr1orQoF41huaubjUm1bl0sBtWRuZK7En5dGWHXFSZTKxZHpXWxjujqzJo5EjtnyTXQ"
                                    "pTdweS2R7JpZs5yEcF4gnPXitBj+KdVfyui1ZFwTu+70cHbdXF+pmUUEM9+IBhfblRlOSWtZ683YaW9Na+"
                                    "FYDo1dNBRlOmOaqn2L5EJq4yE6gmmNHj2Tlmhc1R2HNc516vUdiGYG4yoXaByDMevikuXqBPBdrJeqiO9R"
                                    "+XZl5uUG29OKwHcBKJUcluNX15WWel3rza8mxZCvi6KWoy+pi0zXxECWV/0aq/e46oqp8Gt2Uo34b+oaVr"
                                    "4anndzs0DmzKS5vK7UKbblyvx1qT4tlUdFT/YBDbNiIHsMYOLMWqLisg9solcJZF9fMNxoycRwXxRn2XZz"
                                    "Ub7MKAj3ldKhHYaTgB9EZzM2LrSdERonEtqc4erUdaWXYSrMEV3WiV6nP2MrmvoIXc6YGU8ws5a6qcuEjb"
                                    "u6rIIce2CfzfUuvvmQ7+oS5sHymov4qS0SdHoyX9fBNuoEg/Qd2HkzGubLGuDAfJg0sTjOOB++rEEnowW7"
                                    "lmI4E/PPx9lcMZM6CXqOcCZszMVJMpUixU5GoX7BDQQu6PPUVvh64+RunILrc/wWKPI5FBn4HM+PfHDX9S"
                                    "od851lw93c63tfwxkraLW71h2jUhLQJloU6aawFxkKfwE5AHaO4edY7gWWH2MY6oZuBdM7wH4ewvX+db25"
                                    "n2AxEPfMzR1heRBuv3RXKnPuKHJye2JH9D6myqJj05DAIESuQKKg2hVUBkP/YDj6BySJvhQo0r2GYfER6J"
                                    "srPfQtn8X9b97C60EOSsYwIOvdjGWyrp5d3RYKhRjcYAQOyOb6VDcWuv9rCC+qx8IPeMEdTyDiAA7uASHw"
                                    "GEh/iPttL0g3lnfv9UBgZNaDhnvMY1icSPwoxHjtFBlwQEPWg4gTLtQ8EYMajMlkDkKNapIIAhlR4to8tS"
                                    "3S+JjHwN0xnUC1uQ1EiBH/a4AoPgY1r8KfIChx43YP2Q7ohTh0yoOO2MjFoPtjSCK3xcarrTkVzAQ0mV6K"
                                    "lq1YnIoSfJgy/qAfdUb5JuC/qIvm2LDE9nxoQyg4R3oA1sOvrquCMLNut5SFpTrggMLN/+GVlvIawtEfaA"
                                    "kjCoPb7g4c5Ajh0iz0odeWCgmvqP2QuUxU9d0EvrRClRWvv+p9xr0v5THkZmFBwPu2t8QfA+W3dbwvRC4G"
                                    "KywaeF9j5ctdhf4YSCIfA0nGyQurdf7XqHzt1/9jUMg4YZk4YUSSSzx3uBUQg5mJU0blYzBJPAGTiLjd6A"
                                    "3EoFGFGLQsGYeWFB1JbDcMYnCymRicfCYGJ5PklKTSR7sGMbh5Kga3EKePwhJw46XpeBshBquQSZgcEQeW"
                                    "JJKKlafjfYW4zWFkHBoeJy2bVASVTe9pMyRsOEEdEacumzS5LJHe2XeIQyMS1JFUDFqOTELLbc6wqAkRh0"
                                    "hmExDjhpdLSi8X64tsTSy3CRHBySQoKyThxFUativisDIJmrLxaZrPJWDlN/xIsnMRh5mNz1m3aRHCLCT1"
                                    "kI+abzubGCjqdr1eRXRE5M7r/oN3KTcORcG1+0CLu+WTRBstqYLbEoLAoO5VVf043tuZea0r7nYPHMMg4P"
                                    "XiCvjRC5zTCHrHhwbB7pvI7+4o3CKmt6hSZ6j04e7lCQjDI2pcd+KR4z1AOBiqTkAVvp+aHZTEemgfmZJ4"
                                    "oXknJX40l9AV5W6KzRHI+R7TVSaLwdgDutqmKery5R5PEyQS6A+O1pUjNEE+gkEW8iSaYj33wuOJcnkv5P"
                                    "DsI4yahMFPIymyZ3yPQe9UXt6liaQyx2nCgKY88TSq4v3hx1NVcDdaZ3AcP0JVATIP5RyjnkZUrAec2U0U"
                                    "yokSFOWzLkXZ/CN0B3b3pFkX7+LuoWenkFySyNwxikBGuafLKEYT9eaNW1RBLZyy6KCHWFdJR+72o0gyQy"
                                    "LR1m1Uf4meZj44d175/UmARquW7fjN3a3E6aDIojaI6u/SuRIjULvWIo+ZknJrWErHQM+bfv7FJo5EFy/E"
                                    "YBlIsCysc2HrDsEzLHMcbdTxovuaJc5mitu4T7XPUIVKP9ON2Zk9Ors9E3V1hrLhq6DVgZpPqZ3R/att9j"
                                    "fWrj1IBYQKFg/FOiPMM/P2EL7tnUEI8PtD+w6lEw9fPfxqFx+bK98ePl62DuBCacY25LjyDkAunomShPgy"
                                    "NUU/m3lagYT8bARfw8tbV/RD9CT3OW1KsR1gDJG1A9CzY0axCXqb7XiX+QDbMZQwYw5yE2y22oEs3qp+V+"
                                    "15ydM27Gh+U0/Rn8sN0pY/z3z9HZxnO5KubXo2I6PHTfOjGo1ys22Um4HPqVDGM7htozC1YyYRAHj45S6q"
                                    "CyHV+SdQfdwfxjLFHf5wMx7bizXEd5TJWCb5blPHA3FSBxCA3CGBrfDvVOaSSIJ3IN6K8PYgLl+zjUN43u"
                                    "6EHtueeNiRV4SD0yzMlxOeqdjiHnHXDrISm29OLe1YMr4D9Vbwuhc1sjOYzht2Fl05QEZyy+K2Cd+Ksycw"
                                    "tSsCOxpvvrQMSbFtRf5hV3u9SLM9NsxEyFzUVNH2Y0q2yFRROI7Wb4zMuJvRYXzdrhmGTBuWuxVDRQeV7A"
                                    "up95Qy9iHJ5tDxP0/DsRlZHMGRLbi597Pi+GHwAaZcUTR1pjrgCZANbFVOE4XTeJ0zWTYN59PZVnU+Fy/4"
                                    "x0uR+EZdPbMfCBmng4zTgSdrh3h2P5BMnJJMnBIiyQ1xgB0qUdIvxIvwRAJI7ImBLSDZeMU3G6/AkxsV+A"
                                    "MyySfK+PHabCbJTuYAO4U4JYU4JdRGYRzbDySozyer7VFBPQ7mAC1BYT5ZZI/q6PFC+AHhBvX4ZHU9KqDH"
                                    "wRywlqAQnyypR1XzePGbPAQmnwATt5hcUja5Q7LJJKjJxKnJJ7srucIBMNnEhM7GZ1I+nwCTzyXBvHG7wY"
                                    "7nhPyDsOCCdyyBHfREo4Ox/PMKbqONI+7N7hp9MXeXZBvczkCV3UtTVTdMQ7XR85IrMSi6L9SpaiqyKrrD"
                                    "58MQgqAZI/fIPG/Hf6qsaMrQQtvm0955Wt7OkaK/Y6BsARneqRaGrqm6MpDGoq4rmltpugWSdQntjkmnxq"
                                    "I9cLEPVP3WSH3mWHMlYDM4Qcp2RLTtEBmof9Seuw05ttT+Y7TUPvyDu9T6ggPOf+bSHpG+i9wYN1v0hrwA"
                                    "taqc2jhuQvECFqR3n8xcRGV4kNzvwq0DEV0xkg6JK0ZunMykFAO6ouMnXLoKbhvdiR9o4VsOsq1bw5ohM7"
                                    "EgplItRR7YmuEMRpYxN325o++OMRDBSj7zzpVLuxddov3v3il0aJPuwLak5AUZIrVXaNGzx6jM6CKwBwvV"
                                    "GQ9MS7lTjbk98Kjw9y6o9kDRNNW0VTu8pqBnlj2C0HGKmr9yBkKe6yA4yxa1gSRq2tB7AtgfCD+HFxEZLi"
                                    "i4CJYNS7B7KWAnnEnbe9tOGwMCPcYCTF5zVBNVYH0uw5lmOxYKJ14dZVzXRkdYTqdMESJ11xe4YbO7bQTN"
                                    "sLT/DM3ANRzQ72AAA9Cm6LiYkN5dGbkPIobHIz6vfJSlqUgOWOOdv5vaO6/StaBBMGGicyE9qnxFRlaqB8"
                                    "fTDVCf0QPhAhz4s8EXczolGboEgHRvq/xnaFYMVdRdM0x0RdQCicmqLQ41RY6M1e3EDXzH65P2Cu1R8ULC"
                                    "AJWyFCVEk+jtG9+4PbovCc+0DN2Y6zt/C4zF3YF+Fz5JsLXR6R1sbUvdO8A+VUUbLiJS1pbv+Amqbc/Gs1"
                                    "Oobg/o91Cf69B3qU+2f6Kzrhc7xvXSPca1jI5xLcaOcS16x7gK0TGuJ1CuJ9ynqtLydlFGKvQvnFh1Pj/f"
                                    "d919Xw4g/mjm4D1YF1lDcFDnT9EYvn9h32l0jO5FQXx4aSNYSv72Sfc/at1vzfh9I36adrB1LMlH09yOYP"
                                    "m5wuQfiG6eL05+Dw0l4+HnioR/IBr6qJ4u/Ggr1t2Gj9v67SepHwhQjytI9yrFJ9aOGyPHNOJ9f4QWdLdM"
                                    "+qNRgcfhMR0MDUM7kRLiiR7kOs6jV3fvyo8mS0exGQRlv48a/WjnwF/RmhJd+osbyX0Ni8zX6DyOj7fMxC"
                                    "bKfDRSbK/GYv803Vbq9VyVUJIOIlm4T1Z/ZF082nn9WHTxKmgVPfKYrZS7lyJ64dQXqZmC9lWgjpI/eiAC"
                                    "lJWjSrbb9hvI7saLVHVSWPS7jfWQoMZDujCWSNaUyNZ4qDfnYo2e92vsqtfpY/1Ocy53WU3SCqt+h57INe"
                                    "1uqLO3Uq2wFrstU54Jt/2ZoPVrjXGPsH8e9d2SeFGHxDJcmSGWvQ6kp8ngRWDv/VavdGoId/rnyAcFOB9J"
                                    "0IF7n/bYftU8kcygE/bK7QHJquWdc+MbR7yvjFD6wvrilIIC9mCqOKo+jzbUBLPUHljB1i9vo43XwJT8hB"
                                    "UMyns7k3+TPTbmmjzQ3BejhRMbAKlA4HDuOEZg9f609lprgZUEJ/XATHCfkvh///wPv05tCCVsxKGbBraq"
                                    "ITGglttKM0TZ7b97ZXj35jduP33g32Ip6GifgJBBEoJPhKShF8GFJwOlo46nKaOoylV8Ap+rF+/Xw6+lC2"
                                    "QefgjVgehENur2R6O34o3A2W3JOHBzEFQDL9t0GvrAF26STEk0fUfoi9XXrN+MxPCMhBWG5zlCoc4zBez2"
                                    "XMzi+Dmh3CokcYuJuIj624E3HmoG2A56K57rQnxF+z/6FIZcRmxG1IcUnkR6ICzX0GMvagv1vyFFL3LaLU"
                                    "R3ATxgjL95H2P8zffdGHVlJH4UY/zNBzXGkM3TGmMkvT3G+JtTGuP9P3llUhRA/9XdJPzd/Z/dw9O+uf/L"
                                    "QSvdaZ6HwR2120cY7HFDOMbSCS3kKXwnrQidqzpTJcvw199jNoRs8U3iBZLHcT6Xjby9/1dIxk5kIXFgH8"
                                    "c+kux8COvY4vnZbWMD48ks45+9TSX3f3DPz3mqKcTv/kC6TxL8LMre4ur02t1AcTJ1/neA+M3Zw2/dRywe"
                                    "/gtg+De3o/Kr//gO2j0A7AMp+yA7z6L7Yzyf3hQOYzyxZbi+xHuSCEwPPQn87oaxC9YHtYvdzDyjWezl+L"
                                    "msYg/CkxnF72BF+RMA/q2HyfdIZ/f/jhYfQAcrzhkY41duP/0v72IqT8XwgQzo6Yw/i1m9k3ROb2zvQMaJ"
                                    "TfA7iHPc/Rse7m8RKTAB4MM7OahHAP2ghnaEvWe0reMyeC5zOob5ZBb0v11kf374FfrXQ/nHh/+KekhnYL"
                                    "3osdS/ILT3b9/Fkp4A/ANZ1JPYfRbLeqpMTm9hT6PgZJb2PyE5/5PrBN1Hnn4HH7518/U/wf+/eQfrOgLw"
                                    "A1nUUbaexYoew/vpLec41tNVif/p12c3Ka/N7fazf3//nfsqsnctHO+G972qJUfdSP85t/gDXv5t3oNO23"
                                    "sYYnsFYhzGtOy1c2P0+t1a92G9kTIIzWIP5IQq3U0e7uN5UYXbf/bx+Qrc+yziuWvefvMTCcjSFWdwGz2h"
                                    "9u6V7x26Torye6/73fX5vdPsZOvI/wVfA8vWVw+/hLDaXaq+efi7d4t094L6QGvHAVae26gNfWiIlow2YQ"
                                    "wWY9EZSKI+WBnzgWy809JwSJRPUv2rtPcgR/gY2P858+IRLwT++v7fUGqPkiu0Ccrb8f5nQH3/3cNXZ26V"
                                    "9w/uO4m+un/7AgiXgMlEd9xWZ6bmQ3efYQ4EdAp8iHpx7hgeOwNZ0cTVYGaHIpPRg+AUOoiDwrHsaTkNtq"
                                    "ycCFxMu1/4z8rZwSs9g314wUOWSb8DfIq6bnivb3K3XPgHAMQf+HfQOSPofVkjfebuwPgcA9lZc83bpOE+"
                                    "N558jURFtZwVOpfH+1pTjKIsW94X99h5NtjS5F+sAfmK452IokUvefs8fCWqjsSFNvkkXrpcyBcogszksE"
                                    "z0WtTU4VegRgcZHXu3tOS+cOlF7P1ku9/D6r7h+WcklYP/Mpkj6DOPRv987zb/8C/V3o3xVeJU+ugFrxsr"
                                    "4+bZV9F5JZvG/GrvL8F7Ct88x/sGN5C9Ovy6vqe9NG/LuHwdxd+at+9VXpuv0sIaeafB9sal3pwLXwW1ZC"
                                    "fSgqlMCXYZvgpKLremVWGUcWwLG1RHS6rXwdYMN1ozFQlnvFcNtdxXJzX1hmxlb2vqeeHlss64r1haMwtm"
                                    "wq+YSZOIv4aqqM4vaEe/dtYh7jU7ma6ZdRW/boW4pXJbkyb5K/zyEr1+y3FxsxUmw06qVILGZt0aL4f9tS"
                                    "WFNLIcT7JclWK4eozGmbGc2qTKXxRjNK7Y9Wh9zdWxgMbUe7yh581J373zXq8q2TTG53rTx9bcC03w0a/R"
                                    "2JrpOw5f2xqzdarVIw/ChRgaHUKnB4d1elfsUeKKd3TdxpXkGAirNsagK8kx/oF3G1eSY1Td3hiDriTHoK"
                                    "PukmP8w+/0oweo7vCMB05vS3rife8HiR/blDxu6aBL3nMWnXtyNDpEhkgnjkGjW9fMwD8L7T0OpNt1qNGb"
                                    "o0fRvHrWE2RiNR37/Q6RObhT+kmbYV3li6bpZS93YIQooJy73wYD9OEzWRHloaLcnmfzhfw5ZDfiuXhbIM"
                                    "9zYj5P4KI4JGBlQtJG+7JVl+ZAQ4G4ZeVWjG1HRkS7G2adUHRhtPlJM4Fmdkp0I1x/gmA/RfafIvtPkf2n"
                                    "yP5TZP8psv8U2X+K7H9kkf1WrPQmFtea4lZUiw+VnFzISudDXCycZ/AceZ7HCrnznJSR8/mckscJKvXm/w"
                                    "NT9WsC";

TState MakeState(i32 d, TStringBuf s) {
    TState state;
    TTestState proto;
    proto.SetD(d);
    proto.SetS(s.data(), s.size());
    state.MutableState()->PackFrom(proto);
    return state;
}

TResponseBuilderProto MakeResponseBuilder(const TString& requestId, const TString& intent, const TString& text,
                                          const NMegamind::IGuidGenerator& guidGenerator) {
    auto onProtoModify = [&requestId](NMegamind::TSpeechKitInitContext& initCtx) {
        initCtx.Proto->MutableHeader()->SetRequestId(requestId);
    };
    auto request = TSpeechKitRequestBuilder{TStringBuf(R"(
            {
                "application": {
                    "timestamp": "1"
                },
                "request": {
                    "event": {
                        "type": "text_input"
                    }
                }
            }
        )")}.SetProtoPatcher(onProtoModify).Build();
    TResponseBuilderProto storage;
    TResponseBuilder builder{request, CreateRequest(IEvent::CreateEvent(request.Event()), request), intent, storage, guidGenerator};
    builder.AddSimpleText(text);
    return builder.ToProto();
}

template <typename TProto>
bool Equivalent(const TProto& lhs, const TProto& rhs) {
    return google::protobuf::util::MessageDifferencer::Equivalent(lhs, rhs);
}

TSemanticFrame BuildFrame() {
    TSemanticFrame frame = TSemanticFrameBuilder()
        .SetName("some frame")
        .AddSlot("some slot", {"slot type"}, "slot type", "slot value", /* isRequested */ true)
        .Build();
    return frame;
}

google::protobuf::RepeatedPtrField<TClientEntity> MakeEntities() {
    google::protobuf::RepeatedPtrField<TClientEntity> entities;
    TClientEntity& entity = *entities.Add();
    entity.SetName("separate_entity");
    TNluHint& item = (*entity.MutableItems())["first_item"];
    TNluPhrase& phrase = *item.AddInstances();
    phrase.SetLanguage(L_RUS);
    phrase.SetPhrase("some text");
    return entities;
}

Y_UNIT_TEST_SUITE(Session) {
    Y_UNIT_TEST(MakeSession) {
        NMegamind::TMockGuidGenerator generator;
        EXPECT_CALL(generator, GenerateGuid()).Times(1).WillRepeatedly(Return("deadbeef"));

        NAlice::NScenarios::TLayout layout;
        layout.SetOutputSpeech("Hello!");

        const auto state = MakeState(8, "hello");
        const auto responseBuilder = MakeResponseBuilder("request-id", "intent", "text", generator);
        const TDialogHistory history({{"hi-hi", "hi", "hello", "SomeScenario", 0, 0}});
        const TSemanticFrame frame = BuildFrame();
        const ::google::protobuf::RepeatedPtrField<TClientEntity> entities = MakeEntities();
        const ui32 consequentIrrelevantResponseCount = 2;
        const ui32 activityTurn = 3;
        google::protobuf::Map<TString, NScenarios::TFrameAction> actions;
        actions["action"].MutableFrame()->SetName("semantic_frame");

        const TString& scenarioName = "some name";
        const TString& productScenarioName = "some product name";

        auto scenarioSession = NewScenarioSession(state);
        scenarioSession.SetActivityTurn(activityTurn);
        scenarioSession.SetConsequentIrrelevantResponseCount(consequentIrrelevantResponseCount);
        const ui64 lastWhisperTime = 5213125;
        const auto session = MakeSessionBuilder()
            ->SetPreviousScenarioName(scenarioName)
            .SetPreviousProductScenarioName(productScenarioName)
            .SetScenarioSession(scenarioName, scenarioSession)
            .SetScenarioResponseBuilder(responseBuilder)
            .SetDialogHistory(history)
            .SetLayout(layout)
            .SetResponseFrame(frame)
            .SetResponseEntities(entities)
            .SetActions(actions)
            .SetLastWhisperTimeMs(lastWhisperTime)
            .Build();

        UNIT_ASSERT(session);

        const auto& constructedScenarioSession = session->GetPreviousScenarioSession();
        google::protobuf::Map<TString, NScenarios::TFrameAction> buildedActions = session->GetActions();
        UNIT_ASSERT_EQUAL(buildedActions.size(), 1);
        UNIT_ASSERT_STRINGS_EQUAL(buildedActions.begin()->second.MutableFrame()->GetName(),
                                      actions.begin()->second.MutableFrame()->GetName());

        UNIT_ASSERT_STRINGS_EQUAL(session->GetPreviousScenarioName(), scenarioName);
        UNIT_ASSERT_STRINGS_EQUAL(session->GetPreviousProductScenarioName(), productScenarioName);
        UNIT_ASSERT(Equivalent(constructedScenarioSession.GetState(), state));

        UNIT_ASSERT(session->GetScenarioResponseBuilder().Defined());
        UNIT_ASSERT(Equivalent(*session->GetScenarioResponseBuilder(), responseBuilder));

        UNIT_ASSERT_VALUES_EQUAL(session->GetDialogHistory().GetDialogTurns(), history.GetDialogTurns());

        UNIT_ASSERT(session->GetLayout().Defined());
        UNIT_ASSERT_MESSAGES_EQUAL(*session->GetLayout(), layout);

        UNIT_ASSERT(session->GetResponseFrame().Defined());
        UNIT_ASSERT_MESSAGES_EQUAL(*session->GetResponseFrame(), frame);
        UNIT_ASSERT_VALUES_EQUAL(session->GetResponseEntities().size(), 1);
        UNIT_ASSERT_MESSAGES_EQUAL(session->GetResponseEntities().Get(0), entities.Get(0));
        UNIT_ASSERT_EQUAL(constructedScenarioSession.GetConsequentIrrelevantResponseCount(), consequentIrrelevantResponseCount);
        UNIT_ASSERT_EQUAL(constructedScenarioSession.GetActivityTurn(), activityTurn);
        UNIT_ASSERT_EQUAL(session->GetLastWhisperTimeMs(), lastWhisperTime);
    }

    Y_UNIT_TEST(SerDes) {
        NMegamind::TMockGuidGenerator generator;
        EXPECT_CALL(generator, GenerateGuid()).Times(1).WillRepeatedly(Return("deadbeef"));

        NAlice::NScenarios::TLayout layout;
        layout.SetOutputSpeech("Hello!");
        google::protobuf::Map<TString, NScenarios::TFrameAction> actions;
        actions["action"].MutableFrame()->SetName("semantic_frame");


        const auto responseBuilder = MakeResponseBuilder("request-id", "intent", "text", generator);
        const TDialogHistory history({{"hi-hi", "hi", "hello", "SomeScenario", 0, 0}});

        const TString& scenarioName = "some name";
        auto scenarioSession = NewScenarioSession(MakeState(0, "hi"));
        scenarioSession.SetConsequentIrrelevantResponseCount(2);
        const auto src = MakeSessionBuilder()
            ->SetPreviousScenarioName(scenarioName)
            .SetScenarioSession(scenarioName, scenarioSession)
            .SetScenarioResponseBuilder(responseBuilder)
            .SetDialogHistory(history)
            .SetLayout(layout)
            .SetActions(actions)
            .Build();

        const auto dst = DeserializeSession(src->Serialize());


        UNIT_ASSERT(dst);
        UNIT_ASSERT_STRINGS_EQUAL(src->GetPreviousScenarioName(), dst->GetPreviousScenarioName());
        const auto& srcScenarioSession = src->GetPreviousScenarioSession();
        const auto& dstScenarioSession = dst->GetPreviousScenarioSession();
        UNIT_ASSERT(Equivalent(srcScenarioSession.GetState(), dstScenarioSession.GetState()));

        UNIT_ASSERT(src->GetScenarioResponseBuilder().Defined());
        UNIT_ASSERT(dst->GetScenarioResponseBuilder().Defined());
        UNIT_ASSERT(Equivalent(*src->GetScenarioResponseBuilder(), *dst->GetScenarioResponseBuilder()));

        UNIT_ASSERT_EQUAL(src->GetDialogHistory().GetDialogTurns(),
                          dst->GetDialogHistory().GetDialogTurns());

        UNIT_ASSERT(src->GetLayout().Defined());
        UNIT_ASSERT(dst->GetLayout().Defined());
        UNIT_ASSERT_MESSAGES_EQUAL(*src->GetLayout(),
                                   *dst->GetLayout());

        google::protobuf::Map<TString, NScenarios::TFrameAction> buildedActions = dst->GetActions();
        UNIT_ASSERT_EQUAL(buildedActions.size(), 1);
        UNIT_ASSERT_STRINGS_EQUAL(buildedActions.begin()->second.MutableFrame()->GetName(),
                                      actions.begin()->second.MutableFrame()->GetName());
    }

    Y_UNIT_TEST(Update) {
        NMegamind::TMockGuidGenerator oldGenerator;
        EXPECT_CALL(oldGenerator, GenerateGuid()).Times(1).WillRepeatedly(Return("dead"));

        NAlice::NScenarios::TLayout layout;
        layout.SetOutputSpeech("Hello!");
        google::protobuf::Map<TString, NScenarios::TFrameAction> actions;
        actions["action"].MutableFrame()->SetName("semantic_frame");


        const ui32 originalConsequentIrrelevantResponseCount = 0;
        const TDialogHistory history({{"hi-hi", "hi", "hello", "SomeScenario", 0, 0}});

        const TString& oldName = "old name";
        const auto original = MakeSessionBuilder()
            ->SetPreviousScenarioName(oldName)
            .SetScenarioSession(oldName, NewScenarioSession(MakeState(0, "hi")))
            .SetScenarioResponseBuilder(MakeResponseBuilder("old-request-id", "old intent", "old text", oldGenerator))
            .SetDialogHistory(history)
            .SetActions(actions)
            .SetLayout(layout)
            .Build();

        UNIT_ASSERT(!original->GetResponseFrame().Defined());
        UNIT_ASSERT_EQUAL(
                original->GetPreviousScenarioSession().GetConsequentIrrelevantResponseCount(),
                originalConsequentIrrelevantResponseCount);

        const TString name = "new name";
        const auto state = MakeState(1, "bye");

        NMegamind::TMockGuidGenerator generator;
        EXPECT_CALL(generator, GenerateGuid()).Times(1).WillRepeatedly(Return("beef"));

        NAlice::NScenarios::TLayout expectedLayout;
        expectedLayout.SetOutputSpeech("Bye!");

        const auto responseBuilder = MakeResponseBuilder("new-request-id", "new intent", "new text", generator);
        const TDialogHistory expectedHistory({{"hi-hi", "hi", "hello", "SomeScenario", 0, 0}, {"who is here", "", "dunno", name, 0, 0}});
        const TSemanticFrame frame = BuildFrame();

        const ui32 consequentIrrelevantResponseCount = originalConsequentIrrelevantResponseCount + 1;

        auto scenarioSession = NewScenarioSession(state);
        scenarioSession.SetConsequentIrrelevantResponseCount(consequentIrrelevantResponseCount);
        const auto updated = original->GetUpdater()
            ->SetPreviousScenarioName(name)
            .SetScenarioSession(name, scenarioSession)
            .SetScenarioResponseBuilder(responseBuilder)
            .SetResponseFrame(frame)
            .SetDialogHistory(expectedHistory)
            .SetLayout(expectedLayout)
            .Build();

        const auto& updatedScenarioSession = updated->GetPreviousScenarioSession();

        UNIT_ASSERT_STRINGS_EQUAL(updated->GetPreviousScenarioName(), name);
        UNIT_ASSERT(Equivalent(updatedScenarioSession.GetState(), state));

        UNIT_ASSERT(updated->GetScenarioResponseBuilder().Defined());
        UNIT_ASSERT(Equivalent(*updated->GetScenarioResponseBuilder(), responseBuilder));

        UNIT_ASSERT_VALUES_EQUAL(updated->GetDialogHistory().GetDialogTurns(),
                          expectedHistory.GetDialogTurns());

        UNIT_ASSERT(updated->GetScenarioResponseBuilder().Defined());
        UNIT_ASSERT(updated->GetLayout().Defined());
        UNIT_ASSERT_MESSAGES_EQUAL(*updated->GetLayout(),
                                   expectedLayout);

        google::protobuf::Map<TString, NScenarios::TFrameAction> updatedActions = updated->GetActions();
        UNIT_ASSERT_EQUAL(updatedActions.size(), 1);
        UNIT_ASSERT_STRINGS_EQUAL(updatedActions.begin()->second.MutableFrame()->GetName(),
                                   actions.begin()->second.MutableFrame()->GetName());

        UNIT_ASSERT(updated->GetResponseFrame().Defined());
        UNIT_ASSERT_MESSAGES_EQUAL(*updated->GetResponseFrame(), frame);
        UNIT_ASSERT_EQUAL(updatedScenarioSession.GetConsequentIrrelevantResponseCount(), consequentIrrelevantResponseCount);
    }

    Y_UNIT_TEST(FallbackToOldScenarioState) {
        const TString session = "eJyNVE2XojoU/C+9dWEEnJbZYbAjGhlBWg2bPgrTQERth6/WOe+/v3sD+nreahYeSSB161bVze+nt7fjz2R3zE"
                                "7x29vT9yea5JPd5qXYa3EdH0erYzKZ581k7m39YcReq1Az61D7zMV2/BFdh3KvkTo6vVY/Ny9VyNYGP74Uu42b"
                                "89M4jbTyY88a2M/TPTNPez2sYf+61/2a5y4RWz+f5M58Zh+0hUx0cfR0vlkMhXRKl62PIR1kiyBO+SbMReAMQz"
                                "vMXHt8FKlzpplHXM09uDJMF5QMfwSp5IF3W0ivFHDGzQaHhbSurozlQq7zcCM8Oj5b88S6nJrJKNLHudiUeZSN"
                                "tb2WyzAgWaSlHzEzNbEaDqKr0yMzADPplXDp1TE1iOBeuWoSIkYJ4dwrXDZ2XGoS/grrvleX8Cz6XmXShme2dz"
                                "KZRfizn8SMEm7AualXlbQhfO9VLu7huRtiDzi1vdKlDeAZhNdeBWvcJ8KpYO+dCBvxCeAhvqY4wTeVS2/wzk9u"
                                "o4tFL4kVfN4itk532vrqTMMavViuZrcdywnon/LT+ubI547bFvjgP5yPsQ+joiWNYe3VJu113Lq+Ye/R6y/onX"
                                "4jYunLTG+/4VM/MYGfQH3guWx7T038bpEg3/KqcAeoA55ptbMSxd9k404fqIsYfdCecaxVrowLe9R+x9qfUBv0"
                                "YgsHeQDfKrvMxn/bO+jaXGz0tvmDd9cXYPY7nQfwDP5voR/kbRX/1bM7X7fQk+JhIKbyjEN/WTMns9qXyrMkwb"
                                "0ypnqLjVqyJeqKPhcl9I69gceFqqfjeVi/Y91eW7PLEWiVgY6FqhOp3AFuHzNTUKz56OETeyhjxu79tP5JP8nu"
                                "PqjaNhGG4tdmkvvJFXFUnkfABTy2u9yelVdS1QNN6B844NXhgeO42BNHjCG3znMyB1yV7S7nlCRkpqtc/c8HqL"
                                "3FPD4r3dSsPfuZOjtVs4X5khT94koL6KuHM1XG95lS3PvtzCAPiXlEj0bKr5gtHx4rnqgZaouYAfAETfgSZ6vz"
                                "335wr5UHBfCawE9Hf4w2rxx9+NZmX2UY53atMm2qrF/x3QHnSM0eaqZm3ZfKFwOypXiNkFdhMvvhW3uPoOe97t"
                                "5pucIMFEq74Av/rZ/FzHeUJgHy7XC27YwL48t9gjMHHqkst7kuy8e83/OG301BI8jFcG5NztYqVfchaqOrPmLU"
                                "KwBP7jn58CxmWU///Au9QQMY";
        const auto& deserialized = DeserializeSession(session);
        UNIT_ASSERT_EQUAL(deserialized->GetPreviousScenarioName(), TStringBuf("Dialogovo"));
        UNIT_ASSERT(deserialized->GetScenarioSession("Dialogovo").GetState().HasState());
    }

    Y_UNIT_TEST(FallbackToOldScenarioState2) {
        const auto& deserialized = DeserializeSession(VinsSession);
        UNIT_ASSERT_EQUAL(deserialized->GetPreviousScenarioName(), TStringBuf("alice.vins"));
        UNIT_ASSERT(deserialized->GetScenarioSession("alice.vins").GetState().HasState());
    }

    Y_UNIT_TEST(StackEngineCore) {
        NMegamind::TStackEngine stackEngine{};
        constexpr auto newItem = [](const TString& scenarioName) {
            NMegamind::IStackEngine::TItem item{};
            item.SetScenarioName(scenarioName);
            return item;
        };
        const auto firstItem = newItem(/* scenarioName= */ "first");
        const auto secondItem = newItem(/* scenarioName= */ "second");
        stackEngine.Push(firstItem);
        stackEngine.Push(secondItem);

        const auto serializedSession = MakeSessionBuilder()
                                           ->SetStackEngineCore(stackEngine.GetCore())
                                           .SetPreviousScenarioName({})
                                           .SetScenarioSession(/* name= */ {}, NewScenarioSession(/* state= */ {}))
                                           .Build()
                                           ->Serialize();

        auto session = DeserializeSession(serializedSession);
        auto reconstructedStackEngine = NMegamind::TStackEngine{session->GetStackEngineCore()};

        UNIT_ASSERT_MESSAGES_EQUAL(reconstructedStackEngine.Pop(), secondItem);
        UNIT_ASSERT_MESSAGES_EQUAL(reconstructedStackEngine.Pop(), firstItem);
        UNIT_ASSERT(reconstructedStackEngine.IsEmpty());
    }
}

} // namespace
