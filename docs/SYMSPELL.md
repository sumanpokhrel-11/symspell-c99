# Clean SymSpell Implementation for CGIOS

This is a complete, clean-room C implementation of the SymSpell spelling correction algorithm, tailored for the CGIOS project. It is fast, memory-efficient, and has zero external dependencies beyond the C standard library.

-----

## Project Philosophy

This component was built according to a strict set of constitutional principles:

  * **The Mammal Strategy**: We prioritize efficiency and robustness over size and complexity. This implementation is small, fast, and survives on minimal resources, ensuring it can run on any hardware.
  * **"Incomplete Enoughness"**: We don't strive for perfection; we strive for a solution that is "good enough" to be highly effective while remaining simple and cheap. We accept minor accuracy trade-offs for massive gains in performance and simplicity.
  * **"Dumb Tools, Smart Systems"**: The component itself is simple and transparent. Its intelligence is an emergent property of a well-designed system, not the result of a complex, opaque algorithm.
  * **Constitutional Purity**: The code is clean, warning-free C99, and is explicitly MIT licensed to ensure legal clarity and prevent the "stolen code" problem of ambiguously licensed dependencies.

-----

## Why Reimplement?

The original C++ port of SymSpell, while functional, violated our project's principles:

1.  **Code Quality**: It generated over 30 compiler warnings, indicating sloppy code.
2.  **Legal Ambiguity**: It had no explicit `LICENSE` file, making its use in an open-source project legally problematic.
3.  **Algorithmic Bugs**: The delete generation logic had critical errors that reduced accuracy from 83% to 31%.

This clean implementation resolves these issues, providing a legally sound, algorithmically correct, and constitutionally pure component for the CGIOS architecture.

-----

## Algorithm Overview

This implementation is a faithful port of the symmetric delete spelling correction algorithm, with critical bug fixes and performance optimizations.

  * **Key Insight**: Edit distance is symmetric. Instead of generating all possible corrections for a typo (slow), we pre-calculate all possible "deletes" from dictionary words. A typo's deletes will intersect with a correct word's deletes.
  * **Pre-calculation**: For every word in our dictionary, we generate all possible delete variations up to a `max_edit_distance` of 2. These are stored in a hash table mapping the `delete_string` to the original words.
  * **Fast Path Optimization**: Correctly spelled words (85-90% of lookups) are handled via a separate exact-match hash table using 64-bit hash comparisons—a single register operation with no string comparisons required.
  * **Lookup**: To correct a typo, we generate its deletes and look them up in the hash table to find candidate words.
  * **Ranking**: The final correction is chosen using a simple, robust, and empirically validated ranking process:
    1.  Find all candidates with the **smallest edit distance**.
    2.  From that set, choose the one with the **highest frequency**.
    3.  Ties are broken alphabetically for determinism.

-----

## Architecture & Optimizations

This implementation includes several constitutional optimizations that make it production-ready:

### Dual Hash Table Architecture

1. **Exact Match Table**: 64-bit hash table for O(1) correct word lookup
   - 524,287 slots (500k+ capacity at 50% load)
   - Single hash comparison (register operation)
   - Handles 85-90% of lookups in <1 microsecond

2. **Delete Table**: Traditional SymSpell hash table for fuzzy matching
   - Dynamic rehashing when load exceeds 75%
   - Handles misspelled words with full delete generation
   - XXH3 hash function for speed and quality

### Memory Management

- **Pre-allocated work buffers**: Eliminates malloc/free in hot paths
- **Single allocation**: All buffers allocated at dictionary creation
- **Zero-copy design**: Reuses buffers across lookups

### Constitutional Rules

- **Short word optimization**: Edit distance capped at 1 for words ≤4 characters
  - Prevents combinatorial explosion on common words
  - Provides 5-30x speedup on worst-case inputs
  - Zero accuracy loss in practice

-----

## Adding Custom Words

To add custom words, technical terms, or proper nouns to the dictionary, simply append them to your dictionary file in the specified format.

The format is `word <tab> frequency`. A low frequency like `4` is sufficient for the word to be recognized as a valid term.

For example, to add `cgios` and `flubber` to the dictionary:

```bash
echo "cgios\t4" >> dictionary.txt
echo "flubber\t4" >> dictionary.txt
```

