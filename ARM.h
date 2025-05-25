#ifndef __RAM_H__
#define __RAM_H__

#include <vector>
#include <string>
#include <iostream>

class ARM
{
private:
    std::vector<std::vector<int>> arms = 
        {
            {0, 0, 0}, // Arm 1
            {1, 3, 3}, // Arm 2
            {2, 3, 3}, // Arm 3
            {8, 5, 5}  // Arm 4
        }; // Each arm is a vector of integers
        // {maxDepthm maxCTGs, micAttempts}
public:
    ARM(/* args */);
    ~ARM();

    int get_n_arms() const { return arms.size(); }
    int get_arm_maxDepth(int arm_index) const { 
        return this->arms[arm_index][0]; }
    int get_arm_maxCTGs(int arm_index) const {
        return this->arms[arm_index][1]; }
    int get_arm_micAttempts(int arm_index) const {
        return this->arms[arm_index][2]; 
    }
};






#endif