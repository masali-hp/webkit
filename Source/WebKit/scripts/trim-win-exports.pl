#!/usr/bin/perl

use strict;

use Config;
use Getopt::Long;
use File::Path;

my $usage = "generate-win-exports --input exports-def.i";

my $inputFile = "";
my $architecture = "";
my $exportsFound = 0;

GetOptions('input=s' => \$inputFile,
           'architecture=s' => \$architecture);

die "You must specify a --input <file> " unless (length($inputFile));

unless (open INPUT, "<", $inputFile) { print STDERR "File does not exist: $inputFile\n";}
while (my $line = <INPUT>) {
    chomp $line;
    if (!$exportsFound) {
        if ($line eq "EXPORTS") {
            $exportsFound = 1;
            print $line . "\n";
        }
    }
    else {
        if (!$line eq "") {
            if ($architecture eq "ARM") {
                # Arguments like @QBE need to become @QBA
                $line =~ s/(\@[AIMQU][AB])E/\1A/;
            }
            print $line . "\n";
        }
    }
}