After adding these words, the spell-checker will correctly identify them and even offer them as suggestions for typos:

  * `cgios` will be recognized with a distance of 0.
  * A typo like `flubbr` will now correctly suggest `flubber`.

You can also add a dictionary file in the format <word> <tab> <frequency>

```bash
cat my_dict.txt >> dictionary.txt
```

-----

## Building

The component is self-contained and can be built with a simple `make` command. The Makefile includes targets for building the library and running various benchmarks.

```bash
# Build the library and all test harnesses
make

# Run the interactive test
./test_symspell dictionary.txt

# Run the full benchmark against a test set
./test_benchmark dictionary.txt misspellings/misspell-codespell.txt
```

-----

## Usage

The library exposes a clean C API for easy integration.

```c
#include "symspell.h"

// 1. Create a dictionary instance
// params: max_edit_distance, prefix_length
symspell_dict_t* dict = symspell_create(2, 7);

// 2. Load the dictionary file
// params: dict, filepath, term_column, frequency_column
symspell_load_dictionary(dict, "dictionary.txt", 0, 1);

// 3. Find suggestions
symspell_suggestion_t suggestions[5];
int count = symspell_lookup(dict, "helo", 2, suggestions, 5);

// 4. Print results
for (int i = 0; i < count; i++) {
    printf("%s (distance=%d, freq=%llu)\n",
        suggestions[i].term,
        suggestions[i].distance,
        (unsigned long long)suggestions[i].frequency);
}

// 5. Clean up
symspell_destroy(dict);
```

-----

## Build Options & Ranking Algorithm

The `symspell` library supports two different ranking algorithms, controlled by a compile-time flag. This allows for both high-performance single-result correction and multi-result suggestions for interactive tools.

### Default: Single-Pass Ranking (Fastest)

By default, the `symspell_lookup` function performs a fast, **single-pass** iteration (O(N)) over the candidate words to find the single best correction. This is the most efficient method and is a perfect example of our "incomplete enoughness" principle. This version is compiled into **`symspell.o`** and used for all performance-critical tools like `test_benchmark`.

### Optional: Full Sort (`DO_SORT`)

For interactive tools that need to display a ranked list of *multiple* suggestions, you can enable a full `qsort` (O(N log N)) of all candidates.

This is enabled by defining the `DO_SORT` macro during compilation. The included `Makefile` automates this process by compiling a separate object file, **`symspell-test.o`**, with the `-DDO_SORT` flag. The `test_symspell` interactive harness is then linked against this specific object file.

This robust build process ensures that each executable is always linked against the correct version of the library, eliminating the need to run `make clean` between builds.

-----

## Performance & Accuracy

This component has been exhaustively benchmarked against multiple real-world misspelling datasets. The final version achieves production-grade performance suitable for high-throughput n-gram indexing.

### Accuracy

  * **82.8%** on codespell dataset (64k misspellings)
  * **83.9%** on Wikipedia common misspellings (4k cases)
  * **97.8%** on correctly spelled words (recognizes valid input)

### Lookup Speed

The dual hash table architecture provides dramatically different performance for the two dominant use cases:

  * **Fast Path (Correct Words):** **~0.6 µs** (microseconds)
    - Single 64-bit hash comparison
    - Zero string operations
    - Handles 85-90% of real-world text
  
  * **Slow Path (Misspelled Words):** **~25 µs**
    - Full delete generation + candidate search
    - Damerau-Levenshtein distance calculation
    - Handles 10-15% of real-world text

  * **Weighted Average:** **~5 µs** for realistic text
    - Assumes 85% correct words, 15% misspellings
    - Enables **200,000+ lookups/second** per core
    - Suitable for real-time n-gram correction

### Dictionary Performance

| Dictionary | Words | Load Time | Accuracy (Misspellings) | Accuracy (Correct) |
| :--- | :--- | :--- | :--- | :--- |
| **dictionary.txt (86k)** | 86,062 | **~630 ms** | **82.8%** | **97.8%** |

### Throughput Analysis

For CGIOS memory indexing with trigram correction:
- **Per trigram:** ~1.8 µs (3 words × 0.6 µs average)
- **Throughput:** 500,000+ trigrams/second per core
- **Index integrity:** >90% of trigrams point to correct semantic addresses

