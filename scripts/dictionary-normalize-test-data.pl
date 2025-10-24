#!/usr/bin/env perl
# scripts/dictionary-normalize-test-data.pl

use strict;
use warnings;
use Getopt::Long;
use FindBin qw($Bin);
use lib $Bin;
use NormalizeEN;

$| = 1;

my $variant = '';
GetOptions('en_us' => sub { $variant = 'en_us' },
           'en_uk' => sub { $variant = 'en_uk' });

my $test_dir = 'test/data/symspell/misspellings';

print "=== Test Data Normalizer ===\n";

my $normalizer;
if ($variant) {
    print "Variant: $variant (will normalize)\n\n";
    
    # Determine which dictionary to use
    my $wiki_dict = "dictionaries/reference/filter/dict-wiki_en_2_000_000_${variant}.txt";
    
    # Check if normalized dict exists, if not use original
    unless (-f $wiki_dict) {
        $wiki_dict = 'dictionaries/reference/filter/dict-wiki_en_2_000_000.txt';
        print "Normalized dictionary not found, using original Wikipedia 2M\n";
    }
    
    print "Using dictionary: $wiki_dict\n";
    
    # Create normalizer WITH dictionary
    $normalizer = NormalizeEN->new(
        lang => $variant,
        dict => $wiki_dict
    );
    print "Normalizer ready.\n\n";
} else {
    print "No variant specified (will generate correct-*.txt only)\n\n";
}

# Process files
opendir(my $dh, $test_dir) or die "Cannot open $test_dir: $!\n";
my @misspell_files = sort grep { /^misspell-.+\.txt$/ && -f "$test_dir/$_" } readdir($dh);
closedir($dh);

printf "Found %d test files\n\n", scalar @misspell_files;

foreach my $file (@misspell_files) {
    my $input = "$test_dir/$file";
    my ($base) = $file =~ /^misspell-(.+)\.txt$/;
    next unless $base;
    
    print "Processing $file...\n";
    
    my @pairs;
    open my $in, '<', $input or die "Cannot open $input: $!\n";
    while (<$in>) {
        chomp;
        my ($wrong, $correct) = split /\s+/, $_, 2;
        next unless $wrong && $correct;
        
        # Skip template entries with $1, $2, etc.
        next if $wrong =~ /\$\d+/ || $correct =~ /\$\d+/;
        
        # Normalize if variant specified
        if ($normalizer) {
            my $normalized = $normalizer->transform($correct);
            $correct = $normalized if $normalized;
        }
        
        push @pairs, [$wrong, $correct];
    }
    close $in;
    
    if ($variant) {
        # Write normalized misspell file
        my $output_misspell = "$test_dir/misspell-${base}_${variant}.txt";
        open my $out_miss, '>', $output_misspell or die "Cannot write $output_misspell: $!\n";
        foreach my $pair (@pairs) {
            printf $out_miss "%s\t%s\n", $pair->[0], $pair->[1];
        }
        close $out_miss;
        printf "  Wrote %d pairs to %s\n", scalar @pairs, $output_misspell;
        
        # Write normalized correct file
        my $output_correct = "$test_dir/correct-${base}_${variant}.txt";
        open my $out_corr, '>', $output_correct or die "Cannot write $output_correct: $!\n";
        foreach my $pair (@pairs) {
            printf $out_corr "%s\t%s\n", $pair->[1], $pair->[1];
        }
        close $out_corr;
        printf "  Wrote %d pairs to %s\n", scalar @pairs, $output_correct;
        
    } else {
        # Just write correct file (no normalization)
        my $output_correct = "$test_dir/correct-${base}.txt";
        open my $out_corr, '>', $output_correct or die "Cannot write $output_correct: $!\n";
        foreach my $pair (@pairs) {
            printf $out_corr "%s\t%s\n", $pair->[1], $pair->[1];
        }
        close $out_corr;
        printf "  Wrote %d pairs to %s\n", scalar @pairs, $output_correct;
    }
}

print "\nDone!\n";
