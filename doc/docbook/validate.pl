#!/usr/bin/env perl

use File::Basename;

my $top_srcdir = qx(pwd);
chomp $top_srcdir;

# global vars

# user edit this file for local validation tool paths:
use BRLCAD_DB_VALIDATION;

# for DocBook (DB) validation
my $XML_UTF8_HEADER    = "<?xml version='1.0' encoding='UTF-8'?>";
my $XML_ASCII_HEADER   = "<?xml version='1.0' encoding='ASCII'?>";
my $XML_TMPFILE        = '.tmp';
my $RNG_SCHEMA         = './resources/docbook-5.0/rng/docbookxi.rng';

# for xmllint DB validation
my $XMLLINT_VALID_ARGS = "--xinclude --relaxng ${RNG_SCHEMA} --noout --nonet";

# for msv DB validation
my $MSVCMD     = "$JAVA -Xss1024K -jar ${MSVJAR}";


my $vlog_asc = 'validate-asc-fail.log';
my $vlog_utf = 'validate-utf-fail.log';
my $vfil     = 'db-file-list.txt';
my $sfil     = 'find-db-files.pl';

my $prog = basename($0);
my $usage = "Usage: $prog --go | <input file> [options...]";
if (!@ARGV) {
  print <<"HERE";
$usage

Use option '--help' or '-h' for more details.
HERE
  exit;
}

my $enc  = undef;
my $meth = 'msv';
my $stop = 0;
my $ifil = 0;
my $verb = 0;
my $warnings = 1; # for MSV
foreach my $arg (@ARGV) {
  my $arg = shift @ARGV;
  my $val = undef;
  my $idx = index $arg, '=';

  if ($idx >= 0) {
    $val = substr $arg, $idx+1;
    $arg = substr $arg, 0, $idx;
  }

  if ($arg =~ /^[-]{1,2}h/i) {
    help(); # exits from help()
  }
  elsif ($arg =~ /^[-]{1,2}g/i) {
    ; # ignore
  }
  elsif ($arg =~ /^[-]{1,2}v/i) {
    $verb = 1;
  }
  elsif ($arg =~ /^[-]{1,2}n/i) {
    $warnings = 0;
  }
  elsif ($arg =~ /^[-]{1,2}s/i) {
    $stop = 1;
  }
  elsif ($arg =~ /^[-]{1,2}enc/i) {
    $enc = $val;
  }
  elsif ($arg =~ /^[-]{1,2}met/i) {
    die "ERROR:  Undefined method.\n"
      if !defined $val;
    $meth = $val;
    die "ERROR:  Method is unknown: '$meth'.\n"
      if ($meth !~ /xml/i && $meth !~ /msv/i && $meth !~ /nvd/i);
    $meth = ($meth =~ /xml/i) ? 'xmllint'
          : ($meth =~ /msv/i) ? 'msv' : 'nvdl';
  }
  elsif ($arg =~ /^[-]{1,2}sto/i) {
    $stop = 1;
  }
  elsif (!$ifil) {
    $ifil = $arg;
  }
  else {
    die "ERROR:  Unknown argument '$arg'.\n"
  }
}

# error checks
my $errors = 0;
# check for input file
if (!$ifil) {
  if (-f $vfil) {
    $ifil = $vfil;
  }
  else {
    print "ERROR: No input file entered.\n";
    ++$errors;
  }
}
elsif (! -f $ifil) {
  print "ERROR:  Input file '$ifil' not found.\n";
  ++$errors;
}

if (defined $enc) {
  if ($enc !~ /asc/i && $enc !~ /utf/i) {
    print "ERROR:  Encoding '$enc' is unknown.\n";
    ++$errors;
  }
  else {
    $enc = ($enc =~ /asc/i) ? 'ascii' : 'utf8';
  }
}

die "ERROR exit.\n"
  if $errors;

open my $fp, '<', $ifil
  or die "$ifil: $!\n";

my %dbfils = ();
my @dbfils_asc = ();
my @dbfils_utf = ();

while (defined(my $line = <$fp>)) {
  # eliminate any comments
  my  $idx = index $line, '#';
  if ($idx >= 0) {
    $line = substr $line, 0, $idx;
  }

  # tokenize (may have one or more files on the line)
  my @d = split(' ', $line);
  next if !defined $d[0];
  while (@d) {
    my $f = shift @d;
    if (! -f $f) {
      print "WARNING:  DB file '$f' not found...skipping.\n"
	if $verbose;
      next;
    }
    # is the encoding determinate?
    my $tenc = 'ascii';
    my $lang = undef;
    $f =~ m{(?: \A | [/\\]{1}) ([a-z]{2}) [/\\]{1} }xmsi;
    if ($1) {
      $lang = lc $1;
      # we have a possible language couplet
      if ($lang eq 'en') {
        $tenc = 'ascii';
      }
      else {
        $tenc = 'utf8';
      }
    }
    elsif (defined $enc) {
      $tenc = $enc;
    }

    $dbfils{$f}{$tenc} = 1;
    if ($tenc eq 'ascii') {
      push @dbfils_asc, $f;
    }
    else {
      push @dbfils_utf, $f;
    }
    print "DB file '$f' language code '$lang', using encoding '$tenc'.\n"
	if $verbose;

  }
}
close $fp;

