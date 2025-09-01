// Trill Transformation (C) 2025
// Version with percentage control, multiple choice variant selection, and GUI without dependencies
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <map>
#include <algorithm>
#include <random>
#include <chrono>
#include <ctime>
#include <iomanip>
#include <stdexcept>
#include <cstdlib>
#include <cstring>
#include <memory>
#include <set>

// Platform detection
#if defined(_WIN32) || defined(_WIN64)
    #define PLATFORM_WINDOWS
    #include <windows.h>
    #include <commdlg.h>
    #include <commctrl.h>
    #pragma comment(lib, "comctl32.lib")
#elif defined(__linux__) || defined(__unix__)
    #define PLATFORM_LINUX
    #include <X11/Xlib.h>
    #include <X11/Xutil.h>
    #include <X11/keysym.h>
    #include <unistd.h>
    #include <sys/types.h>
    #include <pwd.h>
#else
    #error "Unsupported platform"
#endif

// Helper to get note name (from MIDI number)
std::string getNoteName(int noteNumber) {
    static const std::string noteNames[] = {
        "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"
    };
    int octave = noteNumber / 12 - 1;
    int noteIndex = noteNumber % 12;
    return noteNames[noteIndex] + std::to_string(octave);
}

// Helper to get MIDI number from note name
int getNoteNumber(const std::string& noteName) {
    static const std::string noteNames[] = {
        "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"
    };

    // Extract note and octave
    std::string baseNote = noteName.substr(0, noteName.size() - 1);
    int octave = std::stoi(noteName.substr(noteName.size() - 1));

    // Find base note index
    auto it = std::find(std::begin(noteNames), std::end(noteNames), baseNote);
    if (it == std::end(noteNames)) {
        throw std::invalid_argument("Invalid note name: " + noteName);
    }
    int noteIndex = std::distance(std::begin(noteNames), it);

    return (octave + 1) * 12 + noteIndex;
}

// Enum for TimeMeter
enum TimeMeter {
    DUPLE,
    TRIPLE
};

void handleMeterShortReg(std::vector<std::pair<int, int>>& EmbRet, int p1, int p2, int p3, int pi, int durPi, TimeMeter meter) {
    if (meter == DUPLE) {
        int segment = durPi / 4;
        EmbRet.push_back({p1, segment});
        EmbRet.push_back({p2, segment});
        EmbRet.push_back({p1, segment});
        EmbRet.push_back({p2, durPi - 3 * segment}); // Use remaining duration
    } else if (meter == TRIPLE) {
        int segment = durPi / 6;
        for (int i = 0; i < 5; ++i) { // Alternate p1 and p2
            EmbRet.push_back({i % 2 == 0 ? p1 : p2, segment});
        }
        EmbRet.push_back({p2, durPi - 5 * segment}); // Remaining duration
    }
}

void handleMeterNormalReg(std::vector<std::pair<int, int>>& EmbRet, int p1, int p2, int p3, int p4, int p5, int pi, int durPi, TimeMeter meter) {
    int segment = durPi / 8;
    if (meter == DUPLE) {
        for (int i = 0; i < 6; ++i) {
            EmbRet.push_back({i % 2 == 0 ? p1 : p2, segment});
        }
        EmbRet.push_back({p2, durPi - 6 * segment}); // Remaining duration
    } else if (meter == TRIPLE) {
        for (int i = 0; i < 6; ++i) {
            EmbRet.push_back({i % 2 == 0 ? p1 : p2, segment});
        }
        EmbRet.push_back({p2, durPi - 6 * segment}); // Remaining duration
    }
}

void handleMeterLongReg(std::vector<std::pair<int, int>>& EmbRet, int p1, int p2, int p3, int p4, int p5, int p6, int p7, int pi, int durPi, TimeMeter meter) {
    if (meter == DUPLE || meter == TRIPLE) {
        int segment = durPi / 8;
        for (int i = 0; i < 7; ++i) {
            EmbRet.push_back({i % 2 == 0 ? p1 : p2, segment});
        }
        EmbRet.push_back({p2, durPi - 7 * segment}); // Remaining duration
    }
}

void handleMeterDelayedNormal(std::vector<std::pair<int, int>>& EmbRet, int p1, int p2, int p3, int p4, int p5, int pi, int durPi, TimeMeter meter) {
    if (meter == DUPLE) {
        int segmentA = durPi / 4;
        int segmentB = durPi / 8;
        EmbRet.push_back({p1, segmentA});
        EmbRet.push_back({p1, segmentB});
        EmbRet.push_back({p2, segmentB});
        EmbRet.push_back({p1, segmentB});
        EmbRet.push_back({p2, segmentB});
        EmbRet.push_back({p2, durPi - (segmentA + 4 * segmentB)}); // Remaining duration
    } else if (meter == TRIPLE) {
        int segmentA = durPi / 8;
        EmbRet.push_back({p1, segmentA * 2}); // p1 at 1/4
        for (int i = 0; i < 4; i++) {
            EmbRet.push_back({i % 2 == 0 ? p2 : p1, segmentA});
        }
        EmbRet.push_back({p2, durPi - (segmentA * 6)}); // Remaining duration
    }
}

