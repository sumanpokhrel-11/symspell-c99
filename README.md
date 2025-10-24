# SymSpell C99

**A blazingly fast spell-checking library** - Pure C99 implementation of Wolf Garbe's SymSpell algorithm.

[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)
[![Language](https://img.shields.io/badge/language-C99-blue.svg)](https://en.wikipedia.org/wiki/C99)
[![POSIX](https://img.shields.io/badge/POSIX-compliant-green.svg)](https://en.wikipedia.org/wiki/POSIX)

**By Suman Pokhrel**

---

## Why SymSpell?

SymSpell is **1 million times faster** than traditional spell-checkers. Instead of generating all possible corrections for a misspelling, it pre-computes deletions of dictionary words, enabling lightning-fast lookups.

Original algorithm by [Wolf Garbe](https://github.com/wolfgarbe/SymSpell)

---

## Features

- ⚡ **Fast**: ~30µs average lookup time
- 🎯 **Accurate**: 82-84% correction rate on standard test sets
- 🧹 **Clean**: Zero compiler warnings, ~700 lines of well-structured code
- 🌐 **Portable**: POSIX-compliant, works on any Unix system
- ✅ **Complete**: Includes tests, benchmarks, and dictionary tools
- 📦 **Self-contained**: No external dependencies

---

## Quick Start
```bash
make
./test_symspell dictionaries/dictionary.txt
```

Interactive mode:
```
Enter words to correct (or 'quit' to exit):
recieve
  Suggestions:
    receive (distance=1, iwf=7.234567, prob=0.001234, freq=234567)
```

---

## Usage
```c
#include "symspell.h"

// Create dictionary with max edit distance=2, prefix length=7
symspell_dict_t* dict = symspell_create(2, 7);

// Load dictionary (86k words optimized for spell-checking)
symspell_load_dictionary(dict, "dictionaries/dictionary.txt", 0, 1);

// Lookup suggestions
symspell_suggestion_t suggestions[5];
int count = symspell_lookup(dict, "speling", 2, suggestions, 5);

// Print results
for (int i = 0; i < count; i++) {
    printf("%s (distance=%d)\n", 
           suggestions[i].term, 
           suggestions[i].distance);
}

// Cleanup
symspell_destroy(dict);
```

---

## Performance
```
Average lookup time:     ~30µs
Correction accuracy:     82-84%
Dictionary size:         86,060 words
Memory usage:            ~45MB (with deletes index)
```

Tested on Apple M4, comparable results on x86.

---

## Building

**Requirements:**
- C99 compiler (GCC, Clang)
- Make
- POSIX-compliant system (Linux, macOS, BSD)

**Compile:**
```bash
make              # Build test and benchmark programs
make test         # Build and run tests
make benchmark    # Build and run benchmark
make clean        # Remove built programs
```

**Manual compilation:**
```bash
gcc -std=c99 -O2 -Iinclude test/test_symspell.c src/symspell.c -o test_symspell
```

---

## Dictionaries

**Main dictionary** (`dictionaries/dictionary.txt`): 86,060 words
- Optimized for spell-checking accuracy (82-84%)
- Balanced across multiple test sets
- Recommended for general use

**Extras dictionary** (`dictionaries/dictionary-extras.txt`): 25,327 words
- Technical terms and acronyms
- For keyword extraction and IWF scoring
- DO NOT use for spell-checking (reduces accuracy)

**Building custom dictionaries:**
See `dictionaries/reference/` for source materials and tools.
Full documentation in `docs/DICTIONARY.md`.

---

## Project Structure
```
symspell-c99/
├── src/
│   └── symspell.c          # Implementation (~700 lines)
├── include/
│   ├── symspell.h          # Public API
│   ├── hash.h              # Hash table implementation
│   ├── xxh3.h              # xxHash for fast hashing
│   └── posix.h             # POSIX compatibility layer
├── test/
│   ├── test_symspell.c     # Interactive test program
│   ├── benchmark_symspell.c # Performance benchmarks
│   └── data/               # Test datasets
├── dictionaries/
│   ├── dictionary.txt      # Main 86k word dictionary
│   └── reference/          # Dictionary building tools
└── docs/
    ├── SYMSPELL.md         # Algorithm details
    └── DICTIONARY.md       # Dictionary customization
```

---

## Documentation

- **[SYMSPELL.md](docs/SYMSPELL.md)** - Algorithm explanation and implementation details
- **[DICTIONARY.md](docs/DICTIONARY.md)** - Dictionary building and customization guide
- **[dictionaries/README.md](dictionaries/README.md)** - Dictionary overview

---

## Algorithm

SymSpell uses **Symmetric Delete** spelling correction:

1. **Pre-computation**: Generate all deletions up to edit distance N for each dictionary word
2. **Lookup**: Generate deletions of the input word
3. **Match**: Find dictionary words that share these deletions
4. **Rank**: Sort by edit distance and word frequency

This trades memory for speed - pre-computing deletions makes lookups extremely fast.

---

## Testing

Run the test program:
```bash
./test_symspell dictionaries/dictionary.txt
```

Run benchmarks:
```bash
./benchmark_symspell dictionaries/dictionary.txt test/data/symspell/misspellings/misspell-codespell.txt
```

---

## License

MIT License - See [LICENSE](LICENSE) file for details.

This implementation is independent but follows the algorithm described in:
- [SymSpell](https://github.com/wolfgarbe/SymSpell) by Wolf Garbe (MIT License)

---

## Credits

- **Algorithm**: [Wolf Garbe](https://github.com/wolfgarbe) - Creator of SymSpell
- **Implementation**: Suman Pokhrel - Pure C99 port

Special thanks to Wolf Garbe for creating and open-sourcing this brilliant algorithm.

---

## Author

**Suman Pokhrel**

- GitHub: [@sumanpokhrel-11](https://github.com/sumanpokhrel-11)
- Email: 11pksuman@gmail.com

---

## Contributing

This is the **first pure C99 implementation** of SymSpell. Contributions welcome:

- Bug reports and fixes
- Performance improvements
- Dictionary enhancements
- Documentation improvements
- Platform-specific optimizations

Please open an issue or pull request on GitHub!

---

## See Also

- [Original SymSpell (C#)](https://github.com/wolfgarbe/SymSpell) - Wolf Garbe's original implementation
- [SymSpell Ports](https://github.com/wolfgarbe/SymSpell#ports) - Implementations in other languages

---

*This is the first and only pure C99 implementation of SymSpell, designed for performance, portability, and clean code.*
