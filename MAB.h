#ifndef MAB_H
#define MAB_H
#include <vector>
#include <random>
#include <Eigen/Dense>
#include <iostream>

class MAB
{
private:
    int n_arms;
    float alpha; // exploration parameter for UCB
    float epsilon; // exploration rate for epsilon-greedy
    std::vector<int> counts;      // arm pull counts
    std::vector<float> values;    // average rewards for each arm
    std::mt19937 rng;

    // reward estimation parameters
    const float pushing_power_weight = 0.6; // weight for pushing power in reward estimation
    const float size_reduction_weight = 0.4; // weight for pulling power in reward estimation 
    const float mab_growth_penalty_factor = 1.5;

    // LinUCB parameters
    std::vector<Eigen::MatrixXd> A_inv;   // A-1
    std::vector<Eigen::VectorXd> theta;   // theta
    std::vector<Eigen::MatrixXd> A;
    std::vector<Eigen::VectorXd> b;
    double lambda;

    // other parameters
    int verbose; // verbosity level
    
public:
    MAB(int n_arms, int ctx_dim, float alpha = 1.0, float epsilon = 0.1, 
        bool use_mab = false, int verbose = 1);
    ~MAB();
    // reward calculation
    float calculate_reward(int original_cube_size, int final_cube_size,
        int po_frame, int pushed_frame, int ic3_frame_index);


    // LinUCB select
    int select_arm_ucb(const Eigen::VectorXd& context); 

    // LinUCB update
    void update(int arm, float reward, const Eigen::VectorXd& context); 
    int num_arms() const { return n_arms; }
};



#endif