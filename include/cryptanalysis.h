#ifndef CRYPTANALYSIS_H
#define CRYPTANALYSIS_H

#include "common.h"
#include "ciphers.h"

double scoreEnglishText(const string &text);
void frequencyAnalysis(const string &text);
void bruteForceBreak(const string &cipher);
void cryptanalysisTools();

#endif

