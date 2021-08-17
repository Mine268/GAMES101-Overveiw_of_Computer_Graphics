#include <iostream>
#include <eigen3/Eigen/Core>
#include <eigen3/Eigen/Dense>

int main() {

    float t = sqrtf(2.f) / 2.f;

    Eigen::Vector3f P(2.f, 1.f, 1.f);
    Eigen::Matrix3f M;
    M << t, -t, 1.f, t, t, 2.f, 0.f, 0.f, 1.f;

    std::cout << P << std::endl << M << std::endl;

    std::cout << M * P << std::endl;

    return 0;
}