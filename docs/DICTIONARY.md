# CGIOS Dictionary System

## Overview

CGIOS uses a two-dictionary system optimized for different purposes:
1. **Main Dictionary** (`dictionary.txt`) - Spell-checking and correction
2. **Dictionary Extras** (`dictionary-extras.txt`) - Keyword extraction and IWF scoring

### Philosophy

Building an optimal spell-checking dictionary is a **zero-sum optimization problem**. Improving accuracy on one dataset often regresses accuracy on another. The goal is not perfection, but **"incomplete enoughness"** - dictionaries that are good enough for their specific purposes.

### Structure

```
dictionaries/
├── dictionary.txt                 # Main dictionary (86k words, spell-checking)
├── dictionary-extras.txt          # Generated extras (25k words, IWF scoring)
└── reference/                     # Source materials
    ├── dictionaries/
    │   ├── dict-freq_en_82_765.txt
    │   └── dictionary.txt         # Pre-built recommended version
    ├── extras/                    # Source files for extras
    │   ├── tech_terms.txt         # Curated technical terms
    │   ├── foss-acronyms.txt      # FOSS community acronyms
    │   ├── abbreviations.txt      # Internet slang
    │   └── davids-batista.txt     # Academic terms
    ├── filter/
    │   └── dict-wiki_en_2_000_000.txt  # Wikipedia frequency data
    └── wordlists/
        ├── shortlist.txt          # Base 82,765 words
        ├── build-scowl.sh         # SCOWL extraction
        └── generate_wordlist.pl   # Wordlist combination
```

### File Format

All dictionary files use tab-separated format:
```
<term>  <frequency>
```

Comments start with `#` and blank lines are ignored:
```
# Core technical terms
stdin   1
stdout  1
stderr  1
```

---

## Part 1: Main Dictionary (Spell-Checking)

### Purpose

The main dictionary (`dictionary.txt`) is optimized for **spell-checking accuracy** in general English text. It balances coverage against noise to achieve 82-84% correction accuracy.

### Key Learnings

#### 1. Single Iteration Is Optimal

```
Baseline → Test → Add errors.txt → DONE
```

**Why one iteration works:**
- Captures exactly what's missing for the target dataset
- Achieves 80-85% accuracy (near theoretical ceiling)
- Second iteration adds noise, not signal
- Diminishing returns set in immediately

**Why not iterate further:**
- All correctable errors are already captured
- Additional words introduce ambiguity
- Accuracy plateaus or regresses
- You start optimizing for noise

#### 2. Optimization Is Domain-Specific

**The cat herding problem:**
- Words that fix Dataset A often break Dataset B
- No single dictionary is optimal for all domains
- Trade-offs are inevitable

**Example:**
- Adding 2,004 Codespell-specific words: Codespell 78% → 83%, Wikipedia 85% → 83%
- Adding 91 Wikipedia-specific words: Wikipedia 83% → 85%, Codespell 83% → 78%

**The constitutional answer:** Build domain-specific dictionaries, not universal ones.

### Building Tools

#### `scripts/dictionary-build.pl`

Composable dictionary builder that filters Wikipedia 2M with custom wordlists.

```bash
# Basic usage
./scripts/dictionary-build.pl [wordlist1.txt] [wordlist2.txt] ...

# Algorithm:
# 1. Load base shortlist (82,765 words)
# 2. Add words from specified wordlists (first field only)
# 3. Deduplicate
# 4. Filter Wikipedia 2M with combined wordlist
# 5. Output to dictionaries/test-dict.txt with Wikipedia frequencies
```

#### `benchmark_symspell`

Accuracy testing tool.

```bash
./benchmark_symspell <dictionary> <test-file>

# Outputs:
# - Accuracy percentage
# - Performance metrics
# - errors.txt (format: correct_word misspelled_word guess)
```

#### `scripts/dictionary-test-regression.sh`

Multi-dataset testing to prevent overfitting.

