## nls文件导出txt

### [nls文件结构](https://gist.github.com/ynkdir/b92727e2a52e55a4010f)

```
svn://svn.reactos.org/reactos/trunk/reactos/tools/create_nls/

HKEY_LOCAL_MACHINE\System\CurrentControlSet\Control\Nls

format of NLS file such as C:\Windows\System32\C_932.NLS

================================================================================
  NLS CODEPAGE (C_XXX.NLS)
================================================================================
                +---------------------------------------------------------------
        HEADER  | WORD      wSize  size in word (0x0D)
                | WORD      CodePage
                | WORD      MaxCharSize
                | BYTE[2]   DefaultChar
                | WORD      UnicodeDefaultChar
                | WORD      unknown1  (maybe Unicode char of DefaultChar)
                | WORD      unknown2  (maybe CodePage char of UnicodeDefaultChar)
                | BYTE[12]  LeadByte
                +---------------------------------------------------------------
   MB2WC TABLE  | WORD      offset of Unicode to CP table in word
                | WORD[256] primary CP to Unicode table
                | WORD      OEM glyph table size in words
                | WORD[size] OEM to Unicode table
                | WORD      Number of DBCS LeadByte range
                | if range != 0:
                |   WORD[256] offsets
                |   WORD[num_of_leadbyte][256] sub table
                +---------------------------------------------------------------
   WC2MB TABLE  | WORD      Unknown (It seems 0x0000 for MaxCharSize==1, 0x0004 for MaxCharSize==2)
                | BYTE[65536] or WORD[65536] (depends on MaxCharSize) Unicode To CP table
                +---------------------------------------------------------------
================================================================================
```

### nlsdump.c

