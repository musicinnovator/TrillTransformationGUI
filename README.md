# TrillTransformationGUI

**TrillTransformationGUI** is a cross-platform application for transforming musical note data (such as from MIDI analyses) into trill variants in Baroque and Classical styles. It offers flexible percentage-based transformation, random or user-selected trill variant pools, and outputs both transformed text files and auto-generated MIDI files. It features a GUI (platform-specific, minimal dependencies), and is designed for musicians, musicologists, and algorithmic composition researchers.

---

## Features

- **Trill Transformation**: Converts notes labeled with certain musical functions (e.g. RLN, CS, DN) into trill sequences, based on trill variant codes.
- **Variant Selection**: Choose trill variants from a randomly generated pool, select multiple variants, or let the application choose randomly.
- **Percentage Control**: Specify what percentage of eligible notes should be transformed.
- **Output**: Writes transformed data to a text file (with trill sequence details) and generates a MIDI file using transformed notes.
- **Statistics & Summary**: Reports on how many notes were transformed, which variants were used, and generates a summary for the user.
- **Cross-platform**: Runs on Windows and Linux, auto-detecting the OS for basic GUI integration.

---

## Quick Start

1. **Prepare Input File**  
   Your input file should be a text file with lines containing:
   ```
   Track NoteName Duration Label
   ```
   Example:
   ```
   1 C4 480 RLN
   1 D4 480 DN
   1 E4 240 CS
   ```

2. **Run the Application**  
   Launch TrillTransformationGUI executable.  
   - On Windows: Double-click the `.exe` or run from command line.
   - On Linux: Execute from terminal.

3. **Choose Settings in the GUI**  
   - **Input File**: Select your input text file.
   - **Output File**: Specify output file path.
   - **Transformation Percentage**: Set e.g. `50` for 50% eligible notes transformed.
   - **Trill Variant Pool**: Choose from a random selection, type codes, or select RANDOM for full randomization.
   - **Generate MIDI**: Optionally, create a MIDI file from the transformed notes.

4. **Click "Process"**  
   The application processes the file, transforms eligible notes, and outputs results.

---

## Trill Variant Codes

Variants use codes such as:

- `BTrRs1`: Baroque Short Regular Trill (Major 2nd)
- `CTrRn5`: Classical Normal Regular Trill (Minor 2nd)
- `BTrDl1`: Baroque Descending Long Trill (Major 2nd)

See the source (`TrillTransformation.cpp`) for a full list.

---

## Example Usage

### Example 1: Transforming Notes with Random Trill Variants

**Input:**
```
1 C4 480 RLN
1 D4 480 DN
1 E4 240 CS
2 G4 240 FM
2 A4 480 RLN
```

**Settings:**
- Transformation Percentage: 60
- Variant Selection: RANDOM

**Output File (excerpt):**
```
Track      Note       Duration            Label               Trill_Variant            
---------------------------------------------------------------------------------
1         D4        120                 RLN                 BTrRs1                  
1         C4        120                 RLN                 BTrRs1                  
1         D4        120                 RLN                 BTrRs1                  
1         C4        120                 RLN                 BTrRs1                  
1         D4        120                 DN                  CTrRn5                  
...
```

### Example 2: User Chooses Specific Variants

**Variant Selection:** `CTrRs1 CTrRn5` (Classical style only)

**Output File:**  
Transformed notes will alternate between the chosen variants.

**Summary Output:**
```
Transformation Statistics:
Total eligible notes found: 4
Notes transformed: 2
Actual transformation percentage: 50.0%

Variants used (2 total):
  CTrRs1: 1 times
  CTrRn5: 1 times

Processing complete. Transformed results written to output.txt
```

### Example 3: MIDI Generation

After transformation, the app can generate a MIDI file (`output.mid`) matching the transformed note sequences. Each trill note is mapped to its proper MIDI number and timing.

---

## Building & Dependencies

- **C++ Standard**: C++11 or newer recommended.
- **Windows**: Uses built-in windows.h, commdlg.h, commctrl.h for GUI.
- **Linux**: Uses X11 for basic GUI.
- **No external dependencies** required for core functionality.

---

## Advanced

- **Label Eligibility**: Only notes with certain labels (e.g., RLN, DN, CS) are transformed.
- **Trill Transformation Logic**: See `applyTrill()` and its helpers in `TrillTransformation.cpp` for trill sequence generation algorithms.

---

## License

(C) 2025 musicinnovator  
For academic, research, and personal use.

---

## Contact

Questions, feature requests, or bug reports:  
Open an issue on [GitHub](https://github.com/musicinnovator/TrillTransformationGUI)
