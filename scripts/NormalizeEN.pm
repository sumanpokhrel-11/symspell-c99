package NormalizeEN;
use strict;
use warnings;

sub new {
    my ($class, %args) = @_;
    
    my $self = {
        lang => $args{lang} || 'en_us',
        dict => {},
    };
    
    bless $self, $class;
    
    # Load dictionary if provided
    if ($args{dict}) {
        $self->load_dict($args{dict});
    }
    else {
        die("A reference dictionary must be supplied!\n");
    }
    
    # Setup transforms and exceptions
    $self->_setup_exceptions();
    $self->_setup_transforms();  # This compiles the code refs
    
    return $self;
}

sub load_dict {
    my ($self, $dict_file) = @_;
    
    open my $fh, '<', $dict_file or die "Cannot open $dict_file: $!\n";
    while (<$fh>) {
        chomp;
        my ($word, $freq) = split /\s+/;
        next unless $word && $freq;
        $self->{dict}{$word} = $freq;
    }
    close $fh;
}

sub _setup_transforms {
    my ($self) = @_;
    
    my @patterns;
    
    if ($self->{lang} eq 'en_us') {
        @patterns = (
            's/\Bour/or/',
            's/\Bise/ize/',
            's/\Bisation/ization/',
            's/\Byse/yze/',
            's/(t|b|c|f|g|l|m|p)re/$1er/',
            's/\Blogue/log/',
            's/\B([^l])ogue/$1og/',
            's/ae/e/',
            's/oe/e/',
            's/\B([fl])ence/$1ense/',
            's/\Bll(ed|ing|er)/l$1/',
            's/\Bmme/m/',
        );
    } elsif ($self->{lang} eq 'en_uk') {
        @patterns = (
            's/\Bor/our/',
            's/\Bize/ise/',
            's/\Bization/isation/',
            's/\Byze/yse/',
            's/(t|b|c|f|g|l|m|p)er/$1re/',
            's/\Blog/logue/',
            's/\B([^l])og/$1ogue/',
            's/\B([fl])ense/$1ence/',
            's/\Bl(ed|ing|er)/ll$1/',
            's/\Bm$/mme/',
        );
    }
    
    # Compile patterns into code refs
    $self->{transforms} = $self->_compile_transforms(@patterns);
}

sub _compile_transforms {
    my ($self, @transform_strings) = @_;
    my @compiled;
    
    for my $trans_str (@transform_strings) {
        if ($trans_str =~ /^s\/(.+)\/(.*)\/$/) {
            my ($pattern, $replacement) = ($1, $2);
            
            # Create code ref that does the substitution
            push @compiled, sub {
                my $word = shift;
                $word =~ s/$pattern/$replacement/e;
                return $word;
            };
        } else {
            warn "Could not parse transform: $trans_str\n";
        }
    }
    
    return \@compiled;
}

sub _setup_exceptions {
    my ($self) = @_;
    
    my %uk_to_us = (
        'grey'          => 'gray',
        'tyre'          => 'tire',
        'jewellery'     => 'jewelry',
        'aluminium'     => 'aluminum',
        'plough'        => 'plow',
        'sceptic'       => 'skeptic',
        'cheque'        => 'check',
        'mould'         => 'mold',
        'smoulder'      => 'smolder',
        'moustache'     => 'mustache',
        'pyjama'        => 'pajama',
        'pyjamas'       => 'pajamas',
        'kerb'          => 'curb',
        'aeroplane'     => 'airplane',
        'draught'       => 'draft',
        'storey'        => 'story',
        'behove'        => 'behoove',
        'manoeuvre'     => 'maneuver',
        'paediatric'    => 'pediatric',
        'encyclopaedia' => 'encyclopedia',
        'archaeology'   => 'archeology',
    );
    
    if ($self->{lang} eq 'en_us') {
        $self->{exceptions} = \%uk_to_us;
    } elsif ($self->{lang} eq 'en_uk') {
        my %us_to_uk = reverse %uk_to_us;
        $self->{exceptions} = \%us_to_uk;
    } else {
        $self->{exceptions} = {};
    }
}

# Transform a word - returns transformed word if it exists in dict, undef otherwise
sub transform {
    my ($self, $word) = @_;
    
    # Check exceptions first
    if (exists $self->{exceptions}{$word}) {
        my $target = $self->{exceptions}{$word};
        # Only return if target exists in dictionary (or no dict loaded)
        if (!%{$self->{dict}} || exists $self->{dict}{$target}) {
            return $target;
        }
    }
    
    # Try cascading transforms using compiled code refs
    my $current = $word;
    for my $transform_sub (@{$self->{transforms}}) {
        my $variant = $transform_sub->($current);
        
        # If transform changed the word
        if ($variant ne $current) {
            # Check if it exists in dictionary (or accept if no dict)
            if (!%{$self->{dict}} || exists $self->{dict}{$variant}) {
                return $variant;
            }
            # Update current for cascading
            $current = $variant;
        }
    }
    
    # No valid transform found
    return undef;
}

1;
