#!/bin/bash
#
# build-scowl.sh - Extract SCOWL wordlist frequency tiers
#
# SCOWL (Spell Checker Oriented Word Lists) provides English vocabulary
# categorized by usage frequency/commonality:
#   10 = most common everyday words
#   20 = common words
#   35 = standard vocabulary
#   50 = typical desktop dictionary
#   70 = larger dictionary
#   95 = comprehensive (includes rare/archaic terms)
#
# SETUP:
#   1. Download SCOWL from: https://github.com/en-wl/wordlist
#   2. Extract and cd into the scowl directory
#   3. Run this script to generate frequency-tiered wordlists
#
# OPTIONS:
#   -p    Retain possessive forms (e.g., "word's")
#         Default: strip possessives to reduce duplicates
#
# USAGE:
#   ./build-scowl.sh        # Strip possessives (default)
#   ./build-scowl.sh -p     # Keep possessives
#
# OUTPUT:
#   scowl-words-{10,20,35,40,50,55,60,70,80,95}.txt
#
# NOTES:
#   - Converts ISO-8859-1 to UTF-8
#   - Removes carriage returns (Windows line endings)
#   - Sorts and deduplicates output
#

# Possessive duplicates ("$word" and "$word's") are stripped by default
# Pass -p as the first arg to retain them
if [ "$1" == "-p" ]; then
    POSSESSIVE=1
else
    POSSESSIVE=
fi

for SIZE in 10 20 35 40 50 55 60 70 80 95
do
    SCOWL_FILE=scowl-words-$SIZE.txt
    perl mk-list english $SIZE |
        iconv -f ISO-8859-1 -t UTF-8 |
        tr -d '\r' |
        ( [[ ! "$POSSESSIVE" ]] && sed -E "s/'s$//g" | sort -u || cat ) > $SCOWL_FILE
    SCOWL_WORDS=$(wc -l $SCOWL_FILE | sed -E 's/ *([0-9]+) .*/\1/')
    echo "Created '$SCOWL_FILE' ($SCOWL_WORDS words)"
done