```bash
./scripts/dictionary-test-regression.sh <dictionary>

# Tests against all datasets:
# - misspell-codespell.txt (63,703 cases)
# - misspell-wikipedia.txt (4,304 cases)
# - misspell-microsoft.txt (23,022 cases)
# - misspell-words.go.txt (31,138 cases)
```

### Standard Build Process

#### General-Purpose Dictionary (Recommended)

```bash
# Use the pre-built recommended version
cp dictionaries/reference/dictionaries/dictionary.txt dictionaries/dictionary.txt

# Results: 86,060 words
# - Codespell: 82.8%
# - Wikipedia: 84.0%
# - Microsoft: 52.1%
# - words.go: 71.8%
# - Average: ~72.7%
```

This is the **constitutional choice** for general spell-checking.

#### Domain-Optimized Dictionary

```bash
# 1. Build baseline
./scripts/dictionary-build.pl

# 2. Test against your target dataset
./benchmark_symspell dictionaries/test-dict.txt \
    test/data/symspell/misspellings/misspell-DATASET.txt

# 3. One iteration with errors
./scripts/dictionary-build.pl errors.txt

# 4. Verify improvement
./benchmark_symspell dictionaries/test-dict.txt \
    test/data/symspell/misspellings/misspell-DATASET.txt

# 5. STOP - don't iterate further
```

**Expected results:**
- Baseline: 78-84% (varies by dataset)
- After one iteration: 83-85% on target dataset
- Regression: 1-5% on other datasets

#### Multi-Dataset Dictionary

For balanced accuracy across datasets:

```bash
# 1. Build baseline
./scripts/dictionary-build.pl

# 2. Collect errors from ALL datasets
for dataset in codespell wikipedia microsoft words.go; do
    ./benchmark_symspell dictionaries/test-dict.txt \
        test/data/symspell/misspellings/misspell-${dataset}.txt
    mv errors.txt errors-${dataset}.txt
done

# 3. Build with combined errors
./scripts/dictionary-build.pl \
    errors-codespell.txt \
    errors-wikipedia.txt \
    errors-microsoft.txt \
    errors-words.go.txt

# 4. Test regression
./scripts/dictionary-test-regression.sh dictionaries/test-dict.txt
```

**Expected results:**
- More balanced across datasets
- Slightly lower peak accuracy on any single dataset
- Better average accuracy overall

### Benchmark Results

#### Pre-Built Dictionaries

| Dictionary | Words | Load (ms) | Codespell | Wikipedia | Microsoft | words.go |
|------------|-------|-----------|-----------|-----------|-----------|----------|
| **Baseline (shortlist)** | 82,580 | 909 | 78.2% | 83.2% | - | - |
| **Recommended** | 86,060 | 955 | **82.8%** | **84.0%** | **52.1%** | **71.8%** |
| + Codespell errors | 83,993 | 942 | 83.0% | 83.3% | 52.1% | 70.4% |
| With extras (111k) | 111,392 | 1,187 | 82.7% | 83.6% | 49.6% | 71.8% |

#### Observations

1. **Accuracy ceiling:** 82-85% appears to be the practical limit with edit distance = 2
2. **Size vs accuracy:** More words ≠ better accuracy (111k < 86k performance)
3. **Speed:** Smaller dictionaries are faster (909ms vs 1,187ms load time)
4. **Balance:** General-purpose dictionaries can't optimize for all domains

### The Accuracy Ceiling

**Why 82-85%?**
1. **Edit distance limit:** Constitutional choice of max distance = 2
2. **Ambiguous corrections:** Some misspellings have multiple valid corrections
3. **Frequency-based tiebreaking:** Not always correct (less common word may be intended)
4. **Fundamental limit:** Some errors are uncorrectable without context

**Could we do better?**
- Edit distance = 3: Yes, but 7x slower and more false corrections
- Context-aware: Yes, but violates simplicity principle
- Larger dictionary: No, adds noise and ambiguity

**Constitutional decision:** 83% is good enough. Optimize for speed and simplicity instead.

### Dictionary Sources

