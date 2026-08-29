module;
#include <cmath>
#include <numbers> // std::numbers::pi
#include <vector>

module math_logic;

void NumberGenerator::AddPoint(unsigned long long p) {
  double r = static_cast<double>(p);
  double theta = std::fmod(static_cast<double>(p), 2.0 * std::numbers::pi);

  NumberPoint pt;
  pt.x = r * std::cos(theta);
  pt.y = -r * std::sin(theta);
  pt.p = p;

  points.push_back(pt); // adds to end of std::vector
}

void NumberGenerator::GeneratePrimesInRange(int rangeSize) {
  unsigned long long low = lastChecked + 1;
  unsigned long long high = low + rangeSize - 1;

  // Initializes the list
  std::vector<char> isNotPrime(rangeSize, 0);

  // If on first sieve
  if (low == 1) {
    isNotPrime[0] = 1; // 1 is not prime
    for (unsigned long long p = 2; p * p <= high; p++) {
      if (!isNotPrime[p - low]) {
        for (unsigned long long j = p * p; j <= high; j += p)
          isNotPrime[j - low] = 1;
      }
    }
  }

  // Sieves based on already calculated primes
  for (const auto &pt : points) {
    unsigned long long p = pt.p;
    if (p * p > high)
      break;

    // Finds first multiple of p in range (low, high) truncating the division
    unsigned long long start = (low / p) * p;
    if (start < low)
      start += p;

    // Numbers smaller than p*p were already eliminated by past iterations
    if (start < p * p)
      start = p * p;

    // Marks multiples of p as non-prime
    for (unsigned long long j = start; j <= high; j += p) {
      isNotPrime[j - low] = 1;
    }
  }

  // Saves all non-marked (primes) calculated
  for (int i = 0; i < rangeSize; i++) {
    if (!isNotPrime[i]) {
      AddPoint(i + low);
    }
  }
  lastChecked = high;
}

void NumberGenerator::GenerateMultiplesInRange(unsigned int n, int rangeSize) {
  unsigned long long low = lastChecked + 1;
  unsigned long long high = low + rangeSize - 1;

  // Finds the first multiple of n starting from 'low'
  unsigned long long start = ((low + n - 1) / n) * n;

  for (unsigned long long i = start; i <= high; i += n) {
    AddPoint(i);
  }
  lastChecked = high;
}

void NumberGenerator::Reset() {
  points.clear();
  lastChecked = 0;
}