my $enc_prev = '';
my ($hdr, $vlog, $typ);

foreach my $f (@dbfils_asc, @dbfils_utf) {
  my $enc = $dbfils{$f};

  # set encoding-specific variables
  if ($enc ne $enc_prev) {
    if ($enc eq 'ascii') {
      $hdr  = $XML_ASCII_HEADER;
      $vlog = $vlog_asc;
      $typ = 'ASCII';
      unlink $vlog if -f $vlog;
    }
    else {
      $hdr  = $XML_UTF8_HEADER;
      $vlog = $vlog_utf;
      $typ = 'UTF8';
      unlink $vlog if -f $vlog;
    }
  }
  $enc_prev = $enc;

  qx(echo "=== $typ VALIDATION ===" >> $vlog );

  my $dir = dirname($f);
  my $fil = basename($f);
  my $tmpfil = "${dir}/${XML_TMPFILE}.${fil}";
  unlink $tmpfil if -f $tmpfil;
  qx(echo "$hdr" > $tmpfil);
  qx(cat $f >> $tmpfil);
  print "=== processing file '$f' (see file '$tmpfil')\n"
    if $verbose;

  if ($meth eq 'msv') {
    validate_msv($tmpfil);
  }
  elsif ($meth eq 'xmllint') {
    validate_xmllint($tmpfil);
  }
  else {
    validate_nvdl($tmpfil);
  }

  print "=== finished processing file '$f' (see file '$tmpfil')\n"
    if $verbose;

}

print "Normal end.\n";
print "See ASCII validation log file '$vlog_asc'.\n"
  if -f $vlog_asc;
print "See UTF-8 validation log file '$vlog_utf'.\n"
  if -f $vlog_utf;

#### SUBROUTINES ####
sub validate_xmllint {
  my $tmpfil = shift @_;

  my $cmd = "$XMLLINT $XMLLINT_VALID_ARGS $tmpfil";
  print "=== cmd: '$cmd'\n";
  my $msg = qx($cmd);
  if ($msg) {
    chomp $msg;
    print "msg: '$msg'\n";
  }

} # validate_xmllint

sub validate_msv {
  my $tmpfil = shift @_;

  my $warn = $$warnings ? '-warning' : '';
  my $cmd = "$MSVCMD $warn $RNG_SCHEMA $tmpfil";
  print "=== cmd: '$cmd'\n";
  my $msg = qx($cmd);
  if ($msg) {
    chomp $msg;
    print "msg: '$msg'\n";
  }

} # validate_msv

sub validate_nvdl {
  die "The 'nvdl' method is not yet supported'.";

  my $tmpfil = shift @_;

  die "Tom, fix this!";

  my $cmd = "?";
  print "=== cmd: '$cmd'\n";
  my $msg = qx($cmd);
  if ($msg) {
    chomp $msg;
    print "msg: '$msg'\n";
  }

} # validate_nvdl


sub help {
  print <<"HERE";
$usage

Options:

  --encoding=E    where E is one of 'ascii' or 'utf8' [default: auto]

                  If the file path contains a two-letter language
                  code bounded by path separators or a relative
                  path starting with the language code, the '/en/'
                  (or '\\en\\' or ' en/' or ' en\\') files will use
                  'ascii' and all others will use 'utf8'.  The
                  default is 'ascii' otherwise.

  --method=M      where M is one of 'msv, 'xmllint', or 'nvdl'
                  [default: msv]

                  The 'msv' method use the Sun Multi-Schema Validator
                  (MSV) and the 'docbook-5.0/docbookxi.rng' schema.

                  The 'xmllint' method uses libXML2's xmllint plus the
                  'docbook-5.0/rng/docbookxi.rng' schema.

                  The 'nvdl' method uses the Apache Xerces xml parser
                  and the 'docbook-5.0/docbookxi.nvdl' schema.

  --stop-on-fail  stop after the first validation failure

  --verbose       show more info about what's going on

  --no-warnings   do not show validation warnings (used for 'msv' only)

Notes:

File and directory names must not have spaces.

The list file may be generated with the shell script '$sfil'
The output file is named '$vfil'.  If the file name
'$vfil' is found it will be used automatically if no file
name is entered at the prompt (in that case at least on arg must be
entered so use '--go' if no other arg is needed).

In the list file all blank lines and all characters on a line at and
after the '#' symbol are ignored.

HERE
  exit;
} # help