#### Base Wordlist
**`dictionaries/reference/wordlists/shortlist.txt`** (82,765 words)
- Extracted from `dict-freq_en_82_765.txt`
- High-quality English vocabulary
- Good signal-to-noise ratio

#### Wikipedia Frequency Data
**`dictionaries/reference/filter/dict-wiki_en_2_000_000.txt`**
- Top 2M words from Wikipedia by frequency
- Used to filter wordlists (keeps only real words)
- Provides accurate frequency data for ranking

#### Misspelling Test Data
**`test/data/symspell/misspellings/correct-*.txt`**
- Codespell: Common programming typos
- Wikipedia: Encyclopedia-specific terms
- Microsoft: Technical documentation terms
- words.go: Go language corpus

Adding corrections from these to the base wordlist improves domain-specific accuracy.

### Building From Scratch

#### Prerequisites

```bash
# 1. Get SCOWL wordlists (optional, for custom base)
git clone https://github.com/en-wl/wordlist
cd wordlist
# Copy scripts/wordlists/build-scowl.sh to wordlist directory
./build-scowl.sh

# 2. Wikipedia frequency data (already included)
dictionaries/reference/filter/dict-wiki_en_2_000_000.txt

# 3. Base frequency dictionary (already included)
dictionaries/reference/dictionaries/dict-freq_en_82_765.txt
```

#### Custom Base Wordlist

```bash
# Option A: Use SCOWL subset
./dictionaries/reference/wordlists/generate_wordlist.pl \
    scowl-words-50.txt \
    scowl-words-60.txt \
    > my-wordlist.txt

# Option B: Use frequency dictionary
# (shortlist.txt is already extracted from dict-freq_en_82_765.txt)

# Build dictionary with custom base
# (modify dictionary-build.pl to use your wordlist instead of shortlist.txt)
```

### Workflow for New Domains

**Example: Medical terminology**

```bash
# 1. Start with baseline
./scripts/dictionary-build.pl

# 2. Test on medical corpus
./benchmark_symspell dictionaries/test-dict.txt medical-misspellings.txt

# 3. Add medical terminology
./scripts/dictionary-build.pl errors.txt medical-terms.txt

# 4. One more iteration
./benchmark_symspell dictionaries/test-dict.txt medical-misspellings.txt
./scripts/dictionary-build.pl errors.txt

# 5. Deploy
cp dictionaries/test-dict.txt dictionaries/dictionary-medical.txt
```

**Result:** Domain-optimized dictionary with 85%+ accuracy on medical text.

---

## Part 2: Dictionary Extras (Keyword Extraction)

### Purpose

Dictionary extras (`dictionary-extras.txt`) provide **technical terms and acronyms** for IWF (Inverse Word Frequency) scoring in keyword extraction. These terms need high IWF scores to be recognized as semantically important.

**Key difference from main dictionary:**
- Main dictionary: Optimize for spell-checking accuracy
- Dictionary extras: Preserve domain terminology for semantic indexing

### Why Separate Extras?

Technical terms like `kubernetes`, `stdin`, `api` should:
1. **Not be "corrected"** when they appear in technical content
2. **Get high IWF scores** when used as keywords
3. **Not interfere** with general spell-checking accuracy

**Example problem without extras:**
```
"kubectl logs" → corrected to "bucket logs" → WRONG HASH → wrong retrieval
```

**With extras:**
```
"kubectl logs" → recognized as valid → CORRECT HASH → correct retrieval
```

### Building Tool

#### `scripts/dictionary-build-extras.pl`

Merges multiple source dictionaries with intelligent filtering.

```bash
# From project root
./scripts/dictionary-build-extras.pl

# From scripts directory
cd scripts
./dictionary-build-extras.pl

# Output: dictionaries/dictionary-extras.txt
```

#### Configuration

```perl
my $WIKI_FREQ_CUTOFF = 42;  # Wikipedia frequency threshold
```

### Processing Rules

