module;
#include <vector>

export module math_logic;

export struct NumberPoint {
  double x, y;
  unsigned long long p;
  float hue;
};

export class NumberGenerator {
public:
  NumberGenerator() = default;

  void GeneratePrimesInRange(int range);
  void GenerateMultiplesInRange(unsigned int n, int range);
  void Reset();

  const std::vector<NumberPoint> &GetPoints() const { return points; }

private:
  void AddPoint(unsigned long long p);

  std::vector<NumberPoint> points;

  // Last number checked on the "is prime/multiple of"
  unsigned long long lastChecked = 0;
};
