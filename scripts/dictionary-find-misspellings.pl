#!/usr/bin/env perl
# scripts/find-misspellings-in-dictionary.pl
#
# Find terms in dictionary that are actually misspellings
# Checks against all test misspelling data

use strict;
use warnings;

my $dict_file = $ARGV[0] || 'dictionaries/dictionary.txt';

# Load dictionary
my %dict;
open my $fh, '<', $dict_file or die "Cannot open $dict_file: $!\n";
while (<$fh>) {
    chomp;
    next if /^\s*#/ || /^\s*$/;
    my ($term) = split /\s+/;
    $dict{lc($term)} = 1 if $term;
}
close $fh;

# Check against all misspelling files
my @misspell_files = glob('test/data/symspell/misspellings/misspell-*.txt');

my %found;
foreach my $file (@misspell_files) {
    open my $mfh, '<', $file or next;
    
    while (<$mfh>) {
        chomp;
        my ($wrong, $correct) = split /\s+/;
        next unless $wrong;
        
        if (exists $dict{lc($wrong)}) {
            $found{lc($wrong)} = $correct;
        }
    }
    close $mfh;
}

# Report findings
if (%found) {
    print "Found misspellings in dictionary:\n";
    foreach my $wrong (sort keys %found) {
        printf "  %s (should be: %s)\n", $wrong, $found{$wrong};
    }
    printf "\nTotal: %d misspellings found\n", scalar keys %found;
    exit 1;
} else {
    print "✓ Dictionary is clean (no misspellings found)\n";
    exit 0;
}
