#!/usr/bin/env perl
# scripts/dictionary-build-extras.pl
use strict;
use warnings;
use Cwd qw(cwd);
use File::Basename;

# Configuration
my $WIKI_FREQ_CUTOFF = 42;  # Only load Wikipedia terms with freq >= this value

# Detect if running from scripts/ directory and adjust
my $cwd = cwd();
if ($cwd =~ m/scripts\/?$/ || basename($cwd) eq 'scripts') {
    chdir '..' or die "Cannot chdir to parent: $!\n";
    print "Detected scripts/ directory, changed to: ", cwd(), "\n\n";
}

# Paths (now relative to project root)
my $dict_file = 'dictionaries/dictionary.txt';
my $filter_file = 'dictionaries/reference/filter/dict-wiki_en_2_000_000.txt';  # FIXED
my $extras_dir = 'dictionaries/reference/extras';  # FIXED
my $output_file = 'dictionaries/dictionary-extras.txt';

# Files that need filtering (must exist in Wikipedia top 2M)
my %needs_filter = (
    'abbreviations.txt' => 1,
    'davids-batista.txt' => 1,
);

print "=== Dictionary Extras Builder ===\n\n";

print "Loading existing dictionary...\n";
my %existing = load_dictionary($dict_file);
printf "  Loaded %d existing terms\n", scalar keys %existing;

print "Loading Wikipedia filter (freq >= $WIKI_FREQ_CUTOFF)...\n";
my %wiki_filter = load_wiki_filter($filter_file, $WIKI_FREQ_CUTOFF);
printf "  Loaded %d Wikipedia terms\n\n", scalar keys %wiki_filter;

print "Processing extras directory...\n";
my %all_terms;

opendir(my $dh, $extras_dir) or die "Cannot open $extras_dir: $!\n";
my @files = sort grep { /\.txt$/ && -f "$extras_dir/$_" } readdir($dh);
closedir($dh);

foreach my $file (@files) {
    my $path = "$extras_dir/$file";
    my $should_filter = exists $needs_filter{$file};
    
    printf "  Processing %s (%s)\n", $file, 
           $should_filter ? "with Wikipedia filter" : "no filter";
    
    open my $fh, '<', $path or die "Cannot open $path: $!\n";
    
    my $added = 0;
    my $skipped_existing = 0;
    my $skipped_filter = 0;
    
    while (my $line = <$fh>) {
        chomp $line;
        
        # Skip comments and blank lines
        next if $line =~ /^\s*#/;
        next if $line =~ /^\s*$/;
        
        # Parse term and count
        my ($term, $count) = split /\t/, $line;
        next unless defined $term && $term =~ /^[a-z]+$/;
        
        $count = 1 unless defined $count;
        
        # Skip if already in main dictionary
        if (exists $existing{$term}) {
            $skipped_existing++;
            next;
        }
        
        # Apply Wikipedia filter if needed
        if ($should_filter && !exists $wiki_filter{$term}) {
            $skipped_filter++;
            next;
        }
        
        # Add to output (keep highest count if duplicate)
        if (!exists $all_terms{$term} || $count > $all_terms{$term}) {
            $all_terms{$term} = $count;
        }
        $added++;
    }
    
    close $fh;
    printf "    Added: %d, Skipped (existing): %d, Skipped (filter): %d\n",
           $added, $skipped_existing, $skipped_filter;
}

print "\nWriting output...\n";
open my $out, '>', $output_file or die "Cannot write $output_file: $!\n";

print $out "# Generated dictionary extras\n";
print $out "# Merged from reference/extras/ with Wikipedia filter (freq >= $WIKI_FREQ_CUTOFF)\n";
print $out "# " . localtime() . "\n\n";

my $total = 0;
foreach my $term (sort keys %all_terms) {
    printf $out "%s\t%d\n", $term, $all_terms{$term};
    $total++;
}

close $out;

printf "\nDone! Wrote %d terms to %s\n", $total, $output_file;

# Subroutines
sub load_dictionary {
    my ($file) = @_;
    my %dict;
    
    open my $fh, '<', $file or die "Cannot open $file: $!\n";
    while (my $line = <$fh>) {
        chomp $line;
        next if $line =~ /^\s*#/;
        next if $line =~ /^\s*$/;
        
        my ($term, $count) = split /[\s\t]+/, $line;
        next unless defined $term && length($term) > 0;
        
        $dict{lc($term)} = $count || 1;
    }
    close $fh;
    
    return %dict;
}

sub load_wiki_filter {
    my ($file, $min_freq) = @_;
    my %dict;
    
    open my $fh, '<', $file or die "Cannot open $file: $!\n";
    while (my $line = <$fh>) {
        chomp $line;
        next if $line =~ /^\s*#/;
        next if $line =~ /^\s*$/;
        
        my ($term, $count) = split /[\s\t]+/, $line;
        next unless defined $term && length($term) > 0;
        next unless defined $count && $count >= $min_freq;
        
        $dict{lc($term)} = $count;
    }
    close $fh;
    
    return %dict;
}
