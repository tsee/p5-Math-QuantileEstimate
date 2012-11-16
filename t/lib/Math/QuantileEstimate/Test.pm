package # Hide from PAUSE indexer
  Math::QuantileEstimate::Test;
use strict;
use warnings;

use Config qw(%Config);
use File::Spec;
use Capture::Tiny qw(capture);
use Exporter ();
use Test::More;

our @ISA = qw(Exporter);
our @EXPORT = qw(run_ctest is_approx);

our ($USE_VALGRIND, $USE_GDB);

my $in_testdir = not(-d 't');
my $base_dir;
my $ctest_dir;

if ($in_testdir) {
  $base_dir = File::Spec->updir;
  $USE_VALGRIND = -e File::Spec->catfile(File::Spec->updir, 'USE_VALGRIND');
  $USE_GDB = -e File::Spec->catfile(File::Spec->updir, 'USE_GDB');
}
else {
  $base_dir = File::Spec->curdir;
  $USE_VALGRIND = -e 'USE_VALGRIND';
  $USE_GDB = -e 'USE_GDB';
}
$ctest_dir = File::Spec->catdir($base_dir, 'ctest');

my @ctests = glob( "$ctest_dir/*.c" );
my @exe = grep -f $_, map {s/\.c$/$Config{exe_ext}/; $_} @ctests;

sub locate_exe {
  my $exe = shift;
  if (-f $exe) {
    return $exe;
  }
  my $inctest = File::Spec->catfile($ctest_dir, $exe);
  if (-f $inctest) {
    return $inctest;
  }
  return;
}

sub run_ctest {
  my ($executable, $options) = @_;
  my $to_run = locate_exe($executable);

  return if not defined $to_run;

  #my ($stdout, $stderr) = capture {
    my @cmd;
    if ($USE_VALGRIND) {
      push @cmd, "valgrind", "--suppressions=" .  File::Spec->catfile($base_dir, 'perl.supp');
    }
    elsif ($USE_GDB) {
      push @cmd, "gdb";
    }
    push @cmd, $to_run, ref($options)?@$options:();
    note("@cmd");
    system(@cmd)
      and fail("C test did not exit with 0");
  #};
  #print $stdout;
  #warn $stderr if defined $stderr and $stderr ne '';
  return 1;
}

sub is_approx {
  my ($l, $r, $m) = @_;
  my $is_undef = !defined($l) || !defined($r);
  $l = "<undef>" if not defined $l;
  $r = "<undef>" if not defined $r;
  my $ok = ok(
    !$is_undef
    && $l+1e-9 > $r
    && $l-1e-9 < $r,
    $m
  );
  note("'$m' failed: $l != $r") if not $ok;
  return $ok;
}

1;
