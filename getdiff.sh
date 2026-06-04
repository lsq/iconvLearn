# sed -n 's/.*\(\[92;1m[[:space:]]*\)\([0-9]\+\)[[:space:]]*\(.\[0m\)\(.\[92;1m\)\(U+[0-9A-Fa-f]\+[[:space:]]*=>[[:space:]]*0x[0-9A-Fa-f]\+\)\(.\[0m\).*/\2 \5/p' iconv_cp936.patch > add.txt
# sed -n 's/.*\(\[91;1m[[:space:]]*\)\([0-9]\+\)[[:space:]]*\(.\[0m\)\(.\[91;1m\)\(U+[0-9A-Fa-f]\+[[:space:]]*=>[[:space:]]*0x[0-9A-Fa-f]\+\)\(.\[0m\).*/\2 \5/p' iconv_cp936.patch > modify.txt
sed -n 's/.*\(.\[92\(;1\)\?m\)\(U+[0-9A-Fa-f]\+[[:space:]]*=>[[:space:]]*0x[0-9A-Fa-f]\+\)\(.\[0m\).*/\3/p' iconv_cp936.patch > add.txt
sed -n 's/.*\(.\[91;1m\)\(U+[0-9A-Fa-f]\+[[:space:]]*=>[[:space:]]*0x[0-9A-Fa-f]\+\)\(.\[0m\).*/\2/p' iconv_cp936.patch > modify.txt