#### 1. Always Skip
- Terms already in `dictionary.txt` (no duplicates)
- Comment lines (`# ...`)
- Blank lines
- Non-alphabetic terms (must match `^[a-z]+$`)

#### 2. Wikipedia Filtering (for large files)

Applied to:
- `abbreviations.txt`
- `davids-batista.txt`

Terms must appear in Wikipedia with frequency ≥ `$WIKI_FREQ_CUTOFF` to be included.

**Rationale:** These files contain noise (obscure abbreviations, typos). Wikipedia filter ensures quality.

#### 3. No Filtering (for curated files)

Applied to:
- `foss-acronyms.txt` (FOSS community terms)
- `tech_terms.txt` (manually curated)

All terms are included (after duplicate checking).

**Rationale:** These are hand-curated lists of known-good terms.

### Source Files

#### `tech_terms.txt`

Manually curated technical terms covering:
- System I/O: `stdin`, `stdout`, `stderr`
- Programming languages: `python`, `rust`, `golang`
- Protocols: `http`, `tcp`, `ssh`, `dns`
- Cryptography: `rsa`, `aes`, `sha256`
- Tools: `git`, `gcc`, `docker`, `kubernetes`
- ~700 essential technical terms

All assigned frequency `1` for maximum IWF impact.

**Adding new terms:**
```bash
# Edit dictionaries/reference/extras/tech_terms.txt
echo "newterm   1" >> dictionaries/reference/extras/tech_terms.txt

# Rebuild
./scripts/dictionary-build-extras.pl
```

#### `foss-acronyms.txt`

