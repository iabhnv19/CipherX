#include "../include/cryptanalysis.h"
#include "../include/vault.h"

const double ENGLISH_FREQ[26] = {
    8.2, 1.5, 2.8, 4.3, 12.7, 2.2, 2.0, 6.1, 7.0, 0.15,
    0.77, 4.0, 2.4, 6.7, 7.5, 1.9, 0.095, 6.0, 6.3, 9.1,
    2.8, 0.98, 2.4, 0.15, 2.0, 0.074
};

double scoreEnglishText(const string &text) {
    int counts[26] = {0};
    int totalLetters = 0;
    for (char c : text) {
        if (isalpha((unsigned char)c)) {
            counts[tolower((unsigned char)c) - 'a']++;
            totalLetters++;
        }
    }
    if (totalLetters == 0) return -9999.0;
    double score = 0.0;
    for (int i = 0; i < 26; i++) {
        double observedFreq = (double)counts[i] / totalLetters * 100.0;
        double diff = observedFreq - ENGLISH_FREQ[i];
        score -= (diff * diff) / (ENGLISH_FREQ[i] + 0.5);
    }
    return score;
}

void frequencyAnalysis(const string &text) {
    clearScreen();
    heading("Letter Frequency Analysis");
    int counts[26] = {0};
    int totalAlpha = 0;
    for (char c : text) {
        if (isalpha((unsigned char)c)) {
            counts[tolower((unsigned char)c) - 'a']++;
            totalAlpha++;
        }
    }
    if (totalAlpha == 0) {
        warn("no letters found to analyze.");
        return;
    }
    cout << GRY << "  letters analyzed: " << WHT << totalAlpha << RESET << "\n\n";
    cout << GRY << "  letter   text %   ref %   distribution" << RESET << "\n";
    for (int i = 0; i < 26; i++) {
        char letter = 'A' + i;
        double pct = ((double)counts[i] / totalAlpha) * 100.0;
        double ref = ENGLISH_FREQ[i];
        cout << "    " << CYN << letter << RESET << "     "
             << setw(5) << fixed << setprecision(1) << pct << "%  "
             << setw(5) << fixed << setprecision(1) << ref << "%   ";
        int barLen = (int)(pct * 1.5);
        if (barLen > 24) barLen = 24;
        cout << GRN;
        for (int b = 0; b < barLen; b++) cout << "|";
        cout << RESET << "\n";
    }
}

void bruteForceBreak(const string &cipher) {
    clearScreen();
    heading("Cipher Cracker");
    cout << GRY << "  ciphertext: " << WHT << cipher << RESET << "\n\n";
    cout << "  " << CYN << "1" << RESET << "  Brute Force Caesar (try all 25 shifts)\n";
    cout << "  " << CYN << "2" << RESET << "  Brute Force Rail Fence (try 2-10 rails)\n";
    cout << "  " << CYN << "3" << RESET << "  Brute Force XOR (try 1-255 keys)\n\n";
    int mode = readInt("  pick ", 1, 3);

    struct Candidate {
        string desc;
        string text;
        double score;
    };
    vector<Candidate> results;

    if (mode == 1) {
        for (int s = 1; s <= 25; s++) {
            string cand = decryptCaesar(cipher, s);
            results.push_back({"shift " + to_string(s), cand, scoreEnglishText(cand)});
        }
    } else if (mode == 2) {
        for (int r = 2; r <= 10; r++) {
            string cand = decryptRailFence(cipher, r);
            results.push_back({"rails " + to_string(r), cand, scoreEnglishText(cand)});
        }
    } else {
        string rawCipher;
        bool isHex = fromHex(cipher, rawCipher);
        if (!isHex) rawCipher = cipher;
        for (int k = 1; k <= 255; k++) {
            string cand = xorCrypt(rawCipher, k);
            results.push_back({"key 0x" + toHex(string(1, (char)k)), cand, scoreEnglishText(cand)});
        }
    }

    sort(results.begin(), results.end(), [](const Candidate &a, const Candidate &b) {
        return a.score > b.score;
    });

    cout << "\n" << GRY << "  rank    score   parameter   preview" << RESET << "\n";
    int topN = min((int)results.size(), 8);
    for (int i = 0; i < topN; i++) {
        string displayStr = results[i].text;
        replace(displayStr.begin(), displayStr.end(), '\n', ' ');
        if (displayStr.length() > 38) displayStr = displayStr.substr(0, 38) + "...";
        cout << "  " << (i == 0 ? GRN : YEL) << "#" << (i + 1) << RESET
             << "    " << setw(7) << fixed << setprecision(1) << results[i].score
             << "   " << setw(11) << left << results[i].desc << RESET << " "
             << WHT << displayStr << RESET << "\n";
    }
}

void cryptanalysisTools() {
    clearScreen();
    heading("Cracker and Frequency Analysis");
    cout << "  " << CYN << "1" << RESET << "  Brute force an unknown cipher\n";
    cout << "  " << CYN << "2" << RESET << "  Letter frequency distribution\n";
    cout << "  " << CYN << "3" << RESET << "  Crack message from vault\n\n";
    int pick = readInt("  pick ", 1, 3);

    if (pick == 1) {
        string cipher = ask("ciphertext");
        bruteForceBreak(cipher);
    } else if (pick == 2) {
        string sample = ask("sample text");
        frequencyAnalysis(sample);
    } else {
        if (messageCount == 0) {
            warn("vault is empty.");
            return;
        }
        int id = readInt("  message id ", 1, 999999999);
        int idx = findMessageIndex(id);
        if (idx == -1) {
            warn("id not found.");
            return;
        }
        cout << "\n  " << CYN << "1" << RESET << " Frequency chart\n";
        cout << "  " << CYN << "2" << RESET << " Brute force cracker\n\n";
        int opt = readInt("  pick ", 1, 2);
        if (opt == 1) frequencyAnalysis(msgArray[idx].encryptedText);
        else bruteForceBreak(msgArray[idx].encryptedText);
    }
}
