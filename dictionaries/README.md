# Dictionary Reference Materials

This directory contains source materials and tools for building CGIOS dictionaries.

## Structure
```
reference/
├── dictionaries/         # Built dictionaries (snapshots)
├── extras/              # Acronyms and technical terms
├── filter/              # Wikipedia frequency data
└── wordlists/           # SCOWL wordlists
```

## Files

### `dictionaries/`
- **`dictionary.txt`**: Snapshot of core dictionary
- **`dictionary-extras.txt`**: Snapshot of extras
- **`dict-freq_en_82_765.txt`**: Original frequency-based dictionary

### `extras/`
Source files for `dictionary-extras.txt`:
- **`tech_terms.txt`**: Curated technical terms (no filter)
- **`foss-acronyms.txt`**: FOSS community acronyms (no filter)
- **`abbreviations.txt`**: Internet slang (no filter)
- **`davids-batista.txt`**: Academic terms (no filter)

### `filter/`
- **`dict-wiki_en_2_000_000.txt`**: Top 2M Wikipedia terms by frequency
  - Used to filter `abbreviations.txt` and `davids-batista.txt`
  - See `scripts/build_dictionary_extras.pl`

### `wordlists/`
SCOWL (Spell Checker Oriented Word Lists) subsets:
- **`scowl-words-{10-95}.txt`**: Frequency tiers (10=common, 95=rare)
- **`wordlist.txt`**: Combined master wordlist (inclusion filter)
- **`build-scowl.sh`**: Extract SCOWL tiers
- **`generate_wordlist.pl`**: Combine tiers into master list

## Building Dictionaries

### Core Dictionary
Built using frequency data + SCOWL inclusion filter:
```bash
# See scripts/dictionary-build.sh
```

### Extras Dictionary
```bash
scripts/build_dictionary_extras.pl
```

Generates `dictionaries/dictionary-extras.txt` from:
1. Curated sources (tech_terms, foss-acronyms) - no filter
2. Large sources (abbreviations, davids-batista) - Wikipedia filtered

## Philosophy

**Core dictionary (86k words):**
- High-quality, general English vocabulary
- Optimized for accuracy on prose

**Extras dictionary (25k words):**
- Technical terms, acronyms, abbreviations
- Optimized for preserving domain terminology
- Prevents "correcting" `kubectl` → `bucket`

**Combined (111k words):**
- Used in CGIOS for n-gram indexing
- Preserves semantic addresses for technical content
- Slight accuracy trade-off (−0.1%) is acceptable for our use case
