/* Host utility for validating the native C DTRZ metadata parser. */
#include "re_dtrz_vfs.h"
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char **argv) {
    FILE *fp;
    long length;
    uint8_t *bytes;
    re_dtrz_archive archive;
    uint32_t i;
    re_dtrz_status status;
    if (argc != 2) {
        fprintf(stderr, "usage: %s /path/to/main.dz\n", argv[0]);
        return 2;
    }
    fp = fopen(argv[1], "rb");
    if (!fp) { perror("fopen"); return 2; }
    if (fseek(fp, 0, SEEK_END) != 0 || (length = ftell(fp)) < 0 ||
        fseek(fp, 0, SEEK_SET) != 0) { fclose(fp); return 2; }
    bytes = (uint8_t *)malloc((size_t)length);
    if (!bytes) { fclose(fp); return 2; }
    if (fread(bytes, 1, (size_t)length, fp) != (size_t)length) {
        free(bytes); fclose(fp); return 2;
    }
    fclose(fp);
    status = re_dtrz_archive_parse(&archive, bytes, (size_t)length);
    if (status != RE_DTRZ_OK) {
        fprintf(stderr, "parse failed: %s (%d)\n", re_dtrz_status_string(status), status);
        free(bytes); return 1;
    }
    printf("DTRZ files=%u folders_including_root=%u payload_offset=%llu archive_bytes=%ld\n",
        archive.file_count, archive.folder_count,
        (unsigned long long)archive.payload_offset, length);
    for (i = 0; i < archive.file_count; ++i) {
        const re_dtrz_entry *e = &archive.entries[i];
        printf("%02u %s offset=%llu packed=%llu decoded=%u method=0x%08x\n",
            i, e->path, (unsigned long long)e->offset,
            (unsigned long long)e->packed_size, e->decoded_size_a, e->method);
    }
    re_dtrz_archive_destroy(&archive);
    free(bytes);
    return 0;
}