void handleMeterDelayedLong(std::vector<std::pair<int, int>>& EmbRet, int p1, int p2, int p3, int p4, int p5, int pi, int durPi, TimeMeter meter) {
    if (meter == DUPLE || meter == TRIPLE) {
        int segment = durPi / 8;
        EmbRet.push_back({p1, segment * 2}); // 1/4 duration
        for (int i = 0; i < 5; ++i) {
            EmbRet.push_back({i % 2 == 0 ? p2 : p1, segment});
        }
        EmbRet.push_back({p2, durPi - 7 * segment}); // Remaining duration
    }
}

void handleMeterAscendingShort(std::vector<std::pair<int, int>>& EmbRet, int p1, int p2, int p3, int p4, int p5, int p6, int durPi, TimeMeter meter) {
    int segment = durPi / 8;
    if (meter == DUPLE) {
        for (int i = 0; i < 4; ++i) {
            EmbRet.push_back({i % 2 == 0 ? p1 : p2, segment});
        }
        EmbRet.push_back({p1, durPi / 4});
        EmbRet.push_back({p2, durPi - (4 * segment + durPi / 4)}); // Remaining duration
    } else if (meter == TRIPLE) {
        for (int i = 0; i < 4; ++i) {
            EmbRet.push_back({i % 2 == 0 ? p1 : p2, segment});
        }
        EmbRet.push_back({p1, durPi / 6});
        EmbRet.push_back({p2, durPi - (4 * segment + durPi / 6)}); // Remaining duration
    }
}

void handleMeterAscendingNormal(std::vector<std::pair<int, int>>& EmbRet, int p1, int p2, int p3, int p4, int p5, int p6, int p7, int p8, int durPi, TimeMeter meter) {
    if (meter == DUPLE) {
        int segment = durPi / 8;
        for (int i = 0; i < 7; ++i) {
            EmbRet.push_back({i % 2 == 0 ? p1 : p2, segment}); // Alternate p1 and p2
        }
        EmbRet.push_back({p2, durPi - 7 * segment}); // Remaining duration
    } else if (meter == TRIPLE) {
        int segment = durPi / 8;
        for (int i = 0; i < 7; ++i) {
            EmbRet.push_back({i % 2 == 0 ? p1 : p2, segment}); // Alternate p1 and p2
        }
        EmbRet.push_back({p2, durPi - 7 * segment}); // Remaining duration
    }
}

void handleMeterTerminalShort(std::vector<std::pair<int, int>>& EmbRet, int p1, int p2, int p3, int pi, int durPi, TimeMeter meter) {
    if (meter == DUPLE) {
        int segment = durPi / 4;
        EmbRet.push_back({p1, segment});
        EmbRet.push_back({p2, segment});
        EmbRet.push_back({p1, segment});
        EmbRet.push_back({p2, durPi - 3 * segment}); // Remaining duration
    } else if (meter == TRIPLE) {
        int segment = durPi / 6;
        for (int i = 0; i < 5; ++i) {
            EmbRet.push_back({i % 2 == 0 ? p1 : p2, segment});
        }
        EmbRet.push_back({p2, durPi - 5 * segment}); // Remaining duration
    }
}

void handleMeterAscendingLong(std::vector<std::pair<int, int>>& EmbRet, int p1, int p2, int p3, int p4, int p5, int p6, int p7, int p8, int p9, int p10, int p11, int p12, int durPi, TimeMeter meter) {
    if (meter == DUPLE) {
        int segment = durPi / 16;
        for (int i = 0; i < 15; ++i) {
            EmbRet.push_back({i % 2 == 0 ? p1 : p2, segment}); // Alternate between p1 and p2 for 15 segments
        }
        EmbRet.push_back({p2, durPi - 15 * segment}); // Remaining time for the last segment
    } else if (meter == TRIPLE) {
        int segment = durPi / 12;
        for (int i = 0; i < 11; ++i) {
            EmbRet.push_back({i % 2 == 0 ? p1 : p2, segment}); // Alternate between p1 and p2 for 11 segments
        }
        EmbRet.push_back({p2, durPi - 11 * segment}); // Remaining time for the last segment
    }
}

void handleMeterTerminalNormal(std::vector<std::pair<int, int>>& EmbRet, int p1, int p2, int p3, int pi, int p4, int p5, int p6, int p7, int durPi, TimeMeter meter) {
    if (meter == DUPLE) {
        int segment = durPi / 8;
        for (int i = 0; i < 7; ++i) {
            EmbRet.push_back({i % 2 == 0 ? p1 : p2, segment});
        }
        EmbRet.push_back({p2, durPi - 7 * segment});
    } else if (meter == TRIPLE) {
        int segment = durPi / 12;
        for (int i = 0; i < 11; ++i) { // Alternate p1 and p2
            EmbRet.push_back({i % 2 == 0 ? p1 : p2, segment});
        }
        EmbRet.push_back({p2, durPi - 11 * segment}); // Remaining duration
    }
}

void handleMeterTerminalLong(std::vector<std::pair<int, int>>& EmbRet, int p1, int p2, int p3, int p4, int p5, int p6, int p7, int p8, int p9, int p10, int p11, int p12, int p13, int p14, int p15, int p16, int durPi, TimeMeter meter) {
    if (meter == DUPLE) {
        int segment = durPi / 16;
        for (int i = 0; i < 15; ++i) {
            EmbRet.push_back({i % 2 == 0 ? p1 : p2, segment}); // Alternate p1 and p2
        }
        EmbRet.push_back({p2, durPi - 15 * segment}); // Remaining duration
    } else if (meter == TRIPLE) {
        int segment = durPi / 12;
        for (int i = 0; i < 11; ++i) {
            EmbRet.push_back({i % 2 == 0 ? p1 : p2, segment}); // Alternate p1 and p2
        }
        EmbRet.push_back({p2, durPi - 11 * segment}); // Remaining duration
    }
}

