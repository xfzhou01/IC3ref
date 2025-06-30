#ifndef __RAM_H__
#define __RAM_H__

#include <vector>
#include <string>
#include <iostream>
#include <climits>

class ARM
{
private:
    std::vector<std::vector<int>> arms_2 = 
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
    std::vector<std::vector<int>> arms_3 = 
        {
            // basic
            {1,3,2147483647}, // CTG standard
            {1,2,2147483647}, // CTG conservative
            {1,4,2147483647}, // CTG aggressive
            {0,0,2147483647}, // no CTG

            // balanced
            {1,3,3}, // ic3ref
            {0,0,3}, // basic


            // deep 
            {1,1,2147483647} // min CTG
        }; 
        // {maxDepth, maxCTGs, micAttempts}
    std::vector<std::vector<int>> arms = 
        {
            // basic
            {0,0,2147483647}, // no CTG
            {1,2,2147483647}, // CTG conservative
            {1,4,2147483647}, // CTG standard
            {2,8,2147483647}, // CTG aggressive

            // balanced
            {1,3,3}, // ic3ref
            {0,0,3}, // basic


            // deep 
            {1,1,2147483647} // min CTG
        }; 
        // {maxDepth, maxCTGs, micAttempts}
public:
    ARM(/* args */);
    ~ARM();

    int get_n_arms() const {
        std::cout << "[ARM] Number of arms: " << arms.size() << std::endl;
        return arms.size(); }
    int get_arm_maxDepth(int arm_index) const { 
        return this->arms[arm_index][0]; }
    int get_arm_maxCTGs(int arm_index) const {
        return this->arms[arm_index][1]; }
    int get_arm_micAttempts(int arm_index) const {
        return this->arms[arm_index][2]; 
    }
};






#endif