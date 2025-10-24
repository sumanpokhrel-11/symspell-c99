#!/usr/bin/env perl
#
# generate_wordlist.pl - Combine multiple wordlists into master list
#
# Takes one or more wordlist files and produces a single deduplicated,
# sorted master wordlist. Used to create the "inclusion filter" for
# dictionary building from multiple SCOWL frequency tiers.
#
# USAGE:
#   ./generate_wordlist.pl scowl-words-10.txt scowl-words-20.txt > wordlist.txt
#   ./generate_wordlist.pl scowl-words-*.txt > wordlist.txt
#
# PROCESSING:
#   - Reads all input files
#   - Extracts first word from each line (handles frequency data)
#   - Converts to lowercase
#   - Deduplicates
#   - Outputs sorted alphabetically
#
# INPUT FORMAT:
#   Each line can be:
#     word
#     word frequency
#     word\tfrequency
#
# OUTPUT:
#   One word per line, lowercase, sorted, unique
#

use strict;
use warnings;

if (@ARGV < 1) {
    print STDERR "Usage: $0 <file1> [file2] [file3] ...\n";
    exit 1;
}

my %words;

# Process each input file
for my $file (@ARGV) {
    open my $fh, '<', $file or die "Cannot open $file: $!\n";
    
    while (my $line = <$fh>) {
        chomp $line;
        $line = (split " ", $line)[0];  # Extract first word
        next unless $line;              # Skip empty lines
        $line = lc($line);              # Lowercase
        next if $line =~ /\s/;          # Skip if contains spaces
        $words{$line} = 1;
    }
    
    close $fh;
}

# Output sorted unique words
for my $word (sort keys %words) {
    print "$word\n";
}
