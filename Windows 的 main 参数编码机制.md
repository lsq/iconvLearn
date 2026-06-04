# Windows 的 `main()` 参数编码机制

你提出的问题非常精准，触及了 **Windows 命令行参数传递机制 + Libtool 启动器设计 + 编码转换陷阱** 的核心。

我们来深入分析：

> **当顶层 `.exe`（Libtool 生成的启动器）从 Windows 内核接收到命令行参数时，如果当前控制台代码页（OEM CP）不是 UTF-8，含非 ASCII 字符的参数会被错误地转为 ANSI/OEM 编码（如 GBK），导致 Unicode 字符变成 `0x3F`（即 `'?'`）——此时信息已永久丢失。随后启动器将这个损坏的 `argv[]` 透传给 `.libs/...exe`，自然出错。**

---

## 🔍 一、问题根源：Windows 的 `main()` 参数编码机制

在 Windows 上，当你用 `int main(int argc, char* argv[])` 时：

1. Windows 内核内部始终以 **UTF-16** 存储命令行
2. 但为了兼容 C 标准，C 运行时（CRT）会在进程启动时：
   - 调用 `GetCommandLineA()`（注意是 **A** 后缀）
   - 将 UTF-16 命令行 **按当前 OEM 代码页（如 936=GBK）转为多字节字符串**
   - 然后分割成 `argv[]`
3. 如果某个 Unicode 字符 **无法用当前 OEM CP 表示**（比如 UTF-8 中的 `é` 在 GBK 中不存在），就会被替换为 `0x3F`（`'?'`）

> ✅ 所以：**`argv[]` 的内容在程序启动前就已损坏，与程序本身无关！**

---

## 🔧 二、Libtool 顶层启动器（`lt-*.c`）的生成过程

### 步骤 1：Libtool 检测平台
- 当目标平台是 `*-*-mingw*` 或 `*-*-cygwin*`，Libtool 会启用 **Windows 启动器模式**

### 步骤 2：生成 `lt-program.c` 源文件
典型内容（简化）如下：

```c
/* lt-iconv_no_i18n.c */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <windows.h>

extern char **environ;

int main (int argc, char *argv[]) {
    char *prog_name = ".libs/iconv_no_i18n.exe";
    char **new_argv;
    int i;

    // 设置 PATH，包含 .libs 目录（便于加载 DLL）
    SetEnvironmentVariableA("PATH", ".libs;" /* + original PATH */);

    // 分配新参数数组
    new_argv = (char **) malloc((argc + 1) * sizeof(char *));
    new_argv[0] = prog_name;
    for (i = 1; i < argc; i++) {
        new_argv[i] = argv[i];  // ←←← 直接透传！不做任何编码转换！
    }
    new_argv[argc] = NULL;

    // 启动真实程序
    return _spawnv(_P_WAIT, prog_name, new_argv);
}
```

### 关键点：
- **它使用 `main(int argc, char* argv[])`**
- **它直接复制 `argv[i]` 给 `_spawnv`**
- **它没有、也无法修复已经损坏的 `argv[]`**

> 🚨 所以：**如果顶层 `.exe` 启动时 `argv[]` 已含 `0x3F`，`.libs/...exe` 收到的也是 `0x3F`**

---

## 📌 三、为什么 Libtool 不用 `wmain`？

这是关键！

- Libtool 是一个 **通用构建工具**，目标是支持所有 Unix-like 平台
- 它生成的 `lt-*.c` 是 **纯标准 C**，不包含平台特定代码（如 `wmain`、`GetCommandLineW`）
- 它假设：**真实程序（`.libs/...exe`）自己会处理编码问题**

但实际上，在 Windows 上，**一旦使用 `main()`，参数就可能损坏**。

> 💡 Libtool 的设计哲学是：“启动器只负责设置环境和转发，不处理业务逻辑（包括编码）”

---

## ✅ 四、解决方案：绕过 `main()` 的编码陷阱

要真正解决这个问题，**必须让顶层启动器也使用宽字符接口**。但 Libtool 不会自动生成这样的代码。

### 方案 1：**修改 Libtool 模板（高级，不推荐）**
- 修改 Libtool 源码中的 `ltmain.in`，为 Windows 生成 `wmain` 版本启动器
- 复杂且难以维护

### 方案 2：**避免使用 Libtool 启动器（推荐）**
- 直接运行 `.libs/iconv_no_i18n.exe`
- 但需确保 `.libs` 在 `PATH` 中（或把 DLL 复制到同目录）

### 方案 3：**全局启用 UTF-8（最实用）**
- 用户开启 Windows 的 **“Beta: 使用 UTF-8”** 选项
- 此时 OEM CP = 65001，`GetCommandLineA()` 返回的就是 UTF-8
- `argv[]` 不再损坏

### 方案 4：**真实程序使用 `wmain`（根本解决）**
即使启动器传的是损坏的 `argv[]`，但如果 **真实程序不用 `argv[]`，而是自己调用 `GetCommandLineW`**，就能拿到原始 UTF-16！

#### 示例（在 `.libs/iconv_no_i18n.exe` 中）：
```c
#define UNICODE
#include <windows.h>
#include <shellapi.h>  // for CommandLineToArgvW

int main() {
    int argc;
    LPWSTR *argv_w = CommandLineToArgvW(GetCommandLineW(), &argc);
    
    // 现在 argv_w 是完整的 UTF-16 参数，无损！
    // 可安全转为 UTF-8
    
    LocalFree(argv_w);
    return 0;
}
```

> ✅ 这样即使顶层启动器传了错误的 `argv[]`，真实程序也能**绕过 CRT 的 `argv[]`，直接获取正确参数！**

---

## 🧪 验证流程

1. **关闭 UTF-8 支持**（OEM CP = 936）
2. 创建文件：`touch café.txt`
3. 运行：`iconv_no_i18n.exe café.txt`
4. 如果程序使用 `main()` + `argv[1]` → 得到 `"caf?.txt"`
5. 如果程序使用 `GetCommandLineW()` → 得到完整 `"café.txt"`

---

## ✅ 总结

| 问题                         | 原因                                     | 解决方案                                |
| ---------------------------- | ---------------------------------------- | --------------------------------------- |
| 顶层 `.exe` 收到 `0x3F` 参数 | Windows CRT 用 OEM CP 转换 UTF-16 命令行 | **真实程序改用 `GetCommandLineW()`**    |
| Libtool 启动器不修复编码     | 它是通用 C 程序，不处理平台编码细节      | 接受限制，或绕过启动器                  |
| 最佳实践                     | —                                        | **真实程序内部使用宽字符 API 获取参数** |

> 💡 **终极建议**：  
> 对于需要处理国际文件名的 Windows 程序（如 `iconv`），**不要依赖 `main()` 的 `argv[]`**，而应使用：
> ```c
> LPWSTR cmd = GetCommandLineW();
> LPWSTR *args = CommandLineToArgvW(cmd, &argc);
> ```
> 这是 Microsoft 官方推荐的 Unicode 参数处理方式。

这样，无论顶层启动器是否损坏参数，你的程序都能拿到原始、正确的 Unicode 字符串。