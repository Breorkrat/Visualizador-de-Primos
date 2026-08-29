module;
#include <cstdint>
#include <vector>

export module math_logic;

export struct NumberPoint {
  float x;
  float y;
  float p;
};

export class NumberGenerator {
public:
  NumberGenerator() = default;

  void GeneratePrimesInRange(int range);
  void GenerateMultiplesInRange(unsigned int n, int range);
  void Reset();

  const std::vector<NumberPoint> &GetPoints() const { return points; }
  size_t Size() const { return points.size(); }

private:
  void AddPoint(uint64_t p);

  std::vector<NumberPoint> points;

  // Last number checked on the "is prime/multiple of"
  uint64_t lastChecked = 0;
};
