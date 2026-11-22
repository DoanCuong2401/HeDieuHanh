#include "kernel/types.h"
#include "user/user.h"
#include "corpus.h"

int
main(void)
{
  struct corpus c;
  struct fuzz_case item;
  int r;

  corpus_init(&c);

  printf("=== CORPUS TEST ===\n");

  r = corpus_add(&c, 100, 0, 3);
  printf("TC1: add result = %d (expected 1)\n", r);

  r = corpus_add(&c, 200, 1, 0);
  printf("TC2: add result = %d (expected 0)\n", r);

  r = corpus_add(&c, 300, 2, 2);
  printf("TC3: add result = %d (expected 1)\n", r);

  r = corpus_add(&c, 100, 0, 5);
  printf("TC4 duplicate: add result = %d (expected 0)\n", r);

  printf("Corpus size = %d (expected 2)\n", (int)c.count);

  r = corpus_get(&c, 0, &item);
  if (r == 0)
    printf("Item 0: seed=%d type=%d\n",
           (int)item.seed, (int)item.type);

  r = corpus_get(&c, 5, &item);
  printf("Invalid index result = %d (expected -1)\n", r);

  if (c.count == 2 &&
      corpus_get(&c, 0, &item) == 0 &&
      item.seed == 100 && item.type == 0 &&
      corpus_get(&c, 1, &item) == 0 &&
      item.seed == 300 && item.type == 2)
    printf("CORPUS TEST: PASS\n");
  else
    printf("CORPUS TEST: FAIL\n");

  exit(0);
}