// Main function for trill transformation
std::vector<std::pair<int, int>> applyTrill(int pi, int durPi, TimeMeter meter, const std::string& variant) {
    if (durPi <= 0) {
        throw std::invalid_argument("Duration (durPi) must be greater than 0");
    }
    if (meter != DUPLE && meter != TRIPLE) {
        throw std::invalid_argument("Invalid TimeMeter");
    }

    std::vector<std::pair<int, int>> EmbRet;
    
    // Short Reg Trills - Baroque and Classical
    if (variant == "BTrRs1") {
        handleMeterShortReg(EmbRet, pi + 2, pi, pi + 2, pi, durPi, meter);
    } else if (variant == "BTrRs5") {
        handleMeterShortReg(EmbRet, pi + 1, pi, pi + 1, pi, durPi, meter);
    } else if (variant == "CTrRs1") {
        handleMeterShortReg(EmbRet, pi, pi + 2, pi, pi + 2, durPi, meter);
    } else if (variant == "CTrRs5") {
        handleMeterShortReg(EmbRet, pi, pi + 1, pi, pi + 1, durPi, meter);
    }
    
    // Normal Reg Trills - Baroque and Classical
    else if (variant == "BTrRn1") {
        handleMeterNormalReg(EmbRet, pi + 2, pi, pi + 2, pi, pi + 2, pi, durPi, meter);
    } else if (variant == "BTrRn5") {
        handleMeterNormalReg(EmbRet, pi + 1, pi, pi + 1, pi, pi + 1, pi, durPi, meter);
    } else if (variant == "CTrRn1") {
        handleMeterNormalReg(EmbRet, pi, pi + 5, pi, pi + 5, pi, pi + 5, durPi, meter);
    } else if (variant == "CTrRn5") {
        handleMeterNormalReg(EmbRet, pi, pi + 1, pi, pi + 1, pi, pi + 1, durPi, meter);
    }
    
    // Long Reg Trills - Baroque and Classical
    else if (variant == "BTrRl1") {
        handleMeterLongReg(EmbRet, pi + 2, pi, pi + 2, pi, pi + 2, pi, pi + 2, pi, durPi, meter);
    } else if (variant == "BTrRl5") {
        handleMeterLongReg(EmbRet, pi + 1, pi, pi + 1, pi, pi + 1, pi, pi + 1, pi, durPi, meter);
    } else if (variant == "CTrRl1") {
        handleMeterLongReg(EmbRet, pi, pi + 2, pi, pi + 2, pi, pi + 2, pi, pi + 2, durPi, meter);
    } else if (variant == "CTrRl5") {
        handleMeterLongReg(EmbRet, pi, pi + 1, pi, pi + 1, pi, pi + 1, pi, pi + 1, durPi, meter);
    }
    
    // Delayed Normal Trills - Baroque and Classical
    else if (variant == "BTrDen1") {
        handleMeterDelayedNormal(EmbRet, pi + 2, pi, pi + 2, pi, pi + 2, pi, durPi, meter);
    } else if (variant == "BTrDen5") {
        handleMeterDelayedNormal(EmbRet, pi + 1, pi, pi + 1, pi, pi + 1, pi, durPi, meter);
    } else if (variant == "CTrDen1") {
        handleMeterDelayedNormal(EmbRet, pi, pi + 5, pi, pi + 5, pi, pi + 5, durPi, meter);
    } else if (variant == "CTrDen5") {
        handleMeterDelayedNormal(EmbRet, pi, pi + 1, pi, pi + 1, pi, pi + 1, durPi, meter);
    }
    
    // Delayed Long Trills - Baroque and Classical
    else if (variant == "BTrDel1") {
        handleMeterDelayedLong(EmbRet, pi + 2, pi, pi + 2, pi, pi + 2, pi, durPi, meter);
    } else if (variant == "BTrDel5") {
        handleMeterDelayedLong(EmbRet, pi + 1, pi, pi + 1, pi, pi + 1, pi, durPi, meter);
    } else if (variant == "CTrDel1") {
        handleMeterDelayedLong(EmbRet, pi, pi + 2, pi, pi + 2, pi, pi + 2, durPi, meter);
    } else if (variant == "CTrDel5") {
        handleMeterDelayedLong(EmbRet, pi, pi + 1, pi, pi + 1, pi, pi + 1, durPi, meter);
    }
    
    // Ascending Short Trills - Baroque and Classical
    else if (variant == "BTrAs1") {
        handleMeterAscendingShort(EmbRet, pi - 2, pi, pi + 2, pi, pi + 2, pi, durPi, meter);
    } else if (variant == "BTrAs5") {
        handleMeterAscendingShort(EmbRet, pi - 1, pi, pi + 2, pi, pi + 2, pi, durPi, meter);
    } else if (variant == "CTrAs1") {
        handleMeterAscendingShort(EmbRet, pi - 2, pi, pi + 2, pi, pi, pi + 2, durPi, meter);
    } else if (variant == "CTrAs5") {
        handleMeterAscendingShort(EmbRet, pi - 1, pi, pi + 2, pi, pi, pi + 2, durPi, meter);
    }
    
    // Ascending Normal Trills - Baroque and Classical
    else if (variant == "BTrAn1") {
        handleMeterAscendingNormal(EmbRet, pi - 2, pi, pi + 2, pi, pi + 2, pi, pi + 2, pi, durPi, meter);
    } else if (variant == "BTrAn5") {
        handleMeterAscendingNormal(EmbRet, pi - 1, pi, pi + 2, pi, pi + 2, pi, pi + 2, pi, durPi, meter);
    } else if (variant == "CTrAn1") {
        handleMeterAscendingNormal(EmbRet, pi - 2, pi, pi + 2, pi, pi, pi + 2, pi, pi + 2, durPi, meter);
    } else if (variant == "CTrAn5") {
        handleMeterAscendingNormal(EmbRet, pi - 1, pi, pi + 2, pi, pi, pi + 2, pi, pi + 2, durPi, meter);
    }
    
    // Ascending Long Trills - Baroque and Classical
    else if (variant == "BTrAl1") {
        handleMeterAscendingLong(EmbRet, pi - 2, pi, pi + 2, pi, pi + 2, pi, pi + 2, pi, pi + 2, pi, pi + 2, pi, durPi, meter);
    } else if (variant == "BTrAl5") {
        handleMeterAscendingLong(EmbRet, pi - 1, pi, pi + 2, pi, pi + 2, pi, pi + 2, pi, pi + 2, pi, pi + 2, pi, durPi, meter);
    } else if (variant == "CTrAl1") {
        handleMeterAscendingLong(EmbRet, pi - 2, pi, pi + 2, pi, pi, pi + 2, pi, pi + 2, pi, pi + 2, pi, pi + 2, durPi, meter);
    } else if (variant == "CTrAl5") {
        handleMeterAscendingLong(EmbRet, pi - 1, pi, pi + 2, pi, pi, pi + 2, pi, pi + 2, pi, pi + 2, pi, pi + 2, durPi, meter);
    }
    
    // Descending Short Trills - Baroque and Classical
    else if (variant == "BTrDs1") {
        handleMeterAscendingShort(EmbRet, pi + 2, pi, pi - 2, pi, pi + 2, pi, durPi, meter);
    } else if (variant == "BTrDs5") {
        handleMeterAscendingShort(EmbRet, pi + 1, pi, pi - 2, pi, pi + 2, pi, durPi, meter);
    } else if (variant == "CTrDs1") {
        handleMeterAscendingShort(EmbRet, pi + 2, pi, pi - 2, pi, pi, pi + 2, durPi, meter);
    } else if (variant == "CTrDs5") {
        handleMeterAscendingShort(EmbRet, pi + 1, pi, pi - 2, pi, pi, pi + 2, durPi, meter);
    }
    
    // Descending Normal Trills - Baroque and Classical
    else if (variant == "BTrDn1") {
        handleMeterAscendingNormal(EmbRet, pi + 2, pi, pi - 2, pi, pi + 2, pi, pi + 2, pi, durPi, meter);
    } else if (variant == "BTrDn5") {
        handleMeterAscendingNormal(EmbRet, pi + 1, pi, pi - 2, pi, pi + 2, pi, pi + 2, pi, durPi, meter);
    } else if (variant == "CTrDn1") {
        handleMeterAscendingNormal(EmbRet, pi + 2, pi, pi - 2, pi, pi, pi + 2, pi, pi + 2, durPi, meter);
    } else if (variant == "CTrDn5") {
        handleMeterAscendingNormal(EmbRet, pi + 1, pi, pi - 2, pi, pi, pi + 2, pi, pi + 2, durPi, meter);
    }
    
    // Descending Long Trills - Baroque and Classical
    else if (variant == "BTrDl1") {
        handleMeterAscendingLong(EmbRet, pi + 2, pi, pi - 2, pi, pi + 2, pi, pi + 2, pi, pi + 2, pi, pi + 2, pi, durPi, meter);
    } else if (variant == "BTrDl5") {
        handleMeterAscendingLong(EmbRet, pi + 1, pi, pi - 2, pi, pi + 2, pi, pi + 2, pi, pi + 2, pi, pi + 2, pi, durPi, meter);
    } else if (variant == "CTrDl1") {
        handleMeterAscendingLong(EmbRet, pi + 2, pi, pi - 2, pi, pi, pi + 2, pi, pi + 2, pi, pi + 2, pi, pi + 2, durPi, meter);
    } else if (variant == "CTrDl5") {
        handleMeterAscendingLong(EmbRet, pi + 1, pi, pi - 2, pi, pi, pi + 2, pi, pi + 2, pi, pi + 2, pi, pi + 2, durPi, meter);
    }
    
    // Terminal Short Trills - Baroque and Classical
    else if (variant == "BTrTs1") {
        handleMeterTerminalShort(EmbRet, pi + 2, pi, pi - 2, pi, durPi, meter);
    } else if (variant == "BTrTs5") {
        handleMeterTerminalShort(EmbRet, pi + 1, pi, pi - 2, pi, durPi, meter);
    } else if (variant == "CTrTs1") {
        handleMeterTerminalShort(EmbRet, pi + 2, pi, pi - 2, pi, durPi, meter);
    } else if (variant == "CTrTs5") {
        handleMeterTerminalShort(EmbRet, pi + 1, pi, pi - 2, pi, durPi, meter);
    }
    
    // Terminal Normal Trills - Baroque and Classical
    else if (variant == "BTrTn1") {
        handleMeterTerminalNormal(EmbRet, pi + 2, pi, pi + 2, pi, pi + 2, pi, pi - 2, pi, durPi, meter);
    } else if (variant == "BTrTn5") {
        handleMeterTerminalNormal(EmbRet, pi + 1, pi, pi + 1, pi, pi + 1, pi, pi - 2, pi, durPi, meter);
    } else if (variant == "CTrTn1") {
        handleMeterTerminalNormal(EmbRet, pi, pi + 2, pi, pi + 2, pi, pi + 2, pi - 2, pi, durPi, meter);
    } else if (variant == "CTrTn5") {
        handleMeterTerminalNormal(EmbRet, pi, pi + 1, pi, pi + 1, pi, pi + 1, pi - 2, pi, durPi, meter);
    }
    
    // Terminal Long Trills - Baroque and Classical
    else if (variant == "BTrTl1") {
        handleMeterTerminalLong(EmbRet, pi + 2, pi, pi + 2, pi, pi + 2, pi, pi + 2, pi, pi + 2, pi, pi + 2, pi, pi + 2, pi, pi - 2, pi, durPi, meter);
    } else if (variant == "BTrTl5") {
        handleMeterTerminalLong(EmbRet, pi + 1, pi, pi + 1, pi, pi + 1, pi, pi + 1, pi, pi + 1, pi, pi + 1, pi, pi + 1, pi, pi - 2, pi, durPi, meter);
    } else if (variant == "CTrTl1") {
        handleMeterTerminalLong(EmbRet, pi, pi + 2, pi, pi + 2, pi, pi + 2, pi, pi + 2, pi, pi + 2, pi, pi + 2, pi, pi + 2, pi - 2, pi, durPi, meter);
    } else if (variant == "CTrTl5") {
        handleMeterTerminalLong(EmbRet, pi, pi + 1, pi, pi + 1, pi, pi + 1, pi, pi + 1, pi, pi + 1, pi, pi + 1, pi, pi + 1, pi - 2, pi, durPi, meter);
    }
    
    return EmbRet;
}

