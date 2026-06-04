#include <windows.h>
#include <stdio.h>

int main() {
    // 设置控制台输出代码页为 Latin-1 (28591)
    SetConsoleOutputCP(28591);

    // 输出 Latin-1 字节序列（需确保字符串字面量是 Latin-1 编码）
    printf("�\x8a\x82�\x82�\n");  // é 的 Latin-1 编码是 0xE9
    /* printf("\n");  // é 的 Latin-1 编码是 0xE9 */
    return 0;
}