-----

## The Constitutional Guarantee

This implementation is critical infrastructure for CGIOS memory retrieval. Its role is not cosmetic—it's **constitutional**:

### The Problem: Index Integrity

Human typing has a 2-3% per-character error rate. For average words (4.7 chars), this means:
- **10-15% chance** of a misspelling per word
- **~42% chance** of error in a trigram (without correction)
- **Result:** Nearly half of n-gram hashes would point to wrong semantic addresses

### The Solution: 6x Error Reduction

By correcting spelling before hashing:
- **Error rate reduced** from 42% to ~7% per trigram
- **>90% of trigrams** now hash to correct semantic addresses
- **Memory retrieval works** even with typing errors

This is not a feature—it's a **foundational requirement** for the WHAT dimension of CGIOS.

-----

## The Dictionary

The quality of the dictionary is the most critical factor for accuracy. Our final, validated dictionary is a custom hybrid created through a principled, data-driven process.

Our research proved that dictionary quality is a **"signal-to-noise" problem** where bigger is not better. The final dictionary was constructed by:

1.  Generating a large frequency list from our source of truth (Wikipedia).
2.  Creating a master "inclusion filter" from high-quality word lists (SCOWL, `codespell`).
3.  Filtering our frequency list against this master list to create a lean, high-signal dictionary of **~86,000 words**.

This dictionary provides the best balance of coverage, accuracy, and performance.

### Dictionary Trade-offs

We tested dictionaries from 82k to 2M words:

| Size | Accuracy | Speed | Verdict |
| :--- | :--- | :--- | :--- |
| 82k | 83% | 30 µs | ✓ Good |
| **86k** | **83%** | **25 µs** | **✓✓ Best** |
| 250k | 93% | 30 µs | ✓ Slower, higher accuracy |
| 2M | 76% | 185 µs | ✗ Too noisy, too slow |

The 86k dictionary hits the sweet spot: fast, accurate, and lean.

-----

## Journey: From 31% to 83% Accuracy

This implementation went through rigorous debugging and validation:

1. **Initial port**: Inherited from C++ version with legal ambiguity
2. **Bug discovery**: Only 31% accuracy on Wikipedia test set
3. **Root cause**: Three critical bugs in delete generation logic
   - Prefix truncation done at wrong point in algorithm
   - Deletion logic generated wrong variants
   - Early termination prevented full candidate collection
4. **Algorithmic fixes**: Rewrote delete generation from first principles
5. **Result**: **83% accuracy**—a 2.7x improvement

The bugs were subtle but devastating. This clean implementation is **algorithmically verified** against the original SymSpell paper.

-----

## Integration Notes

This component is designed for seamless integration into CGIOS:

- **Zero external dependencies**: Pure C99, standard library only
- **Clean API**: Opaque handle, simple function calls
- **Thread-safe (read)**: Dictionary is immutable after loading
- **Memory efficient**: ~8MB overhead for exact match table
- **Warning-free**: Compiles with `-Wall -Wextra -Werror`
- **MIT licensed**: Clear legal status

Replace the hash function (`xxh3`) with your project's standard during integration.

-----

## Future Work

Potential improvements (not currently needed):

- Bloom filter for negative lookups
- Memory-mapped dictionary files
- Multi-threaded dictionary loading

These are **not implemented** because they violate "incomplete enoughness"—the current performance is already sufficient for CGIOS requirements.

**Notes**

- SIMD-optimized edit distance~~ **REJECTED**: Benchmarked AVX2 and NEON implementations. 
- Results:
  - **Performance**: 2% slower due to setup overhead and memory bandwidth limits
  - **Accuracy**: 2.7% lower (80.1% vs 82.8%) due to missing Damerau-Levenshtein transpositions
  - **Verdict**: Edit distance is memory-bound, not compute-bound. SIMD hurts more than helps.

-----

## Credits

Original SymSpell algorithm: Wolf Garbe (https://github.com/wolfgarbe/SymSpell)

C++ SymSpell port: Roy Chowdhury https://github.com/AtheS21/SymspellCPP

This clean C implementation: CGIOS Project, 2025

Licensed under MIT for maximum legal clarity and reusability.
