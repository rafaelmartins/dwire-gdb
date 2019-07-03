/*
 * blogc: A blog compiler.
 * Copyright (C) 2014-2019 Rafael G. Martins <rafael@rafaelmartins.eng.br>
 *
 * This program can be distributed under the terms of the BSD License.
 * See the file LICENSE.
 */

#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>
#include <stdbool.h>
#include <stdlib.h>
#include "../src/utils.h"

#define DG_STRING_CHUNK_SIZE 128


static void
test_strdup(void **state)
{
    char *str = dg_strdup("bola");
    assert_string_equal(str, "bola");
    free(str);
    str = dg_strdup(NULL);
    assert_null(str);
}


static void
test_strndup(void **state)
{
    char *str = dg_strndup("bolaguda", 4);
    assert_string_equal(str, "bola");
    free(str);
    str = dg_strndup("bolaguda", 30);
    assert_string_equal(str, "bolaguda");
    free(str);
    str = dg_strndup("bolaguda", 8);
    assert_string_equal(str, "bolaguda");
    free(str);
    str = dg_strdup(NULL);
    assert_null(str);
}


static void
test_strdup_printf(void **state)
{
    char *str = dg_strdup_printf("bola");
    assert_string_equal(str, "bola");
    free(str);
    str = dg_strdup_printf("bola, %s", "guda");
    assert_string_equal(str, "bola, guda");
    free(str);
}


static void
test_string_new(void **state)
{
    dg_string_t *str = dg_string_new();
    assert_non_null(str);
    assert_string_equal(str->str, "");
    assert_int_equal(str->len, 0);
    assert_int_equal(str->allocated_len, DG_STRING_CHUNK_SIZE);
    assert_null(dg_string_free(str, true));
}


static void
test_string_free(void **state)
{
    dg_string_t *str = dg_string_new();
    free(str->str);
    str->str = dg_strdup("bola");
    str->len = 4;
    str->allocated_len = DG_STRING_CHUNK_SIZE;
    char *tmp = dg_string_free(str, false);
    assert_string_equal(tmp, "bola");
    free(tmp);
    assert_null(dg_string_free(NULL, false));
}


