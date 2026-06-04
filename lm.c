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

// 打印 UTF-16（小端）十六进制
void print_utf16_hex(const wchar_t* wstr, int wlen) {
    printf("UTF-16 (LE): ");
    for (int i = 0; i < wlen; ++i) {
        printf("%02X %02X ", (unsigned char)(wstr[i] & 0xFF), (unsigned char)((wstr[i] >> 8) & 0xFF));
    }
    printf("\n");
}

int main() {
    // CP936 编码的字符串字面量（必须以实际 CP936 字节形式提供）
    // "texstudio瀹夎鍖呬笅杞藉湴鍧€" 在 CP936 下的实际字节序列（已通过工具确认）
    unsigned char cp936_bytes[] = {
        // "texstudio"
        't', 'e', 'x', 's', 't', 'u', 'd', 'i', 'o',
        // "瀹" = B7 BB, "夎" = B0 CE, "" = A6 F6, "鍖" = BA D3,
        // "ì" = EC, "…" = 85, "杞" = C8 CF, "藉" = D4 D8,
        // "湴" = B5 D8, "鍧" = BA D7, "€" = 80
        0xB7, 0xBB, 0xB0, 0xCE, 0xA6, 0xF6, 0xBA, 0xD3,
        0xEC, 0x85, 0xC8, 0xCF, 0xD4, 0xD8, 0xB5, 0xD8,
        0xBA, 0xD7, 0x80
    };
    int cp936_len = sizeof(cp936_bytes);

    // 1. 输出原始 CP936 字节序列
    print_hex("CP936 hex sequence", cp936_bytes, cp936_len);

    // 2. 将 CP936 转换为 UTF-16 (WideChar)
    int wlen = MultiByteToWideChar(936, MB_ERR_INVALID_CHARS, (char*)cp936_bytes, cp936_len, NULL, 0);
    if (wlen == 0) {
        DWORD err = GetLastError();
        if (err == ERROR_NO_UNICODE_TRANSLATION) {
            // 如果有无效字符，尝试宽容模式（替换为 ?）
            printf("Warning: Invalid CP936 sequence detected. Using tolerant mode.\n");
            wlen = MultiByteToWideChar(936, 0, (char*)cp936_bytes, cp936_len, NULL, 0);
        }
        if (wlen == 0) {
            fprintf(stderr, "Failed to convert CP936 to UTF-16. Error: %lu\n", GetLastError());
            return 1;
        }
    }

    wchar_t* wstr = (wchar_t*)malloc(wlen * sizeof(wchar_t));
    if (!wstr) {
        fprintf(stderr, "Memory allocation failed.\n");
        return 1;
    }

    if (MultiByteToWideChar(936, 0, (char*)cp936_bytes, cp936_len, wstr, wlen) == 0) {
        fprintf(stderr, "Second call to MultiByteToWideChar failed.\n");
        free(wstr);
        return 1;
    }

    // 3. 输出 UTF-16 序列
    print_utf16_hex(wstr, wlen);

    // 4. 将 UTF-16 转换为 UTF-8
    int utf8_len = WideCharToMultiByte(CP_UTF8, 0, wstr, wlen, NULL, 0, NULL, NULL);
    if (utf8_len == 0) {
        fprintf(stderr, "Failed to convert UTF-16 to UTF-8. Error: %lu\n", GetLastError());
        free(wstr);
        return 1;
    }

    char* utf8_str = (char*)malloc(utf8_len);
    if (!utf8_str) {
        fprintf(stderr, "Memory allocation failed.\n");
        free(wstr);
        return 1;
    }

    if (WideCharToMultiByte(CP_UTF8, 0, wstr, wlen, utf8_str, utf8_len, NULL, NULL) == 0) {
        fprintf(stderr, "Second call to WideCharToMultiByte failed.\n");
        free(wstr);
        free(utf8_str);
        return 1;
    }

    // 5. 输出 UTF-8 字节序列
    print_hex("UTF-8 hex sequence", (unsigned char*)utf8_str, utf8_len);

    // 6. 尝试打印可读字符串（需终端支持 UTF-8）
    // 注意：Windows 控制台默认不启用 UTF-8，可能显示乱码
    printf("Decoded UTF-8 string: %s\n", utf8_str);

    // 清理内存
    free(wstr);
    free(utf8_str);

    return 0;
}
