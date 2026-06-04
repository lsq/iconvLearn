#include <windows.h>
#include <stdio.h>
#include <winnls.h>

int main() {
    wchar_t w = 0xE5CA;
    char mb[10] = {0};
    // 关键：不使用 WC_NO_BEST_FIT_CHARS，允许 best-fit 映射
    /* int len = WideCharToMultiByte(936, 0, &w, 1, mb, sizeof(mb), NULL, NULL); */
    int len = WideCharToMultiByte(936, WC_NO_BEST_FIT_CHARS, &w, 1, mb, sizeof(mb), NULL, NULL);
    if (len == 0) {
        printf("Error: %lu\n", GetLastError());
    } else {
        printf("U+%04X -> %02X %02X\n", w, (unsigned char)mb[0], (unsigned char)mb[1]);
    }
    return 0;
}
