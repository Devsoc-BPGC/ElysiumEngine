#include <iostream>
#include <vector>
struct point {
  double x, y;
};
std::vector<point> draw() {
  std::vector<point> quad(4);
  for (int i = 0; i < 4; i++) {
    std::cout << "Enter x and y co-ordinates for a point " << (i + 1) << "\n";
    std::cin >> quad[i].x >> quad[i].y;
  }
  return quad;
}
bool checkAABBcollision(const std::vector<point> &quad1,
                        const std::vector<point> &quad2) {
  double min1x = quad1[0].x;
  double min1y = quad1[0].y;
  double min2x = quad2[0].x;
  double min2y = quad2[0].y;
  double max1x = quad1[0].x;
  double max1y = quad1[0].y;
  double max2x = quad2[0].x;
  double max2y = quad2[0].y;
  for (int i = 0; i < 4; i++) {
    if (quad1[i].x < min1x)
      min1x = quad1[i].x;
    if (quad1[i].x > max1x)
      max1x = quad1[i].x;
    if (quad1[i].y < min1y)
      min1y = quad1[i].y;
    if (quad1[i].y > max1y)
      max1y = quad1[i].y;
    if (quad2[i].x < min2x)
      min2x = quad2[i].x;
    if (quad2[i].x > max2x)
      max2x = quad2[i].x;
    if (quad2[i].y < min2y)
      min2y = quad2[i].y;
    if (quad2[i].y > max2y)
      max2y = quad2[i].y;
  }
  bool xoverlap = (min1x <= max2x) && (max1x >= min2x);
  bool yoverlap = (min1y <= max2y) && (max1y >= min2y);
  return xoverlap && yoverlap;
}
bool isQuadIntersect(const std::vector<point> &quad1,
                     const std::vector<point> &quad2) {
  return (checkAABBcollision(quad1, quad2));
}
int main() {
  std::cout << "draw the first quadrilateral\n";
  std::vector<point> quad1 = draw();
  std::cout << "draw the second quadrilateral\n";
  std::vector<point> quad2 = draw();
  if (isQuadIntersect(quad1, quad2))
    std::cout << "they are colliding";
  else
    std::cout << "they are not colliding";
}
