// test_cp57002.c
#include <windows.h>
#include <stdio.h>

int main() {
    printf("Converting with CP57002...\n");
    wchar_t w[] = L"क"; // 印地文字符
    char mb[10] = {0};
    int len = WideCharToMultiByte(57002, 0, w, -1, mb, sizeof(mb), NULL, NULL);
    if (len) {
        printf("Success: %d bytes\n", len);
    } else {
        printf("Failed! Error: %lu\n", GetLastError());
    }

    // 👇 关键：防止退出！
    printf("Press Enter to exit...");
    getchar(); // 等待用户输入
    return 0;
}