static void
test_string_append_len(void **state)
{
    dg_string_t *str = dg_string_new();
    str = dg_string_append_len(str, "guda", 4);
    assert_non_null(str);
    assert_string_equal(str->str, "guda");
    assert_int_equal(str->len, 4);
    assert_int_equal(str->allocated_len, DG_STRING_CHUNK_SIZE);
    assert_null(dg_string_free(str, true));
    str = dg_string_new();
    str = dg_string_append_len(str, "guda", 4);
    str = dg_string_append_len(str, "bola", 4);
    assert_non_null(str);
    assert_string_equal(str->str, "gudabola");
    assert_int_equal(str->len, 8);
    assert_int_equal(str->allocated_len, DG_STRING_CHUNK_SIZE);
    assert_null(dg_string_free(str, true));
    str = dg_string_new();
    str = dg_string_append_len(str, "guda", 3);
    str = dg_string_append_len(str, "bola", 4);
    assert_non_null(str);
    assert_string_equal(str->str, "gudbola");
    assert_int_equal(str->len, 7);
    assert_int_equal(str->allocated_len, DG_STRING_CHUNK_SIZE);
    assert_null(dg_string_free(str, true));
    str = dg_string_new();
    str = dg_string_append_len(str, "guda", 4);
    str = dg_string_append_len(str,
        "cwlwmwxxmvjnwtidmjehzdeexbxjnjowruxjrqpgpfhmvwgqeacdjissntmbtsjidzkcw"
        "nnqhxhneolbwqlctcxmrsutolrjikpavxombpfpjyaqltgvzrjidotalcuwrwxtaxjiwa"
        "xfhfyzymtffusoqywaruxpybwggukltspqqmghzpqstvcvlqbkhquihzndnrvkaqvevaz"
        "dxrewtgapkompnviiyielanoyowgqhssntyvcvqqtfjmkphywbkvzfyttaalttywhqcec"
        "hgrwzaglzogwjvqncjzodaqsblcbpcdpxmrtctzginvtkckhqvdplgjvbzrnarcxjrsbc"
        "sbfvpylgjznsuhxcxoqbpxowmsrgwimxjgyzwwmryqvstwzkglgeezelvpvkwefqdatnd"
        "dxntikgoqlidfnmdhxzevqzlzubvyleeksdirmmttqthhkvfjggznpmarcamacpvwsrnr"
        "ftzfeyasjpxoevyptpdnqokswiondusnuymqwaryrmdgscbnuilxtypuynckancsfnwtg"
        "okxhegoifakimxbbafkeannglvsxprqzfekdinssqymtfexf", 600);
    str = dg_string_append_len(str, NULL, 0);
    str = dg_string_append_len(str,
        "cwlwmwxxmvjnwtidmjehzdeexbxjnjowruxjrqpgpfhmvwgqeacdjissntmbtsjidzkcw"
        "nnqhxhneolbwqlctcxmrsutolrjikpavxombpfpjyaqltgvzrjidotalcuwrwxtaxjiwa"
        "xfhfyzymtffusoqywaruxpybwggukltspqqmghzpqstvcvlqbkhquihzndnrvkaqvevaz"
        "dxrewtgapkompnviiyielanoyowgqhssntyvcvqqtfjmkphywbkvzfyttaalttywhqcec"
        "hgrwzaglzogwjvqncjzodaqsblcbpcdpxmrtctzginvtkckhqvdplgjvbzrnarcxjrsbc"
        "sbfvpylgjznsuhxcxoqbpxowmsrgwimxjgyzwwmryqvstwzkglgeezelvpvkwefqdatnd"
        "dxntikgoqlidfnmdhxzevqzlzubvyleeksdirmmttqthhkvfjggznpmarcamacpvwsrnr"
        "ftzfeyasjpxoevyptpdnqokswiondusnuymqwaryrmdgscbnuilxtypuynckancsfnwtg"
        "okxhegoifakimxbbafkeannglvsxprqzfekdinssqymtfexf", 600);
    assert_non_null(str);
    assert_string_equal(str->str,
        "gudacwlwmwxxmvjnwtidmjehzdeexbxjnjowruxjrqpgpfhmvwgqeacdjissntmbtsjid"
        "zkcwnnqhxhneolbwqlctcxmrsutolrjikpavxombpfpjyaqltgvzrjidotalcuwrwxtax"
        "jiwaxfhfyzymtffusoqywaruxpybwggukltspqqmghzpqstvcvlqbkhquihzndnrvkaqv"
        "evazdxrewtgapkompnviiyielanoyowgqhssntyvcvqqtfjmkphywbkvzfyttaalttywh"
        "qcechgrwzaglzogwjvqncjzodaqsblcbpcdpxmrtctzginvtkckhqvdplgjvbzrnarcxj"
        "rsbcsbfvpylgjznsuhxcxoqbpxowmsrgwimxjgyzwwmryqvstwzkglgeezelvpvkwefqd"
        "atnddxntikgoqlidfnmdhxzevqzlzubvyleeksdirmmttqthhkvfjggznpmarcamacpvw"
        "srnrftzfeyasjpxoevyptpdnqokswiondusnuymqwaryrmdgscbnuilxtypuynckancsf"
        "nwtgokxhegoifakimxbbafkeannglvsxprqzfekdinssqymtfexfcwlwmwxxmvjnwtidm"
        "jehzdeexbxjnjowruxjrqpgpfhmvwgqeacdjissntmbtsjidzkcwnnqhxhneolbwqlctc"
        "xmrsutolrjikpavxombpfpjyaqltgvzrjidotalcuwrwxtaxjiwaxfhfyzymtffusoqyw"
        "aruxpybwggukltspqqmghzpqstvcvlqbkhquihzndnrvkaqvevazdxrewtgapkompnvii"
        "yielanoyowgqhssntyvcvqqtfjmkphywbkvzfyttaalttywhqcechgrwzaglzogwjvqnc"
        "jzodaqsblcbpcdpxmrtctzginvtkckhqvdplgjvbzrnarcxjrsbcsbfvpylgjznsuhxcx"
        "oqbpxowmsrgwimxjgyzwwmryqvstwzkglgeezelvpvkwefqdatnddxntikgoqlidfnmdh"
        "xzevqzlzubvyleeksdirmmttqthhkvfjggznpmarcamacpvwsrnrftzfeyasjpxoevypt"
        "pdnqokswiondusnuymqwaryrmdgscbnuilxtypuynckancsfnwtgokxhegoifakimxbba"
        "fkeannglvsxprqzfekdinssqymtfexf");
    assert_int_equal(str->len, 1204);
    assert_int_equal(str->allocated_len, DG_STRING_CHUNK_SIZE * 10);
    assert_null(dg_string_free(str, true));
    str = dg_string_new();
    str = dg_string_append_len(str, NULL, 0);
    assert_non_null(str);
    assert_string_equal(str->str, "");
    assert_int_equal(str->len, 0);
    assert_int_equal(str->allocated_len, DG_STRING_CHUNK_SIZE);
    assert_null(dg_string_free(str, true));
    assert_null(dg_string_append_len(NULL, "foo", 3));
}


