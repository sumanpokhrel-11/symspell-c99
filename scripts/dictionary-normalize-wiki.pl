#!/usr/bin/env perl
# scripts/dictionary-normalize-wiki.pl

use strict;
use warnings;
use Getopt::Long;
use FindBin qw($Bin);
use lib $Bin;
use NormalizeEN;

$| = 1;

my $variant = 'en_us';
GetOptions('en_us' => sub { $variant = 'en_us' },
           'en_uk' => sub { $variant = 'en_uk' });

my $input = 'dictionaries/reference/filter/dict-wiki_en_2_000_000.txt';
my $output = "dictionaries/reference/filter/dict-wiki_en_2_000_000_${variant}.txt";

my $start = time();

print "=== Wikipedia 2M Normalizer ===\n";
print "Variant: $variant\n\n";

# Load Wikipedia 2M (global)
print "Loading $input...\n";
our %wiki_2m;
open my $in, '<', $input or die "Cannot open $input: $!\n";
while (<$in>) {
    chomp;
    my ($word, $freq) = split /\s+/;
    next unless $word && $freq && $freq =~ /^\d+$/;
    $wiki_2m{$word} = $freq;
}
close $in;

printf "Loaded %d terms\n\n", scalar keys %wiki_2m;

# Create normalizer with dictionary loaded
print "Initializing normalizer...\n";
my $normalizer = NormalizeEN->new(
    lang => $variant,
    dict => $input
);
print "Ready.\n\n";

# Normalize variants
print "Normalizing variants...\n";
my $merged = 0;
my $processed_count = 0;
my $total_words = scalar keys %wiki_2m;

for my $word (keys %wiki_2m) {
    $processed_count++;
    
    if ($processed_count % 10000 == 0) {
        printf "\rProcessed: %07d / %d (%.1f%%)", 
               $processed_count, $total_words, 
               ($processed_count / $total_words) * 100;
    }

    next unless $wiki_2m{$word};  # Skip if already zeroed
    
    # Try to transform
    my $target = $normalizer->transform($word);
    
    if ($target && $target ne $word) {
        # Merge frequencies
        $wiki_2m{$target} += $wiki_2m{$word};
        $wiki_2m{$word} = 0;
        $merged++;
    }
}

printf "\rProcessed: %d / %d (100.0%%)\n", $processed_count, $total_words;
printf "\nMerged %d variant pairs\n", $merged;

# Write output
print "\nWriting $output...\n";
open my $out, '>', $output or die "Cannot write $output: $!\n";

my $final_count = 0;
for my $word (sort { $wiki_2m{$b} <=> $wiki_2m{$a} } keys %wiki_2m) {
    next unless $wiki_2m{$word};
    printf $out "%s\t%d\n", $word, $wiki_2m{$word};
    $final_count++;
}

close $out;

printf "Result: %d unique terms written\n", $final_count;
my $duration = time() - $start;
print "Done in $duration sec!\n";
