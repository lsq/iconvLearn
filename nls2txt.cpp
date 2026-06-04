#include <windows.h>
#include <stdio.h>
#include <vector>
#include <map>

#pragma warning(disable: 4996) // disable deprecation warnings for fopen etc.

struct NLS_HEADER {
    DWORD magic;        // "NLSF" (little-endian: 'F','S','L','N' -> 0x46534C4E)
    DWORD version;
    DWORD unknown1;
    DWORD codePage;
    DWORD unknown2;
    DWORD dbcsOffset;
    DWORD unicodeOffset;
    DWORD glyphOffset;
    DWORD unknown3[5];
};

int main(int argc, char* argv[]) {
    if (argc != 2) {
        printf("Usage: nls2txt <nls_file>\n");
        return 1;
    }

    FILE* f = fopen(argv[1], "rb");
    if (!f) {
        perror("fopen");
        return 1;
    }

    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    fseek(f, 0, SEEK_SET);

    std::vector<unsigned char> data(size);
    if (fread(data.data(), 1, size, f) != (size_t)size) {
        fclose(f);
        perror("fread");
        return 1;
    }
    fclose(f);

    if (size < (long)sizeof(NLS_HEADER)) {
        fprintf(stderr, "File too small\n");
        return 1;
    }

    NLS_HEADER* hdr = (NLS_HEADER*)data.data();
    if (hdr->magic != 0x46534C4E) { // "NLSF"
        fprintf(stderr, "Invalid NLS header (magic = 0x%08X)\n", hdr->magic);
        return 1;
    }

    if (hdr->dbcsOffset == 0 || hdr->dbcsOffset >= (DWORD)size) {
        fprintf(stderr, "No valid DBCS table found\n");
        return 1;
    }

    unsigned char* dbcs = &data[hdr->dbcsOffset];
    std::map<WORD, WORD> mappings; // GBK (lead<<8|trail) -> Unicode

    DWORD pos = 0;
    while (pos + 4 <= (DWORD)(size - hdr->dbcsOffset)) {
        WORD lead = *(WORD*)(dbcs + pos);
        if (lead == 0) break;
        pos += 2;
        WORD count = *(WORD*)(dbcs + pos);
        pos += 2;

        if (pos + count * 4 > (DWORD)(size - hdr->dbcsOffset)) {
            break; // avoid overflow
        }

        for (DWORD i = 0; i < count; ++i) {
            WORD trail = *(WORD*)(dbcs + pos);
            pos += 2;
            WORD uni = *(WORD*)(dbcs + pos);
            pos += 2;
            WORD gbk = (lead << 8) | trail;
            mappings[gbk] = uni;
        }
    }

    for (const auto& kv : mappings) {
        printf("0x%04X = U+%04X\n", kv.first, kv.second);
    }

    return 0;
}
