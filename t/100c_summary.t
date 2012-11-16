use strict;
use warnings;
BEGIN {
  push @INC, 't/lib', 'lib';
}
use Math::QuantileEstimate::Test;

run_ctest('100summary')
  or Test::More->import(skip_all => "C executable not found");


