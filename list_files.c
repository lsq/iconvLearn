#include <windows.h>
#include <stdio.h>
#include <wchar.h>
#define _UNICODE
#define UNICODE
#include <io.h>
#include <fcntl.h>

int main() {
    // 关键：设置 stdout 为 UTF-16 文本模式
    /* _setmode(_fileno(stdout), _O_U16TEXT); */
    WIN32_FIND_DATAW findData;
    HANDLE hFind;

    // 使用通配符查找当前目录下所有文件
    hFind = FindFirstFileW(L".\\*", &findData);
    if (hFind == INVALID_HANDLE_VALUE) {
        wprintf(L"无法打开当前目录。\n");
        return 1;
    }

    do {
        // 跳过 "." 和 ".."
        if (wcscmp(findData.cFileName, L".") == 0 || wcscmp(findData.cFileName, L"..") == 0) {
            continue;
        }

        // 只处理文件（跳过目录）
        if (findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
            continue;
        }

        // 打印文件名（Unicode）
        wprintf(L"文件名: %ls\n", findData.cFileName);

        // 打印 UTF-16LE 的十六进制编码（每个 wchar_t 是 2 字节，小端）
        wprintf(L"UTF-16LE 编码: ");
        const wchar_t* p = findData.cFileName;
        while (*p != L'\0') {
            // 每个 wchar_t 拆成两个字节（小端：低字节在前）
            unsigned char low = (unsigned char)(*p & 0xFF);
            unsigned char high = (unsigned char)((*p >> 8) & 0xFF);
            wprintf(L"%02X%02X ", low, high);
            p++;
        }
        wprintf(L"\n");
        // ===== 2. 转换为 ANSI (CP_ACP) 并打印十六进制 =====
        int ansiLen = WideCharToMultiByte(CP_ACP, 0, findData.cFileName, -1, NULL, 0, NULL, NULL);
        if (ansiLen == 0) {
            wprintf(L"ANSI编码: [转换失败]\n\n");
            continue;
        }

        char* ansiBuf = (char*)malloc(ansiLen);
        if (!ansiBuf) {
            wprintf(L"ANSI编码: [内存不足]\n\n");
            continue;
        }

        if (WideCharToMultiByte(CP_ACP, 0, findData.cFileName, -1, ansiBuf, ansiLen, NULL, NULL) == 0) {
            free(ansiBuf);
            wprintf(L"ANSI编码: [转换失败]\n\n");
            continue;
        }

        // 注意：ansiLen 包含结尾的 '\0'，我们只打印到 strlen
        printf("filename (cp936): %s\n", ansiBuf);
        size_t actualLen = strlen(ansiBuf);
        wprintf(L"ANSI (%u): ", GetACP()); // 打印当前 ANSI 代码页编号，如 936=GBK
        for (size_t i = 0; i < actualLen; ++i) {
            wprintf(L"%02X ", (unsigned char)ansiBuf[i]);
        }

        free(ansiBuf);
        wprintf(L"\n\n");

    } while (FindNextFileW(hFind, &findData));

    FindClose(hFind);
    return 0;
}