```c
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>

typedef struct {
    WORD wSize; /* in words 0x000D */
    WORD CodePage;
    WORD MaxCharSize; /* 1 or 2 */
    BYTE DefaultChar[MAX_DEFAULTCHAR];
    WCHAR UnicodeDefaultChar;
    WCHAR unknown1;
    WCHAR unknown2;
    BYTE LeadByte[MAX_LEADBYTES];
} NLS_FILE_HEADER;

static size_t
fsize(FILE *f)
{
    size_t p;
    size_t s;

    p = ftell(f);
    fseek(f, 0, SEEK_END);
    s = ftell(f);
    fseek(f, p, SEEK_SET);
    return s;
}

static void *
readfile(char *path, size_t *psize)
{
    FILE *f = fopen(path, "rb");
    size_t s = fsize(f);
    void *p = malloc(s);
    fread(p, 1, s, f);
    fclose(f);
    *psize = s;
    return p;
}

static void
writefile(char *path, void *data, size_t size)
{
    FILE *f = fopen(path, "wb");
    fwrite(data, 1, size, f);
    fclose(f);
}

int main(int argc, char **argv)
{
    size_t s;
    WORD *base;

    base = readfile(argv[1], &s);

    NLS_FILE_HEADER *nls = (NLS_FILE_HEADER *)&base[0];
    printf("Size = %d\n", nls->wSize);
    printf("CodePage = %d\n", nls->CodePage);
    printf("MaxCharSize = %d\n", nls->MaxCharSize);
    printf("DefaultChar = %02x%02x\n", nls->DefaultChar[1], nls->DefaultChar[0]);
    printf("UnicodeDefaultChar = %04x\n", nls->UnicodeDefaultChar);
    printf("unknown1 = %04x\n", nls->unknown1);
    printf("unknown1 = %04x\n", nls->unknown2);
    printf("LeadByte = %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x\n", nls->LeadByte[0], nls->LeadByte[1], nls->LeadByte[2], nls->LeadByte[3], nls->LeadByte[4], nls->LeadByte[5], nls->LeadByte[6], nls->LeadByte[7], nls->LeadByte[8], nls->LeadByte[9], nls->LeadByte[10], nls->LeadByte[11]);

    WORD *cp_to_wc_table_size = base + nls->wSize;
    WORD *cp_to_wc_table_base = base + nls->wSize + 1;

    printf("offset of Unicode To CP table in words = %d\n", *cp_to_wc_table_size);

    WORD *primary_table_base = cp_to_wc_table_base;
    printf("primary table\n");
    for (int i = 0; i < 256; ++i)
        printf("0x%02x => U+%04x\n", i, primary_table_base[i]);

    WORD *oem_table_size = primary_table_base + 256;
    WORD *oem_table_base = primary_table_base + 256 + 1;
    printf("oem size = %d\n", *oem_table_size);
    for (int i = 0; i < *oem_table_size; ++i)
        printf("oem[0x%02x] => U+%04x\n", i, oem_table_base[i]);

    WORD *num_of_dbcs_leadbyte_range = oem_table_base + *oem_table_size;
    WORD *dbcs_table_base = oem_table_base + *oem_table_size + 1;
    WORD *dbcs_leadbyte_offs = dbcs_table_base;

    WORD num_of_dbcs_leadbyte = 0;
    if (*num_of_dbcs_leadbyte_range != 0) {
        for (int i = 0; i < 256; ++i) {
            if (dbcs_leadbyte_offs[i] != 0)
                num_of_dbcs_leadbyte++;
        }
    }

    WORD dbcs_table_size = 0;
    if (*num_of_dbcs_leadbyte_range != 0)
        dbcs_table_size = 256 + 256 * num_of_dbcs_leadbyte;

    printf("num_of_dbcs_leadbyte_range = %d\n", *num_of_dbcs_leadbyte_range);
    printf("num_of_dbcs_leadbyte = %d\n", num_of_dbcs_leadbyte);

    if (*num_of_dbcs_leadbyte_range != 0) {
        for (int i = 0; i < 256; ++i)
            printf("off[%d] = %d(%x)\n", i, dbcs_leadbyte_offs[i], dbcs_leadbyte_offs[i]);
        for (int i = 0; i < 256; ++i) {
            if (dbcs_leadbyte_offs[i] == 0)
                continue;
            for (int j = 0; j < 256; ++j)
                printf("0x%02x%02x => U+%04x\n", i, j, dbcs_table_base[dbcs_leadbyte_offs[i] + j]);
        }
    }

    /* WORD *p_unknown = cp_to_wc_table_base + *cp_to_wc_table_base; */
    WORD *p_unknown = cp_to_wc_table_base + *cp_to_wc_table_size - 1;
    printf("p_unknown = %04x\n", *p_unknown);

    /* WORD *wc_to_cp_table_base2 = cp_to_wc_table_base + *cp_to_wc_table_size + 1; */
    WORD *wc_to_cp_table_base2 = cp_to_wc_table_base + *cp_to_wc_table_size ;
    BYTE *wc_to_cp_table_base1 = (BYTE *)wc_to_cp_table_base2;

    for (int i = 0; i < 65536; ++i) {
        if (nls->MaxCharSize == 1)
            printf("U+%04x => 0x%02x\n", i, wc_to_cp_table_base1[i]);
        else
            printf("U+%04x => 0x%04x\n", i, wc_to_cp_table_base2[i]);
    }

    return 0;
}

```
### nls2txt.c