// Structure to represent a trill variant
struct TrillVariant {
    std::string code;
    std::string description;
};

// Generate a random pool of trill variants for user selection
std::vector<TrillVariant> generateRandomTrillVariantPool(int poolSize = 10) {
    // Define the complete pool of trill variants
    const std::vector<TrillVariant> allVariants = {
        // Regular Trills - Baroque and Classical - Short, Normal and Long
        {"BTrRs1", "Baroque Short Regular Trill - Major 2nd"},
        {"BTrRs5", "Baroque Short Regular Trill - Minor 2nd"},
        {"CTrRs1", "Classical Short Regular Trill - Major 2nd"},
        {"CTrRs5", "Classical Short Regular Trill - Minor 2nd"},
        
        {"BTrRn1", "Baroque Normal Regular Trill - Major 2nd"},
        {"BTrRn5", "Baroque Normal Regular Trill - Minor 2nd"},
        {"CTrRn1", "Classical Normal Regular Trill - Major 2nd"},
        {"CTrRn5", "Classical Normal Regular Trill - Minor 2nd"},
        
        {"BTrRl1", "Baroque Long Regular Trill - Major 2nd"},
        {"BTrRl5", "Baroque Long Regular Trill - Minor 2nd"},
        {"CTrRl1", "Classical Long Regular Trill - Major 2nd"},
        {"CTrRl5", "Classical Long Regular Trill - Minor 2nd"},
        
        // Delayed Trills - Baroque and Classical
        {"BTrDen1", "Baroque Delayed Normal Trill - Major 2nd"},
        {"BTrDen5", "Baroque Delayed Normal Trill - Minor 2nd"},
        {"CTrDen1", "Classical Delayed Normal Trill - Major 2nd"},
        {"CTrDen5", "Classical Delayed Normal Trill - Minor 2nd"},
        
        {"BTrDel1", "Baroque Delayed Long Trill - Major 2nd"},
        {"BTrDel5", "Baroque Delayed Long Trill - Minor 2nd"},
        {"CTrDel1", "Classical Delayed Long Trill - Major 2nd"},
        {"CTrDel5", "Classical Delayed Long Trill - Minor 2nd"},
        
        // Ascending Trills - Baroque and Classical
        {"BTrAs1", "Baroque Ascending Short Trill - Major 2nd"},
        {"BTrAs5", "Baroque Ascending Short Trill - Minor 2nd"},
        {"CTrAs1", "Classical Ascending Short Trill - Major 2nd"},
        {"CTrAs5", "Classical Ascending Short Trill - Minor 2nd"},
        
        {"BTrAn1", "Baroque Ascending Normal Trill - Major 2nd"},
        {"BTrAn5", "Baroque Ascending Normal Trill - Minor 2nd"},
        {"CTrAn1", "Classical Ascending Normal Trill - Major 2nd"},
        {"CTrAn5", "Classical Ascending Normal Trill - Minor 2nd"},
        
        {"BTrAl1", "Baroque Ascending Long Trill - Major 2nd"},
        {"BTrAl5", "Baroque Ascending Long Trill - Minor 2nd"},
        {"CTrAl1", "Classical Ascending Long Trill - Major 2nd"},
        {"CTrAl5", "Classical Ascending Long Trill - Minor 2nd"},
        
        // Descending Trills - Baroque and Classical
        {"BTrDs1", "Baroque Descending Short Trill - Major 2nd"},
        {"BTrDs5", "Baroque Descending Short Trill - Minor 2nd"},
        {"CTrDs1", "Classical Descending Short Trill - Major 2nd"},
        {"CTrDs5", "Classical Descending Short Trill - Minor 2nd"},
        
        {"BTrDn1", "Baroque Descending Normal Trill - Major 2nd"},
        {"BTrDn5", "Baroque Descending Normal Trill - Minor 2nd"},
        {"CTrDn1", "Classical Descending Normal Trill - Major 2nd"},
        {"CTrDn5", "Classical Descending Normal Trill - Minor 2nd"},
        
        {"BTrDl1", "Baroque Descending Long Trill - Major 2nd"},
        {"BTrDl5", "Baroque Descending Long Trill - Minor 2nd"},
        {"CTrDl1", "Classical Descending Long Trill - Major 2nd"},
        {"CTrDl5", "Classical Descending Long Trill - Minor 2nd"},
        
        // Terminal Trills - Baroque and Classical
        {"BTrTs1", "Baroque Terminal Short Trill - Major 2nd"},
        {"BTrTs5", "Baroque Terminal Short Trill - Minor 2nd"},
        {"CTrTs1", "Classical Terminal Short Trill - Major 2nd"},
        {"CTrTs5", "Classical Terminal Short Trill - Minor 2nd"},
        
        {"BTrTn1", "Baroque Terminal Normal Trill - Major 2nd"},
        {"BTrTn5", "Baroque Terminal Normal Trill - Minor 2nd"},
        {"CTrTn1", "Classical Terminal Normal Trill - Major 2nd"},
        {"CTrTn5", "Classical Terminal Normal Trill - Minor 2nd"},
        
        {"BTrTl1", "Baroque Terminal Long Trill - Major 2nd"},
        {"BTrTl5", "Baroque Terminal Long Trill - Minor 2nd"},
        {"CTrTl1", "Classical Terminal Long Trill - Major 2nd"},
        {"CTrTl5", "Classical Terminal Long Trill - Minor 2nd"}
    };

    // Create a copy of all variants and shuffle it
    std::vector<TrillVariant> shuffledVariants = allVariants;

    // Use modern random number generation
    std::random_device rd;
    std::mt19937 g(rd());
    std::shuffle(shuffledVariants.begin(), shuffledVariants.end(), g);

    // Return the first poolSize variants
    std::vector<TrillVariant> pool;
    for (int i = 0; i < std::min(poolSize, static_cast<int>(shuffledVariants.size())); ++i) {
        pool.push_back(shuffledVariants[i]);
    }

    return pool;
}

