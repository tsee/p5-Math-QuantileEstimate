package Math::QuantileEstimate;
use 5.008001;
use strict;
use warnings;
use Carp qw(croak);

our $VERSION = '0.01';

require XSLoader;
XSLoader::load('Math::QuantileEstimate', $VERSION);

require Exporter;
our @ISA = qw(Exporter);
our @EXPORT_OK = qw();
our %EXPORT_TAGS = ('all' => \@EXPORT_OK);

1;
__END__

=head1 NAME

Math::QuantileEstimate - Online calculation of quantiles with low memory overhead

=head1 SYNOPSIS

  use Math::QuantileEstimate;

=head1 DESCRIPTION

=head1 METHODS

=head2 C<new>

Constructor.

=head1 SEE ALSO

The algorithm implemented here is following:
Qi Zhang and Wei Wang, "A Fast Algorithm for Approximate Quantiles in High Speed Data Streams",
19th International Conference on Scientific and Statistical Database Management (SSDBM 2007)

Other algorithms (with different space/time trade-off):
Michael Greenwald and Sanjeev Khanna, "Space-Efﬁcient Online Computation of Quantile Summaries",
at SIGMOD (2001), p. 58-66.

For an exact O(n) time-complexity selection algorithm (quantile/median calculation),
see C<select_kth> in the L<Statistics::CaseResampling> module on CPAN.

=head1 AUTHOR

Steffen Mueller, E<lt>smueller@cpan.orgE<gt>

=head1 COPYRIGHT AND LICENSE

Copyright (C) 2012 by Steffen Mueller

This library is free software; you can redistribute it and/or modify
it under the same terms as Perl itself, either Perl version 5.8.1 or,
at your option, any later version of Perl 5 you may have available.

=cut
