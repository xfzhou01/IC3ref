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
            // basic
            {0,0,2},
            {0,0,4},

            // balanced
            {1,2,3},
            {1,3,3},
            {1,4,3},

            // deep 
            {2,3,3},
            {2,4,999}
        }; // Each arm is a vector of integers
        // {maxDepth, maxCTGs, micAttempts}
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