// Parse user input for multiple choice selection
std::vector<int> parseUserChoices(const std::string& input, int maxChoice) {
    std::vector<int> choices;
    std::istringstream iss(input);
    std::string token;

    while (iss >> token) {
        try {
            int choice = std::stoi(token);
            if (choice >= 1 && choice <= maxChoice) {
                // Avoid duplicates
                if (std::find(choices.begin(), choices.end(), choice) == choices.end()) {
                    choices.push_back(choice);
                }
            }
        } catch (const std::exception&) {
            // Ignore invalid tokens
        }
    }

    return choices;
}

// Check if a label should be transformed based on percentage
bool shouldTransformLabel(double transformationPercentage) {
    // Generate random number between 0 and 100
    double randomValue = (static_cast<double>(rand()) / RAND_MAX) * 100.0;
    return randomValue < transformationPercentage;
}

// Structure to represent a MIDI note event
struct MidiEvent {
    int track;
    int noteNumber;
    int startTime;
    int duration;
    bool isNoteOn;
};

// Application state
struct AppState {
    std::string inputFile;
    std::string outputFile;
    std::string midiOutputFile;
    double transformationPercentage = 50.0;
    std::vector<std::string> selectedVariants;
    bool processingComplete = false;
    std::string statusMessage;
    std::string resultSummary;
    int totalEligibleNotes = 0;
    int transformedNotes = 0;
    std::map<std::string, int> variantUsageCount;
};

