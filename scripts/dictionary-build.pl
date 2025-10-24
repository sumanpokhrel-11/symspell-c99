#!/usr/bin/env perl
# scripts/dictionary-build.pl

use strict;
use warnings;
use Cwd qw(cwd);
use File::Basename qw(basename);
use Getopt::Long;

# Detect if running from scripts/ and adjust
my $cwd = cwd();
if (basename($cwd) eq 'scripts') {
    chdir '..' or die "Cannot chdir to parent: $!\n";
    print "Detected scripts/ directory, changed to: ", cwd(), "\n\n";
}

# Parse options
my $variant = '';  # Empty = use default (non-normalized)
GetOptions('en_us' => sub { $variant = 'en_us' },
           'en_uk' => sub { $variant = 'en_uk' });

# Paths
my $SHORTLIST = 'dictionaries/reference/wordlists/shortlist.txt';
my $WIKI_2M;
my $OUTPUT = 'dictionaries/test-dict.txt';

# Check if correct-*.txt files exist (needed for coverage testing)
my $test_file = $variant
    ? "test/data/symspell/misspellings/correct-codespell_${variant}.txt"
    : "test/data/symspell/misspellings/correct-codespell.txt";

unless (-f $test_file) {
    print "Test data files not found\n";
    print "Generating test data files...\n\n";

    my $normalize_tests = 'scripts/dictionary-normalize-test-data.pl';
    my @args = $variant ? ($^X, $normalize_tests, "--$variant") : ($^X, $normalize_tests);
    system(@args) == 0
        or die "Failed to generate test data files\n";

    print "\n";
    die "Error: Test data generation failed, $test_file not created\n" unless -f $test_file;
}

# Determine which Wikipedia dictionary to use
if ($variant) {
    # Normalized version requested
    $WIKI_2M = "dictionaries/reference/filter/dict-wiki_en_2_000_000_${variant}.txt";
    
    # Check if normalized Wikipedia exists
    unless (-f $WIKI_2M) {
        print "Normalized dictionary not found: $WIKI_2M\n";
        print "Generating normalized Wikipedia dictionary...\n\n";
        
        my $normalize_wiki = 'scripts/dictionary-normalize-wiki.pl';
        system($^X, $normalize_wiki, "--$variant") == 0
            or die "Failed to generate normalized Wikipedia dictionary\n";
        
        print "\n";
        die "Error: Normalization failed, $WIKI_2M not created\n" unless -f $WIKI_2M;
    }
    
    # Check if normalized test data exists
    my $test_file = "test/data/symspell/misspellings/misspell-codespell_${variant}.txt";
    unless (-f $test_file) {
        print "Normalized test data not found\n";
        print "Generating normalized test data...\n\n";
        
        my $normalize_tests = 'scripts/dictionary-normalize-test-data.pl';
        system($^X, $normalize_tests, "--$variant") == 0
            or die "Failed to generate normalized test data\n";
        
        print "\n";
        die "Error: Test normalization failed, $test_file not created\n" unless -f $test_file;
    }
} else {
    # Default: use original (non-normalized)
    $WIKI_2M = 'dictionaries/reference/filter/dict-wiki_en_2_000_000.txt';
}

# Verify files exist
die "Error: $SHORTLIST not found\n" unless -f $SHORTLIST;
die "Error: $WIKI_2M not found\n" unless -f $WIKI_2M;

print "=== Dictionary Builder ===\n";
print "Variant: " . ($variant || 'original (mixed UK/US)') . "\n\n";

# Step 1: Load base shortlist
print "1. Loading base shortlist... ";
my %wordlist;
open my $fh, '<', $SHORTLIST or die "Cannot open $SHORTLIST: $!\n";
while (<$fh>) {
    chomp;
    my ($word) = split /\s+/;
    next unless $word && length($word) > 0;
    $wordlist{lc($word)} = 1;
}
close $fh;
printf "   Loaded %d base words\n", scalar keys %wordlist;

# Step 2: Add additional wordlists from command line
if (@ARGV > 0) {
    print "2. Adding additional wordlists...\n";
    
    foreach my $file (@ARGV) {
        unless (-f $file) {
            warn "   Warning: $file not found, skipping\n";
            next;
        }
        
        my $before = scalar keys %wordlist;
        
        open my $add_fh, '<', $file or do {
            warn "   Warning: Cannot open $file: $!, skipping\n";
            next;
        };
        
        while (<$add_fh>) {
            chomp;
            my ($word) = split /\s+/;
            next unless $word && length($word) > 0;
            $wordlist{lc($word)} = 1;
        }
        close $add_fh;
        
        my $added = (scalar keys %wordlist) - $before;
        printf "   Added %d new words from %s\n", $added, $file;
    }
} else {
    print "2. No additional wordlists specified\n";
}

printf "3. Total wordlist size: %d words\n", scalar keys %wordlist;

# Step 3: Filter Wikipedia 2M with wordlist
print "4. Filtering Wikipedia 2M...\n";

open my $wiki_fh, '<', $WIKI_2M or die "Cannot open $WIKI_2M: $!\n";
open my $out_fh, '>', $OUTPUT or die "Cannot write $OUTPUT: $!\n";

my $filtered_count = 0;
while (<$wiki_fh>) {
    chomp;
    my ($word, $freq) = split /\s+/;
    next unless $word && $freq;
    
    if (exists $wordlist{lc($word)}) {
        print $out_fh "$word\t$freq\n";
        $filtered_count++;
    }
}

close $wiki_fh;
close $out_fh;

printf "5. Wrote %d words to %s\n", $filtered_count, $OUTPUT;
print "\nDone!\n";

if ($variant) {
    print "\nTest with:\n";
    print "  ./benchmark_symspell $OUTPUT test/data/symspell/misspellings/misspell-codespell_${variant}.txt\n";
} else {
    print "\nTest with:\n";
    print "  ./benchmark_symspell $OUTPUT test/data/symspell/misspellings/misspell-codespell.txt\n";
}
