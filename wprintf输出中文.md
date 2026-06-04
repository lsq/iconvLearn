wprintf输出中文

标签： CRT, wprintf, 中文

标题: wprintf输出中文
作者: Demon
链接: https://demon.tw/programming/wprintf-unicode.html
版权: 本博客的所有文章，都遵守“署名-非商业性使用-相同方式共享 2.5 中国大陆”协议条款。

虽然一直知道有wprintf这个函数，但是一直很少用到，今天才发现wprintf居然不能直接输出中文，坑爹啊，w不是宽字符么？

要想输出中文的话还要在调用wprintf之前先调用setlocale设置区域：
```c
    #include <stdio.h>
    #include <locale.h>

    int main()
    {
        wchar_t s[] = L"中文";
        setlocale(LC_CTYPE, "");
        wprintf(L"%s\n", s);
        return 0;
    }
```
用OllyDbg简单跟了一下msvcrt.dll中的wprintf，发现居然会根据当前区域设置用WideCharToMultiByte把wchar_t转成char再WriteFile输出，直接WriteConsoleW不更省事？

以上内容仅限于微软的wprintf实现，其他平台和编译器未做测试。
