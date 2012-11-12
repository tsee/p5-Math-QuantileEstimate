#define PERL_NO_GET_CONTEXT
#include "EXTERN.h"
#include "perl.h"
#include "XSUB.h"

#include "ppport.h"

#include "quant_est.h"

MODULE = Math::QuantileEstimate    PACKAGE = Math::QuantileEstimate

REQUIRE: 2.2201

void
DESTROY(self)
    quant_est* self
  CODE:

