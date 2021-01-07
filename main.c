#include <stdio.h>
#include <wchar.h>
#include <string.h>
#include <locale.h>
#include <stdint.h>
#include <stdlib.h>
#include <inttypes.h>

#include "utf-8.h"

#define LOW     0x010000
#define HIGH    0x10FFFF
#define N       ((HIGH) - (LOW) + 1)

void print_help(const char *name)
{
    fprintf(stdout, "USAGE\n");
    fprintf(stdout, "\t%s [LOW_HEX] [HIGH_HEX] [OUT]\n", name);
    fprintf(stdout, "EXAMPLE\n");
    fprintf(stdout, "\t%s 0xC0 0xD6 sample.txt\n", name);
}

int main(int argc, char *argv[])
{
    print_help(argv[0]);
    if (argc != 4)
    {
        exit(EXIT_FAILURE);
    }

    setlocale(LC_ALL, "");

    unsigned long low = strtoul(argv[1], NULL, 16);
    unsigned long high = strtoul(argv[2], NULL, 16);
    size_t n = high - low + 1;
    enc_unit_t *enc_units = (enc_unit_t*) malloc(N * sizeof (enc_unit_t));

    for (uint32_t cp = low, i = 0; cp <= high; ++cp, ++i)
        enc_units[i] = encode(cp);

    write(argv[3], enc_units, n);
    free(enc_units);

    /*
    size_t n_elems = 0;
    uint32_t *code_points = read("sample3.txt", &n_elems);
    wprintf(L"# code points = %lu\n", n_elems);
    for (size_t i = 0; i < n_elems; ++i)
        wprintf(L"%lc", code_points[i]);
    for (size_t i = 0; i < n_elems; ++i)
        wprintf(L"0x%0x ", code_points[i]);
    wprintf(L"\n");
    if (code_points) free(code_points);
    */

    return 0;
}
