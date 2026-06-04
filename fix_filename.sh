#!/bin/bash

# fix-latin1-mojibake-filenames.sh
# 仅修复因 "UTF-8 字节被误当 Latin-1 解码" 导致的乱码文件名

for f in *; do
    [[ ! -e "$f" ]] && continue  # 处理空目录或无匹配

    # 调用 Python 判断是否为可修复的乱码，并返回新名字（若可修复）
    newname=$(python3 -c "
import sys
s = sys.argv[1]
try:
    # Step 1: 尝试用 latin-1 编码（只有乱码字符串才能成功，因为只含 0x00-0xFF 字符）
    # raw_bytes = s.encode('utf-8')
    raw_bytes = s.encode('ANSI')
    # Step 2: 尝试用 utf-8 解码这些字节
    corrected = raw_bytes.decode('utf-8')
    print(corrected)
    # Step 3: 仅当解码成功且与原名不同，才输出新名字
    # if corrected != s:
        # print(corrected)
    # else:
        # 名字相同，无需修复（比如纯 ASCII 文件名）
        # pass
except (UnicodeEncodeError, UnicodeDecodeError):
    # UnicodeEncodeError: 原名含非 Latin-1 字符（如正常中文）→ 跳过
    # UnicodeDecodeError: 字节不是合法 UTF-8 → 不是目标乱码，跳过
    pass
" "$f")

    # 如果 newname 非空且不等于原名，且目标文件不存在，则重命名
    echo $newname
    if [[ -n "$newname" ]] && [[ "$f" != "$newname" ]]; then
        if [[ ! -e "$newname" ]]; then
            echo "Renaming: '$f' → '$newname'"
            # mv "$f" "$newname"
        else
            echo "Skip: '$f' → '$newname' (target exists)"
        fi
    fi
done
