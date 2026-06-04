#include <windows.h>
#include <stdio.h>
#include <stdlib.h>

// 打印字节数组的十六进制
void print_hex(const char* label, const unsigned char* data, size_t len) {
    printf("%s: ", label);
    for (size_t i = 0; i < len; ++i) {
        printf("%02X ", data[i]);
    }
    printf("\n");
}

// 打印 UTF-16 小端十六进制
void print_utf16_hex(const wchar_t* wstr, int wlen) {
    printf("Interpreted as CP936 -> UTF-16 (LE): ");
    for (int i = 0; i < wlen; ++i) {
        printf("%02X %02X ", (unsigned char)(wstr[i] & 0xFF), (unsigned char)((wstr[i] >> 8) & 0xFF));
    }
    printf("\n");
}

int main() {
    // 原始 UTF-8 字节序列：对应 "安装包下载地址"
    unsigned char fake_cp936_bytes[] = {
        0xE5, 0xAE, 0x89,   // 这些其实是 UTF-8 字节，但我们假装它们是 CP936
        0xE8, 0xA3, 0x85,
        0xE5, 0x8C, 0x85,
        0xE4, 0xB8, 0x8B,
        0xE8, 0xBD, 0xBD,
        0xE5, 0x9C, 0xB0,
        0xE5, 0x9D, 0x80
    };
    int byte_len = sizeof(fake_cp936_bytes);

    // 1. 打印原始字节（即被误认为是 CP936 的字节）
    print_hex("Bytes (treated as CP936)", fake_cp936_bytes, byte_len);

    // 2. 将这些字节按 CP936 解码为宽字符（UTF-16）
    // 注意：CP936 = 936，也可以用 CP_ACP（如果系统是简体中文）
    int wlen = MultiByteToWideChar(936, MB_ERR_INVALID_CHARS, (char*)fake_cp936_bytes, byte_len, NULL, 0);
    if (wlen == 0) {
        DWORD err = GetLastError();
        if (err == ERROR_NO_UNICODE_TRANSLATION) {
            printf("Error: Some bytes are not valid in CP936.\n");
            // 可选：去掉 MB_ERR_INVALID_CHARS 以允许替换无效字符
            wlen = MultiByteToWideChar(936, 0, (char*)fake_cp936_bytes, byte_len, NULL, 0);
            if (wlen == 0) {
                fprintf(stderr, "Failed to convert even with error tolerance.\n");
                return 1;
            }
        } else {
            fprintf(stderr, "MultiByteToWideChar failed. Error: %lu\n", err);
            return 1;
        }
    }

    wchar_t* wstr = (wchar_t*)malloc(wlen * sizeof(wchar_t));
    if (!wstr) {
        fprintf(stderr, "Memory allocation failed.\n");
        return 1;
    }

    // 再次调用，实际转换
    if (MultiByteToWideChar(936, 0, (char*)fake_cp936_bytes, byte_len, wstr, wlen) == 0) {
        fprintf(stderr, "Second call to MultiByteToWideChar failed.\n");
        free(wstr);
        return 1;
    }

    // 3. 打印 UTF-16 序列（小端）
    print_utf16_hex(wstr, wlen);

    // 4. 打印“乱码”字符串（在 GBK 终端中可显示）
    // 先转回 ANSI（CP936）以便 printf 输出（因为控制台通常是 ANSI）
    int ansi_len = WideCharToMultiByte(CP_ACP, 0, wstr, wlen, NULL, 0, NULL, NULL);
    char* ansi_str = (char*)malloc(ansi_len);
    if (ansi_str && WideCharToMultiByte(CP_ACP, 0, wstr, wlen, ansi_str, ansi_len, NULL, NULL)) {
        printf("Decoded string (garbled): %s\n", ansi_str);
        print_hex("Bytes (Decoded CP936)", ansi_str, ansi_len);
    } else {
        printf("Decoded string (garbled): [failed to convert to console encoding]\n");
    }

    // 清理
    free(wstr);
    free(ansi_str);

    return 0;
}
