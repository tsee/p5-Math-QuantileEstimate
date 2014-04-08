#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <math.h>

#include <quant_est.h>

#include "mytap.h"

static void
test_basics()
{
  stream_t *s;
  double t;
  int i;

  s = gkstr_new(0.001, 3000+3);
  ok_m(s != NULL, "gkstr_new didn't (obviously) fail");
  ok_m(!gkstr_update(s, 1), "gkstr_update didn't (obviously) fail");
  ok_m(!gkstr_update(s, 2), "gkstr_update didn't (obviously) fail (2)");
  ok_m(!gkstr_update(s, 3), "gkstr_update didn't (obviously) fail (3)");
  for (i = 0; i < 1000; ++i) {
    gkstr_update(s, 1);
    gkstr_update(s, 2);
    gkstr_update(s, 3);
  }
  gkstream_finish(s);
  t = gkstream_query(s, 2);
  ok_m(t > 0 && t < 4, "gkstream_query returns something in the right _order_of_magnitude_");
}


int
main ()
{
  test_basics();
  ok_m(1, "alive");
  done_testing();
  return 0;
}