// Function to process file with GUI integration
void processFile(const std::string& inputFile, const std::string& outputFile, AppState& state) {
    std::ifstream input(inputFile);
    std::ofstream output(outputFile);

    if (!input.is_open() || !output.is_open()) {
        state.statusMessage = "Error opening files.";
        return;
    }

    // Write header to the output file
    output << std::left << std::setw(11) << "Track"
           << std::setw(11) << "Note"
           << std::setw(20) << "Duration"
           << std::setw(20) << "Label"
           << std::setw(25) << "Trill_Variant"
           << "\n";
    output << "---------------------------------------------------------------------------------\n";

    // Reset statistics
    state.totalEligibleNotes = 0;
    state.transformedNotes = 0;
    state.variantUsageCount.clear();

    std::string line;
    while (std::getline(input, line)) {
        std::istringstream ss(line);

        int track, duration;
        std::string noteName, label;

        // Parse line with Note in string format (e.g., "C4")
        if (!(ss >> track >> noteName >> duration)) {
            output << line << "\n";  // Handle malformed lines
            continue;
        }

        std::getline(ss, label);
        label.erase(0, label.find_first_not_of(" \t"));  // Trim leading whitespace
        // Remove trailing carriage return and whitespace (Windows line endings)
        label.erase(label.find_last_not_of(" \t\r\n") + 1);

        // Check if this label is eligible for transformation
        if (label == "RLN" || label == "CS" || label == "I3" || label == "I8" ||
            label == "U2R" || label == "BM" || label == "SPU" || label == "SPD" ||
            label == "CH" || label == "CW" || label == "CD" || label == "HT" ||
            label == "FM" || label == "RN" || label == "LAD" || label == "DN" ||
            label == "DNW" || label == "SN" || label == "LNSN" || label == "SAN" ||
            label == "SMP" || label == "DLP3") {

            state.totalEligibleNotes++;

            // Check if this note should be transformed based on percentage
            if (shouldTransformLabel(state.transformationPercentage)) {
                state.transformedNotes++;

                try {
                    // Convert note name to MIDI number
                    int noteIndex = getNoteNumber(noteName);

                    // Randomly select a variant from the user's choices
                    std::string selectedVariant;
                    if (state.selectedVariants.empty() || (state.selectedVariants.size() == 1 && state.selectedVariants[0] == "RANDOM")) {
                        // Use a random variant from the complete list
                        std::vector<TrillVariant> allVariants = generateRandomTrillVariantPool(100); // Get a large pool
                        selectedVariant = allVariants[rand() % allVariants.size()].code;
                    } else {
                        // Use one of the user's selected variants randomly
                        selectedVariant = state.selectedVariants[rand() % state.selectedVariants.size()];
                    }

                    // Apply trill transformation
                    auto transformed = applyTrill(noteIndex, duration, DUPLE, selectedVariant);

                    // Track variant usage
                    state.variantUsageCount[selectedVariant]++;

                    // Output the transformed notes
                    for (const auto& [transformedNote, transformedDuration] : transformed) {
                        std::string transNote = getNoteName(transformedNote); // Convert MIDI to readable name
                        output << std::left
                               << std::setw(11) << track
                               << std::setw(11) << transNote
                               << std::setw(20) << transformedDuration
                               << std::setw(20) << label
                               << std::setw(25) << selectedVariant
                               << "\n";
                    }
                } catch (const std::exception& e) {
                    // Handle cases where getNoteNumber produces an error
                    state.statusMessage += "Error processing note '" + noteName + "': " + e.what() + "\n";
                }
            } else {
                // Output original data for notes not selected for transformation
                output << std::left
                       << std::setw(11) << track
                       << std::setw(11) << noteName
                       << std::setw(20) << duration
                       << std::setw(20) << label
                       << std::setw(25) << "ORIGINAL" // Mark as original
                       << "\n";
            }
        } else {
            // Output original data for non-eligible labels
            output << std::left
                   << std::setw(11) << track
                   << std::setw(11) << noteName
                   << std::setw(20) << duration
                   << std::setw(20) << label
                   << std::setw(25) << "" // Empty variant column
                   << "\n";
        }
    }

    input.close();
    output.close();

    // Calculate actual percentage
    double actualPercentage = state.totalEligibleNotes > 0 ?
        (static_cast<double>(state.transformedNotes) / state.totalEligibleNotes) * 100.0 : 0.0;

    // Update result summary
    std::stringstream summary;
    summary << "Transformation Statistics:\n"
            << "Total eligible notes found: " << state.totalEligibleNotes << "\n"
            << "Notes transformed: " << state.transformedNotes << "\n"
            << "Actual transformation percentage: " << std::fixed << std::setprecision(1)
            << actualPercentage << "%\n\n";

    if (state.selectedVariants.size() == 1 && state.selectedVariants[0] != "RANDOM") {
        summary << "Variant used: " << state.selectedVariants[0] << "\n";
    } else if (state.selectedVariants.size() > 1) {
        summary << "Variants used (" << state.selectedVariants.size() << " total):\n";
        for (const auto& [variant, count] : state.variantUsageCount) {
            summary << "  " << variant << ": " << count << " times\n";
        }
    } else {
        summary << "Variant selection: Random\n";
    }

    summary << "Processing complete. Transformed results written to " << outputFile << "\n";
    state.resultSummary = summary.str();
    state.statusMessage = "Processing complete!";
    state.processingComplete = true;
}

