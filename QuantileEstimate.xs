#define PERL_NO_GET_CONTEXT
#include "EXTERN.h"
#include "perl.h"
#include "XSUB.h"

#include "ppport.h"

#include "quant_est.h"

MODULE = Math::QuantileEstimate    PACKAGE = Math::QuantileEstimate

REQUIRE: 2.2201

quant_est_t *
quant_est_t::new()
  CODE:
    /* TODO: call quant_est_t constructor instead... */
    RETVAL = (quant_est_t *)malloc(sizeof(quant_est_t));
  OUTPUT: RETVAL

void
quant_est_t::DESTROY()
  CODE:
    /* TODO: call quant_est_t destructor instead... */
    free(THIS);