static void
test_string_append(void **state)
{
    dg_string_t *str = dg_string_new();
    str = dg_string_append(str, "guda");
    assert_non_null(str);
    assert_string_equal(str->str, "guda");
    assert_int_equal(str->len, 4);
    assert_int_equal(str->allocated_len, DG_STRING_CHUNK_SIZE);
    assert_null(dg_string_free(str, true));
    str = dg_string_new();
    str = dg_string_append(str, "guda");
    str = dg_string_append(str, "bola");
    assert_non_null(str);
    assert_string_equal(str->str, "gudabola");
    assert_int_equal(str->len, 8);
    assert_int_equal(str->allocated_len, DG_STRING_CHUNK_SIZE);
    assert_null(dg_string_free(str, true));
    str = dg_string_new();
    str = dg_string_append(str, "guda");
    str = dg_string_append(str,
        "cwlwmwxxmvjnwtidmjehzdeexbxjnjowruxjrqpgpfhmvwgqeacdjissntmbtsjidzkcw"
        "nnqhxhneolbwqlctcxmrsutolrjikpavxombpfpjyaqltgvzrjidotalcuwrwxtaxjiwa"
        "xfhfyzymtffusoqywaruxpybwggukltspqqmghzpqstvcvlqbkhquihzndnrvkaqvevaz"
        "dxrewtgapkompnviiyielanoyowgqhssntyvcvqqtfjmkphywbkvzfyttaalttywhqcec"
        "hgrwzaglzogwjvqncjzodaqsblcbpcdpxmrtctzginvtkckhqvdplgjvbzrnarcxjrsbc"
        "sbfvpylgjznsuhxcxoqbpxowmsrgwimxjgyzwwmryqvstwzkglgeezelvpvkwefqdatnd"
        "dxntikgoqlidfnmdhxzevqzlzubvyleeksdirmmttqthhkvfjggznpmarcamacpvwsrnr"
        "ftzfeyasjpxoevyptpdnqokswiondusnuymqwaryrmdgscbnuilxtypuynckancsfnwtg"
        "okxhegoifakimxbbafkeannglvsxprqzfekdinssqymtfexf");
    str = dg_string_append(str, NULL);
    str = dg_string_append(str,
        "cwlwmwxxmvjnwtidmjehzdeexbxjnjowruxjrqpgpfhmvwgqeacdjissntmbtsjidzkcw"
        "nnqhxhneolbwqlctcxmrsutolrjikpavxombpfpjyaqltgvzrjidotalcuwrwxtaxjiwa"
        "xfhfyzymtffusoqywaruxpybwggukltspqqmghzpqstvcvlqbkhquihzndnrvkaqvevaz"
        "dxrewtgapkompnviiyielanoyowgqhssntyvcvqqtfjmkphywbkvzfyttaalttywhqcec"
        "hgrwzaglzogwjvqncjzodaqsblcbpcdpxmrtctzginvtkckhqvdplgjvbzrnarcxjrsbc"
        "sbfvpylgjznsuhxcxoqbpxowmsrgwimxjgyzwwmryqvstwzkglgeezelvpvkwefqdatnd"
        "dxntikgoqlidfnmdhxzevqzlzubvyleeksdirmmttqthhkvfjggznpmarcamacpvwsrnr"
        "ftzfeyasjpxoevyptpdnqokswiondusnuymqwaryrmdgscbnuilxtypuynckancsfnwtg"
        "okxhegoifakimxbbafkeannglvsxprqzfekdinssqymtfexf");
    assert_non_null(str);
    assert_string_equal(str->str,
        "gudacwlwmwxxmvjnwtidmjehzdeexbxjnjowruxjrqpgpfhmvwgqeacdjissntmbtsjid"
        "zkcwnnqhxhneolbwqlctcxmrsutolrjikpavxombpfpjyaqltgvzrjidotalcuwrwxtax"
        "jiwaxfhfyzymtffusoqywaruxpybwggukltspqqmghzpqstvcvlqbkhquihzndnrvkaqv"
        "evazdxrewtgapkompnviiyielanoyowgqhssntyvcvqqtfjmkphywbkvzfyttaalttywh"
        "qcechgrwzaglzogwjvqncjzodaqsblcbpcdpxmrtctzginvtkckhqvdplgjvbzrnarcxj"
        "rsbcsbfvpylgjznsuhxcxoqbpxowmsrgwimxjgyzwwmryqvstwzkglgeezelvpvkwefqd"
        "atnddxntikgoqlidfnmdhxzevqzlzubvyleeksdirmmttqthhkvfjggznpmarcamacpvw"
        "srnrftzfeyasjpxoevyptpdnqokswiondusnuymqwaryrmdgscbnuilxtypuynckancsf"
        "nwtgokxhegoifakimxbbafkeannglvsxprqzfekdinssqymtfexfcwlwmwxxmvjnwtidm"
        "jehzdeexbxjnjowruxjrqpgpfhmvwgqeacdjissntmbtsjidzkcwnnqhxhneolbwqlctc"
        "xmrsutolrjikpavxombpfpjyaqltgvzrjidotalcuwrwxtaxjiwaxfhfyzymtffusoqyw"
        "aruxpybwggukltspqqmghzpqstvcvlqbkhquihzndnrvkaqvevazdxrewtgapkompnvii"
        "yielanoyowgqhssntyvcvqqtfjmkphywbkvzfyttaalttywhqcechgrwzaglzogwjvqnc"
        "jzodaqsblcbpcdpxmrtctzginvtkckhqvdplgjvbzrnarcxjrsbcsbfvpylgjznsuhxcx"
        "oqbpxowmsrgwimxjgyzwwmryqvstwzkglgeezelvpvkwefqdatnddxntikgoqlidfnmdh"
        "xzevqzlzubvyleeksdirmmttqthhkvfjggznpmarcamacpvwsrnrftzfeyasjpxoevypt"
        "pdnqokswiondusnuymqwaryrmdgscbnuilxtypuynckancsfnwtgokxhegoifakimxbba"
        "fkeannglvsxprqzfekdinssqymtfexf");
    assert_int_equal(str->len, 1204);
    assert_int_equal(str->allocated_len, DG_STRING_CHUNK_SIZE * 10);
    assert_null(dg_string_free(str, true));
    str = dg_string_new();
    str = dg_string_append(str, NULL);
    assert_non_null(str);
    assert_string_equal(str->str, "");
    assert_int_equal(str->len, 0);
    assert_int_equal(str->allocated_len, DG_STRING_CHUNK_SIZE);
    assert_null(dg_string_free(str, true));
    assert_null(dg_string_append(NULL, "asd"));
    assert_null(dg_string_append(NULL, NULL));
}


