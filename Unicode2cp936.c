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
      fprintf(fp, "U+%04x => 0x", cp);
      if (len == 1) {
        fprintf(fp, "00");
      }
      for (int i = 0; i < len; i++) {
        fprintf(fp, "%02x", cp936_buf[i]);
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
