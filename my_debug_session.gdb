set args --unicode-subst=«U+%04X» --byte-subst=«0x%02x» --widechar-subst=«%08x» -f ASCII -t UTF-8 tmp-in
set breakpoint pending on
b src/iconv.c:1191
b libcharset/lib/localecharset.c:locale_charset
b lib/loop_unicode.h:312
b src/iconv.c:766
b src/iconv.c:312
b src/iconv.c:320
b lib/iconv_open1.h:101
b lib/iconv_open1.h:115
b lib/iconv_open1.h:155
b lib/iconv_open1.h:214
b lib/iconv_open1.h:228
b lib/iconv_open1.h:268
b src/iconv.c:552
b src/iconv.c:1133
b src/iconv.c:1047
b src/iconv.c:706
b src/iconv.c:791
b src/iconv.c:798
b src/iconv.c:811
