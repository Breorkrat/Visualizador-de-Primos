module;
#include <cmath>
#include <cstdint>
#include <numbers>
#include <vector>

module math_logic;

void NumberGenerator::AddPoint(uint64_t p) {
  double r = static_cast<double>(p);
  double theta = std::fmod(static_cast<double>(p), 2.0 * std::numbers::pi);

  NumberPoint pt;
  pt.x = static_cast<float>(r * std::cos(theta));
  pt.y = static_cast<float>(-r * std::sin(theta));
  pt.p = static_cast<float>(p);

  points.push_back(pt);
}

void NumberGenerator::GeneratePrimesInRange(int rangeSize) {
  uint64_t low = lastChecked + 1;
  uint64_t high = low + rangeSize - 1;

  // Initializes the list
  std::vector<char> isNotPrime(rangeSize, 0);

  // If on first sieve
  if (low == 1) {
    isNotPrime[0] = 1; // 1 is not prime
    for (uint64_t p = 2; p * p <= high; p++) {
      if (!isNotPrime[p - low]) {
        for (uint64_t j = p * p; j <= high; j += p)
          isNotPrime[j - low] = 1;
      }
    }
  }

  // Sieves based on already calculated primes
  for (const auto &pt : points) {
    uint64_t p = pt.p;
    if (p * p > high)
      break;

    // Finds first multiple of p in range (low, high) truncating the division
    uint64_t start = (low / p) * p;
    if (start < low)
      start += p;

    // Numbers smaller than p*p were already eliminated by past iterations
    if (start < p * p)
      start = p * p;

    // Marks multiples of p as non-prime
    for (uint64_t j = start; j <= high; j += p) {
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
  uint64_t low = lastChecked + 1;
  uint64_t high = low + rangeSize - 1;

  // Finds the first multiple of n starting from 'low'
  uint64_t start = ((low + n - 1) / n) * n;

  for (uint64_t i = start; i <= high; i += n) {
    AddPoint(i);
  }
  lastChecked = high;
}

void NumberGenerator::Reset() {
  points.clear();
  lastChecked = 0;
}