另外一个可以使用的导出程序是reactos自带的工具nls2txt，具体代码可能参考nls2txt目录，
由于程序用到Windows没有公开的api: `GetUName`，所以在mingw下编译需要`-lgetuname`参数；
源码来自于[ReactOS官网](https://doxygen.reactos.org/dc/dbe/modules_2rosapps_2applications_2devutils_2nls2txt_2nls_8c.html)
可以去官网直接了解实现细节；

## `latin-1`中文乱码还原方法

### shell脚本和python相结合

```bash
#!/bin/bash

# fix-latin1-mojibake-filenames.sh
# 仅修复因 "UTF-8 字节被误当 Latin-1 解码" 导致的乱码文件名

for f in *; do
    [[ ! -e "$f" ]] && continue  # 处理空目录或无匹配

    # 调用 Python 判断是否为可修复的乱码，并返回新名字（若可修复）
    newname=$(python3 -c "
import sys
s = sys.argv[1]
try:
    # Step 1: 尝试用 latin-1 编码（只有乱码字符串才能成功，因为只含 0x00-0xFF 字符）
    # raw_bytes = s.encode('utf-8')
    raw_bytes = s.encode('ANSI')
    # Step 2: 尝试用 utf-8 解码这些字节
    corrected = raw_bytes.decode('utf-8')
    print(corrected)
    # Step 3: 仅当解码成功且与原名不同，才输出新名字
    # if corrected != s:
        # print(corrected)
    # else:
        # 名字相同，无需修复（比如纯 ASCII 文件名）
        # pass
except (UnicodeEncodeError, UnicodeDecodeError):
    # UnicodeEncodeError: 原名含非 Latin-1 字符（如正常中文）→ 跳过
    # UnicodeDecodeError: 字节不是合法 UTF-8 → 不是目标乱码，跳过
    pass
" "$f")

    # 如果 newname 非空且不等于原名，且目标文件不存在，则重命名
    echo $newname
    if [[ -n "$newname" ]] && [[ "$f" != "$newname" ]]; then
        if [[ ! -e "$newname" ]]; then
            echo "Renaming: '$f' → '$newname'"
            # mv "$f" "$newname"
        else
            echo "Skip: '$f' → '$newname' (target exists)"
        fi
    fi
done
```

## Unicode全部导出

```c
// dump_cp936.c
#include <errno.h>
#include <iconv.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>

void my_uc_hook(unsigned int code, void *data) {
    printf("Converted U+%04X\n", code);
}
// 将单个 Unicode 码点（0~0xFFFF）转换为 CP936 字节序列
int unicode_to_cp936(uint32_t codepoint, unsigned char *out_buf,
                     size_t out_size) {
  // 使用 UTF-32BE 作为输入（固定 4 字节，大端）
  unsigned char in_buf[4];
  in_buf[0] = (codepoint >> 24) & 0xFF;
  in_buf[1] = (codepoint >> 16) & 0xFF;
  in_buf[2] = (codepoint >> 8) & 0xFF;
  in_buf[3] = codepoint & 0xFF;

  char *in_ptr = (char *)in_buf;
  size_t in_bytes_left = 4;
  char *out_ptr = (char *)out_buf;
  size_t out_bytes_left = out_size;
  struct iconv_hooks hooks;
  hooks.uc_hook = my_uc_hook;
  hooks.wc_hook = NULL;
  hooks.data = NULL;

  // 打开 iconv 转换器：UTF-32BE -> CP936
  iconv_t cd = iconv_open("CP936", "UTF-32BE");
  iconvctl(cd, ICONV_SET_HOOKS,&hooks);
  if (cd == (iconv_t)-1) {
    perror("iconv_open failed");
    return -1;
  }

  // 执行转换
  size_t result = iconv(cd, &in_ptr, &in_bytes_left, &out_ptr, &out_bytes_left);
  int saved_errno = errno;

  iconv_close(cd);

  if (result == (size_t)-1) {
    if (saved_errno == EILSEQ || saved_errno == EINVAL) {
      // 无法转换：无效或不支持的字符
      return 0;
    } else {
      fprintf(stderr, "iconv error at U+%04X: %s\n", codepoint,
              strerror(saved_errno));
      return -1;
    }
  }

  // 返回写入的字节数
  return (int)(out_size - out_bytes_left);
}

int main() {
  FILE *f = fopen("cp936_mapping.txt", "w");
  if (!f)
    return 1;

  for (DWORD u = 0; u <= 0xFFFF; u++) {
    if (u > 0xD800 && u <= 0xDFFF) {
      continue;
    }
    wchar_t wc = (wchar_t)u;
    char mb[3] = {0};
    int len = WideCharToMultiByte(936, 0, &wc, 1, mb, sizeof(mb), NULL, NULL);
    if (len <= 0) {
      continue;
    }
    if (len == 1 && (unsigned char)mb[0] == 0x3F && u != 0x003F) {
      continue;
    }
    if (len == 1) {
      fprintf(f, "U+%04lX -> %02X\n", u, (unsigned char)mb[0]);
    } else if (len == 2) {
      fprintf(f, "U+%04lX -> %02X %02X\n", u, (unsigned char)mb[0],
              (unsigned char)mb[1]);
    }
  }
  fclose(f);
  printf("Mapping saved to cp936_mapping.txt\n");

  FILE *fp = fopen("unicode_to_cp936.txt", "w");
  if (!fp) {
    perror("Cannot open output file");
    return 1;
  }

  unsigned char cp936_buf[10]; // CP936 最多 2 字节/字符，10 足够

  for (uint32_t cp = 0; cp <= 0xFFFF; cp++) {
    int len = unicode_to_cp936(cp, cp936_buf, sizeof(cp936_buf));
    if (len > 0) {
      // 成功转换，输出 U+XXXX -> YY ZZ ...
      fprintf(fp, "U+%04X ->", cp);
      for (int i = 0; i < len; i++) {
        fprintf(fp, " %02X", cp936_buf[i]);
      }
      fprintf(fp, "\n");
    }
    // len == 0: 无法转换，跳过
    // len < 0: 错误，已处理
  }

  fclose(fp);
  printf("Conversion completed. Output written to 'unicode_to_cp936.txt'\n");

  return 0;
}
```

## 其他工具

- nlsdump导出拆解出cp2wc.txt、 wc2cp.txt

```bash
./nlsdump.exe C_936.NLS > nlsdump.txt # 导出字符集
# 多字节到宽字符映射
sed -n '/^0x[^ ]\+ => .*/p' nlsdump.txt > cp2wc.txt
# 宽字符到多字节映射
sed -n '/^U+[^ ]\+ => .*/p' nlsdump.txt > wc2cp.txt

```
然后vim查看，去除`:g/0x003f/d` 和`U+0000`
使用`difft --color always iconv_unicode_to_cp936.txt wc2cp.txt > iconv_cp936.patch`
过滤出差异部分，

```bash
sed -n 's/.*\(.\[92\(;1\)\?m\)\(U+[0-9A-Fa-f]\+[[:space:]]*=>[[:space:]]*0x[0-9A-Fa-f]\+\)\(.\[0m\).*/\3/p' iconv_cp936.patch > add.txt
sed -n 's/.*\(.\[91;1m\)\(U+[0-9A-Fa-f]\+[[:space:]]*=>[[:space:]]*0x[0-9A-Fa-f]\+\)\(.\[0m\).*/\2/p' iconv_cp936.patch > modify.txt
```
注意上面过滤并不全面，需要手工修改一下；

- 找出相同多字节，不同源Unicode信息

```bash
ruby find_same.rb
```
相应分析文件./rst.txt 和 ./cp936Header.txt

- 找出cp936.nls独有信息

```bash
ruby ./find_add_only.rbb
```
相应分析文件./rst_win.txt,

- 生成c语言补丁代码

```bash
ruby ./cp936Header_win.rb
```
生成./c9.case.c文件，复制到lib/cp936.h中即可

- 进入`src/build-UCRT64/lib`下，重新生成lib库文件

```bash
mingw32-make STRIP=true clean
mingw32-make STRIP=true
```

- 进入`build-UCRT64`下，重新安装准备测试

```bash
mingw32-make STRIP=true
MINGW_ARCH=ucrt64 make install STRIP=true DESTDIR=$(pwd)
```
注意一定要使用make进行安装，不能使用mingw32-make

- 进入`build-UCRT64/tests`下，准备测试

```bash
bash -x ../../libiconv/tests/check-stateless ../../libiconv/tests CP936
```

### Reference
- [分析下iconv在把utf-8编码字符转换为gbk时的转换逻辑](https://deepwiki.com/search/iconvutf8gbk_dc6fec3b-0b65-4d78-ae21-4cd0846f9db9)