static void
test_string_append_c(void **state)
{
    dg_string_t *str = dg_string_new();
    str = dg_string_append_len(str, "guda", 4);
    for (int i = 0; i < 600; i++)
        str = dg_string_append_c(str, 'c');
    assert_non_null(str);
    assert_string_equal(str->str,
        "gudaccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccc"
        "ccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccc"
        "ccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccc"
        "ccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccc"
        "ccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccc"
        "ccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccc"
        "ccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccc"
        "ccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccc"
        "cccccccccccccccccccccccccccccccccccccccccccccccccccc");
    assert_int_equal(str->len, 604);
    assert_int_equal(str->allocated_len, DG_STRING_CHUNK_SIZE * 5);
    assert_null(dg_string_free(str, true));
    assert_null(dg_string_append_c(NULL, 0));
}


static void
test_string_append_printf(void **state)
{
    dg_string_t *str = dg_string_new();
    str = dg_string_append_printf(str, "guda: %s %d", "bola", 1);
    assert_non_null(str);
    assert_string_equal(str->str, "guda: bola 1");
    assert_int_equal(str->len, 12);
    assert_int_equal(str->allocated_len, DG_STRING_CHUNK_SIZE);
    assert_null(dg_string_free(str, true));
    assert_null(dg_string_append_printf(NULL, "asd"));
}


int
main(void)
{
    const UnitTest tests[] = {

        // strfuncs
        unit_test(test_strdup),
        unit_test(test_strndup),
        unit_test(test_strdup_printf),

        // string
        unit_test(test_string_new),
        unit_test(test_string_free),
        unit_test(test_string_append_len),
        unit_test(test_string_append),
        unit_test(test_string_append_c),
        unit_test(test_string_append_printf),
    };
    return run_tests(tests);
}