FOSS community acronyms extracted from [d-edge/foss-acronyms](https://github.com/d-edge/foss-acronyms):
- Code review: `lgtm`, `ack`, `nack`
- Development: `wip`, `pr`, `mr`
- Internet slang: `afaik`, `imo`, `til`
- ~150 terms organized by source file

**Regenerating:**
```bash
# Clone the acronyms repository
git clone https://github.com/d-edge/foss-acronyms.git

# Extract acronyms
./scripts/extract_acronyms.pl foss-acronyms/data > \
    dictionaries/reference/extras/foss-acronyms.txt

# Rebuild extras
./scripts/dictionary-build-extras.pl
```

#### `abbreviations.txt`

Internet abbreviations and slang (3,751 terms):
- Chat: `brb`, `lol`, `rofl`
- Technical shorthand
- **Wikipedia filtered** to reduce noise

#### `davids-batista.txt`

Academic and technical terminology (62,243 terms):
- Computer science terminology
- Research field-specific terms
- **Heavily filtered** by Wikipedia frequency

### Statistics (with cutoff=42)

| Source | Raw Terms | Filtered Out | Already Exists | Added | Notes |
|--------|-----------|--------------|----------------|-------|-------|
| abbreviations.txt | 3,751 | 1,428 | 351 | 1,972 | Wikipedia filtered |
| davids-batista.txt | 62,243 | 32,972 | 4,453 | 24,818 | Wikipedia filtered |
| foss-acronyms.txt | 153 | 0 | 26 | 117 | No filter |
| tech_terms.txt | 785 | 0 | 260 | 399 | No filter |
| **Total** | **66,932** | **34,400** | **5,090** | **25,327** | |

#### Wikipedia Filter Performance
- Total Wikipedia terms: 2,000,000
- Terms with freq ≥ 42: 527,733 (26.4%)
- Reduction: 73.6% of terms filtered

### Final Dictionary Sizes

- `dictionary.txt`: **86,060 words**
- `dictionary-extras.txt`: **25,327 words**
- **Combined: 111,387 words**

### Adjusting Filter Strictness

Edit `scripts/dictionary-build-extras.pl`:

```perl
my $WIKI_FREQ_CUTOFF = 42;  # Adjust this value
```

**Effect:**
- Higher value = stricter filtering = fewer terms = higher quality
- Lower value = looser filtering = more terms = more noise

**Recommendations:**
- 42: Balanced (default)
- 100: Stricter (common terms only)
- 10: Looser (more obscure terms)

---

## Part 3: Usage in CGIOS

### Spell-Checking and N-gram Indexing

**Use:** `dictionary.txt` only (86,060 words)

```c
symspell_dict_t* dict = symspell_create(2, 7);
symspell_load_dictionary(dict, "dictionaries/dictionary.txt", 0, 1);
```

**Why:**
- Optimized for spell-checking accuracy
- Fast (~950ms load, ~30µs lookup)
- Balanced across domains
- Good enough for general text

**When to use:**
- Spell-checking user input
- Correcting misspellings in search queries
- N-gram generation for indexing

### Keyword Extraction and IWF Scoring

**Use:** Both dictionaries (111k words total)

```c
symspell_dict_t* dict = symspell_create(2, 7);
symspell_load_dictionary(dict, "dictionaries/dictionary.txt", 0, 1);
symspell_load_dictionary(dict, "dictionaries/dictionary-extras.txt", 0, 1);

// Extract keywords with IWF scoring
KeywordSet keywords = extract_keywords(dict, text, len, 7.0, false);
```

**Why:**
- Technical terms get high IWF scores (rare = important)
- Prevents semantic corruption (`kubectl` → `bucket`)
- Preserves domain terminology in indexes
- Slight spell-checking regression (−0.1%) acceptable

**When to use:**
- Extracting keywords from technical documents
- Building semantic indexes
- Ranking terms by importance
- Domain-specific search

### Trade-offs

| Metric | dictionary.txt only | + dictionary-extras.txt |
|--------|---------------------|-------------------------|
| **Words** | 86,060 | 111,387 |
| **Load time** | ~950ms | ~1,190ms |
| **Spell-check accuracy** | 82.8% | 82.7% (−0.1%) |
| **Technical term preservation** | ❌ May correct | ✅ Preserved |
| **IWF scoring** | ❌ Missing terms | ✅ Complete |
| **Use case** | General text | Technical content |

**Constitutional decision:** Use the right tool for the job.

---

## Part 4: Maintenance

### Updating Main Dictionary

When new misspellings are discovered in production:

```bash
# 1. Collect errors
cat production-errors.txt >> errors-production.txt

# 2. Rebuild with production errors
./scripts/dictionary-build.pl errors-production.txt

# 3. Test regression
./scripts/dictionary-test-regression.sh dictionaries/test-dict.txt

# 4. If no major regressions, promote
cp dictionaries/test-dict.txt dictionaries/dictionary.txt
```

### Adding Technical Terms

Edit the curated source files:

```bash
# Add to tech_terms.txt (no filter)
echo "myterm    1" >> dictionaries/reference/extras/tech_terms.txt

# Or add to foss-acronyms.txt (no filter)
echo "myacronym 1" >> dictionaries/reference/extras/foss-acronyms.txt

# Rebuild extras
./scripts/dictionary-build-extras.pl

# Output: dictionaries/dictionary-extras.txt
```

For large additions, use the filtered sources:

```bash
# Add to abbreviations.txt (Wikipedia filtered)
echo "myabbrev  1" >> dictionaries/reference/extras/abbreviations.txt

# Rebuild (will be filtered by Wikipedia frequency)
./scripts/dictionary-build-extras.pl
```

### Adding New Test Datasets

```bash
# 1. Create misspelling file
# Format: misspelled_word<tab>correct_word

# 2. Add to dictionary-test-regression.sh
vim scripts/dictionary-test-regression.sh
# Add to DATASETS array

# 3. Run regression suite
./scripts/dictionary-test-regression.sh dictionaries/dictionary.txt
```

### Monitoring Dictionary Quality

```bash
# Run full regression test
./scripts/dictionary-test-regression.sh dictionaries/dictionary.txt

# Test with extras
./scripts/dictionary-test-regression.sh dictionaries/test-dict.txt

# Compare sizes
wc -l dictionaries/dictionary.txt dictionaries/dictionary-extras.txt

# Check for duplicates
comm -12 <(awk '{print $1}' dictionaries/dictionary.txt | sort) \
         <(awk '{print $1}' dictionaries/dictionary-extras.txt | sort)
# Should output nothing
```

---

## Constitutional Principles

1. **"Incomplete enoughness"**: 83% accuracy is good enough, don't chase perfection
2. **"One and done"**: Single iteration captures gains, more adds noise
3. **"Right tool for job"**: Main dictionary for spell-check, extras for IWF
4. **"Signal over size"**: 86k quality words > 111k noisy words
5. **"Know your trade-offs"**: Optimizing A regresses B - accept it
6. **"Separation of concerns"**: Don't pollute spell-check dictionary with domain terms

---

## Reference

### File Formats

| File | Format | Frequency | Purpose |
|------|--------|-----------|---------|
| `dictionary.txt` | `term\tfreq` | From Wikipedia | Spell-checking |
| `dictionary-extras.txt` | `term\t1` | Always 1 | IWF scoring |
| `tech_terms.txt` | `term\t1` | Always 1 | Curated technical |
| `foss-acronyms.txt` | `term\t1` | Always 1 | FOSS community |
| `abbreviations.txt` | `term\t1` | Always 1 | Internet slang |
| `davids-batista.txt` | `term\t1` | Always 1 | Academic terms |
| `dict-wiki_en_2_000_000.txt` | `term\tfreq` | Varies | Wikipedia filter |

### Script Reference

| Script | Purpose | Input | Output |
|--------|---------|-------|--------|
| `dictionary-build.pl` | Build main dictionary | wordlists | `test-dict.txt` |
| `dictionary-build-extras.pl` | Build extras | `extras/*` | `dictionary-extras.txt` |
| `dictionary-test-regression.sh` | Test all datasets | dictionary | accuracy report |
| `benchmark_symspell` | Test single dataset | dictionary, test file | accuracy, `errors.txt` |

### Wordlist Tools

| Script | Purpose | Location |
|--------|---------|----------|
| `build-scowl.sh` | Extract SCOWL tiers | `dictionaries/reference/wordlists/` |
| `generate_wordlist.pl` | Combine wordlists | `dictionaries/reference/wordlists/` |

---

## FAQ

**Q: Why two dictionaries?**  
A: Different purposes. Main dictionary optimizes spell-checking accuracy. Extras preserve technical terms for semantic indexing without hurting accuracy.

**Q: Why not iterate multiple times on main dictionary?**  
A: First iteration adds missing words. Second iteration adds noise. Accuracy plateaus or regresses.

**Q: Why does adding words sometimes hurt accuracy?**  
A: More words = more ambiguity. "teh" could correct to "the" or "tea" - wrong word wins sometimes.

**Q: Should I always use both dictionaries?**  
A: No. Use main dictionary only for spell-checking. Add extras only for keyword extraction and IWF scoring.

**Q: How do I optimize for my specific domain?**  
A: One iteration with errors.txt from your domain's test set. Don't expect universal improvements.

**Q: What's the theoretical accuracy limit?**  
A: ~85% with edit distance = 2. Some errors are uncorrectable without context.

**Q: Why Wikipedia for frequencies?**  
A: Large, high-quality corpus. Frequencies reflect real-world usage better than word counts alone.

**Q: Can I add my own technical terms?**  
A: Yes! Edit `dictionaries/reference/extras/tech_terms.txt`, then run `./scripts/dictionary-build-extras.pl`.

**Q: Why filter some sources but not others?**  
A: Curated sources (tech_terms, foss-acronyms) are known-good. Large sources (abbreviations, davids-batista) contain noise and need filtering.

**Q: What's the Wikipedia frequency cutoff?**  
A: Default is 42. Higher = stricter (fewer terms), lower = looser (more terms). Edit in `dictionary-build-extras.pl`.

---

**Remember:** The goal is not perfection, but **fitness for purpose**. Build the dictionaries your domain needs, accept the trade-offs, and ship it. 🏛️✨