// Function to convert processed data to MIDI file with MIDI sync fix
void convertToMidi(const std::string& inputFile, const std::string& outputFile, AppState& state) {
    std::ifstream input(inputFile);
    if (!input.is_open()) {
        state.statusMessage += "Error opening input file: " + inputFile + "\n";
        return;
    }

    // Skip header lines
    std::string line;
    std::getline(input, line); // Skip column headers
    std::getline(input, line); // Skip separator line

    // Parse the file and collect note events
    std::map<int, std::vector<MidiEvent>> trackEvents;
    std::map<int, int> trackPositions; // FIXED: Track positions for sequential notes within each track

    while (std::getline(input, line)) {
        std::istringstream ss(line);
        int track;
        std::string noteName;
        int duration;
        std::string label, variant;

        // Skip lines that don't contain note data
        if (line.empty() || line[0] == '-' || line.find("MIDI File Analyzed") != std::string::npos) {
            continue;
        }

        // Parse the line
        if (!(ss >> track >> noteName >> duration)) {
            continue; // Skip malformed lines
        }

        // Skip header or non-note lines
        if (noteName == "Note" || noteName == "Track") {
            continue;
        }

        try {
            int noteNumber = getNoteNumber(noteName);

            // FIXED: Use track-specific positioning for sequential notes within each track
            int& trackPosition = trackPositions[track];

            // Create note-on event at the track's current position
            MidiEvent noteOn{track, noteNumber, trackPosition, duration, true};
            trackEvents[track].push_back(noteOn);

            // Create note-off event
            MidiEvent noteOff{track, noteNumber, trackPosition + duration, 0, false};
            trackEvents[track].push_back(noteOff);

            // Update the position for this track (notes within a track are sequential)
            trackPosition += duration;

        } catch (const std::exception& e) {
            state.statusMessage += "Error processing note '" + noteName + "': " + std::string(e.what()) + "\n";
        }
    }

    input.close();

    // Write MIDI file
    std::ofstream midiFile(outputFile, std::ios::binary);
    if (!midiFile.is_open()) {
        state.statusMessage += "Error opening output MIDI file: " + outputFile + "\n";
        return;
    }

    // Write MIDI header
    // Format: MThd + <length> + <format> + <tracks> + <division>
    midiFile.write("MThd", 4); // Chunk type

    // Header length (always 6 bytes)
    char headerLength[4] = {0, 0, 0, 6};
    midiFile.write(headerLength, 4);

    // Format (0 = single track, 1 = multiple tracks, same timebase)
    char format[2] = {0, 1};
    midiFile.write(format, 2);

    // Number of tracks
    int numTracks = trackEvents.size();
    char tracksCount[2] = {static_cast<char>((numTracks >> 8) & 0xFF),
                          static_cast<char>(numTracks & 0xFF)};
    midiFile.write(tracksCount, 2);

    // Division (ticks per quarter note = 1024)
    char division[2] = {0x04, 0x00}; // 1024 in big-endian
    midiFile.write(division, 2);

    // Write each track
    for (const auto& [trackNum, events] : trackEvents) {
        // Sort events by time
        std::vector<MidiEvent> sortedEvents = events;
        std::sort(sortedEvents.begin(), sortedEvents.end(),
                 [](const MidiEvent& a, const MidiEvent& b) {
                     return a.startTime < b.startTime ||
                            (a.startTime == b.startTime && !a.isNoteOn && b.isNoteOn);
                 });

        // Write track header
        midiFile.write("MTrk", 4);

        // Placeholder for track length (will be filled in later)
        long trackLengthPos = midiFile.tellp();
        midiFile.write("\0\0\0\0", 4);

        // Track start position
        long trackStartPos = midiFile.tellp();

        // Write track events
        int lastTime = 0;

        // Set instrument (program change) - using piano (0) as default
        char programChange[3] = {0x00, static_cast<char>(0xC0), 0x00}; // Delta time, command, program number
        midiFile.write(programChange, 3);

        for (const auto& event : sortedEvents) {
            // Write delta time (variable length)
            int deltaTime = event.startTime - lastTime;
            lastTime = event.startTime;

            // Convert delta time to variable length quantity
            std::vector<char> vlq;
            if (deltaTime == 0) {
                vlq.push_back(0);
            } else {
                while (deltaTime > 0) {
                    char byte = deltaTime & 0x7F;
                    deltaTime >>= 7;
                    if (!vlq.empty()) {
                        byte |= 0x80;
                    }
                    vlq.push_back(byte);
                }
                std::reverse(vlq.begin(), vlq.end());
            }

            for (char byte : vlq) {
                midiFile.put(byte);
            }

            // Write note event
            if (event.isNoteOn) {
                // Note on: 0x90 | channel, note, velocity
                midiFile.put(0x90);
                midiFile.put(static_cast<char>(event.noteNumber));
                midiFile.put(0x64); // Velocity (100)
            } else {
                // Note off: 0x80 | channel, note, velocity
                midiFile.put(0x80);
                midiFile.put(static_cast<char>(event.noteNumber));
                midiFile.put(0x00); // Velocity (0)
            }
        }

        // Write end of track
        midiFile.put(0x00); // Delta time
        midiFile.put(0xFF); // Meta event
        midiFile.put(0x2F); // End of track
        midiFile.put(0x00); // Length

        // Calculate and write track length
        long trackEndPos = midiFile.tellp();
        long trackLength = trackEndPos - trackStartPos;

        midiFile.seekp(trackLengthPos);
        char trackLengthBytes[4] = {
            static_cast<char>((trackLength >> 24) & 0xFF),
            static_cast<char>((trackLength >> 16) & 0xFF),
            static_cast<char>((trackLength >> 8) & 0xFF),
            static_cast<char>(trackLength & 0xFF)
        };
        midiFile.write(trackLengthBytes, 4);
        midiFile.seekp(trackEndPos);
    }

    midiFile.close();
    state.statusMessage += "MIDI file created successfully: " + outputFile + "\n";
}