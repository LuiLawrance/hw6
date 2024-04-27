#ifndef HASH_H
#define HASH_H

#include <iostream>
#include <cmath>
#include <random>
#include <chrono>
#include <string>
#include <vector>

typedef std::size_t HASH_INDEX_T;

struct MyStringHash {
    // Predefined or randomized values
    HASH_INDEX_T rValues[5] { 983132572, 1468777056, 552714139, 984953261, 261934300 };

    // Constructor to optionally initialize with random values
    MyStringHash(bool debug = true)
    {
        if(!debug){
            generateRValues();
        }
    }

    // Operator() to compute hash value of a string
    HASH_INDEX_T operator()(const std::string& k) const {
      std::vector<unsigned long long> w(5, 0);
      const int segmentLength = 6;
      int fullSegments = k.length() / segmentLength;
      int remainingChars = k.length() % segmentLength;
      
      int segmentIndex = 0;
      
      if (remainingChars > 0) {
        unsigned long long segmentValue = 0;
        unsigned long long base = 1;

        for (int j = remainingChars - 1; j >= 0; --j) {
          segmentValue += letterDigitToNumber(k[j]) * base;
          base *= 36;
        }

        w[4 - fullSegments] = segmentValue;
        // std::cout << "Segment " << segmentIndex << ": start=0, end=" << remainingChars << ", w[4 - " << fullSegments << "] = " << segmentValue << std::endl;
        segmentIndex++;
      }
      
      for (int i = 0; i < fullSegments; ++i) {
        int start = remainingChars + i * segmentLength;
        int end = start + segmentLength;
        unsigned long long segmentValue = 0;
        unsigned long long base = 1;

        for (int j = end - 1; j >= start; --j) {
            segmentValue += letterDigitToNumber(k[j]) * base;
            base *= 36;
        }

        w[4 - i] = segmentValue;
        
        // std::cout << "Segment " << segmentIndex << ": start=" << start << ", end=" << end << ", w[4 - " << i << "] = " << segmentValue << std::endl;
        segmentIndex++;
    }

    unsigned long long hashValue = 0;

    for (int i = 0; i < 5; ++i) {
      unsigned long long part = rValues[i] * w[i];
      hashValue += part;
      // std::cout << "w[" << i << "] = " << w[i] << ", rValues[" << i << "] = " << rValues[i] << ", Product = " << part << ", Partial Hash = " << hashValue << std::endl;
    }

    // std::cout << "Final Hash Value = " << hashValue << std::endl;
    return hashValue;
  }

    // Convert letters and digits to numbers from 0 to 35
    unsigned int letterDigitToNumber(char letter) const
    {
        if (isdigit(letter)) return letter - '0' + 26;
        if (isupper(letter)) letter = tolower(letter);
        return letter - 'a';
    }

    // Code to generate the random R values
    void generateRValues()
    {
        unsigned seed = std::chrono::system_clock::now().time_since_epoch().count();
        std::mt19937 generator(seed);  // mt19937 is a standard random number generator
        for (int i = 0; i < 5; ++i) {
            rValues[i] = generator();
        }
    }
};

#endif
