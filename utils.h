#ifndef UTILS_H
#define UTILS_H

#include <fstream>

inline void write_int(std::ofstream& fout, int v) {
    fout.write((char*)(&v), sizeof(int));
}

inline void write_long(std::ofstream& fout, long v) {
    fout.write((char*)(&v), sizeof(long));
}

inline void write_short(std::ofstream& fout, short v) {
    fout.write((char*)(&v), sizeof(short));
}

inline void write_char(std::ofstream& fout, char v) {
    fout.write((char*)(&v), sizeof(char));
}


#endif // UTILS_H
