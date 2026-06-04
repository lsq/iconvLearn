#include <windows.h>
#include <stdio.h>
#include <stdlib.h>

// 辅助函数：打印字节数组的十六进制表示
void print_hex(const char* label, const unsigned char* data, size_t len) {
    printf("%s: ", label);
    for (size_t i = 0; i < len; ++i) {
        printf("%02X ", data[i]);
    }
    printf("\n");
}

// 辅助函数：打印宽字符（UTF-16）的十六进制表示（小端序）
void print_utf16_hex(const wchar_t* wstr, size_t wlen) {
    printf("UTF-16 (LE) hex sequence: ");
    for (size_t i = 0; i < wlen; ++i) {
        // Windows 使用小端序（LE），所以低字节在前
        printf("%02X %02X ", (unsigned char)(wstr[i] & 0xFF), (unsigned char)((wstr[i] >> 8) & 0xFF));
    }
    printf("\n");
}

int main() {
    // 原始 UTF-8 字符串（注意：C 字符串字面量默认是本地编码，但这里我们显式构造 UTF-8 字节）
    // "安装包下载地址" 的 UTF-8 编码（已知）
    unsigned char utf8_bytes[] = {
        0xE5, 0xAE, 0x89,   // 安
        0xE8, 0xA3, 0x85,   // 装
        0xE5, 0x8C, 0x85,   // 包
        0xE4, 0xB8, 0x8B,   // 下
        0xE8, 0xBD, 0xBD,   // 载
        0xE5, 0x9C, 0xB0,   // 地
        0xE5, 0x9D, 0x80    // 址
    };
    /* unsigned char utf8_bytes[] = { */
    /* } */
    int utf8_len = sizeof(utf8_bytes);

    // 1. 输出原始 UTF-8 字符串的十六进制序列
    print_hex("UTF-8 hex sequence", utf8_bytes, utf8_len);

    // 2. 将 UTF-8 转换为 UTF-16 (WideChar)
    int wlen = MultiByteToWideChar(CP_UTF8, 0, (char*)utf8_bytes, utf8_len, NULL, 0);
    if (wlen == 0) {
        fprintf(stderr, "MultiByteToWideChar (UTF-8 -> UTF-16) failed. Error: %lu\n", GetLastError());
        return 1;
    }

    wchar_t* wstr = (wchar_t*)malloc(wlen * sizeof(wchar_t));
    if (!wstr) {
        fprintf(stderr, "Memory allocation failed.\n");
        return 1;
    }

    if (MultiByteToWideChar(CP_UTF8, 0, (char*)utf8_bytes, utf8_len, wstr, wlen) == 0) {
        fprintf(stderr, "MultiByteToWideChar (UTF-8 -> UTF-16) failed on second call. Error: %lu\n", GetLastError());
        free(wstr);
        return 1;
    }

    // 3. 输出 UTF-16 编码的十六进制序列（小端序）
    print_utf16_hex(wstr, wlen);

    // 4. 将 UTF-16 转换为 CP936 (GBK)
    int cp936_len = WideCharToMultiByte(936, 0, wstr, wlen, NULL, 0, NULL, NULL);
    if (cp936_len == 0) {
        fprintf(stderr, "WideCharToMultiByte (UTF-16 -> CP936) failed. Error: %lu\n", GetLastError());
        free(wstr);
        return 1;
    }

    char* cp936_str = (char*)malloc(cp936_len);
    if (!cp936_str) {
        fprintf(stderr, "Memory allocation failed.\n");
        free(wstr);
        return 1;
    }

    if (WideCharToMultiByte(936, 0, wstr, wlen, cp936_str, cp936_len, NULL, NULL) == 0) {
        fprintf(stderr, "WideCharToMultiByte (UTF-16 -> CP936) failed on second call. Error: %lu\n", GetLastError());
        free(wstr);
        free(cp936_str);
        return 1;
    }

    // 5. 输出 CP936 字符串的十六进制序列
    print_hex("CP936 hex sequence", (unsigned char*)cp936_str, cp936_len);

    // 可选：验证转换结果（在支持 CP936 的终端中可显示中文）
    printf("CP936 string (may show as garbled if terminal doesn't support): %s\n", cp936_str);

    // 清理内存
    free(wstr);
    free(cp936_str);

    return 0;
}
