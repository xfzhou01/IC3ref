#include "MAB.h"
#include <cmath>
#include <limits>
#include <Eigen/Dense>
#include <fstream> // For file output

MAB::MAB(int n_arms, int ctx_dim, float alpha, float epsilon, 
    bool use_mab, int verbose) 
    : n_arms(n_arms), alpha(alpha), epsilon(epsilon), counts(n_arms, 0), 
    values(n_arms, 0.0), rng(std::random_device{}()), 
    A_inv(n_arms, Eigen::MatrixXd::Identity(ctx_dim, ctx_dim)), 
    theta(n_arms, Eigen::VectorXd::Zero(ctx_dim)),
    A(n_arms, Eigen::MatrixXd::Identity(ctx_dim, ctx_dim)),
    b(n_arms, Eigen::VectorXd::Zero(ctx_dim)),
    lambda(0.1), verbose(verbose)
{
    if (verbose > 0 && use_mab) {
        std::cout << "MAB initialized with " << n_arms << " arms." << std::endl;
    }
    std::random_device rd;
    rng.seed(rd());
    // initialize A_inv, theta, A, and b for each arm
    // for (int i = 0; i < n_arms; ++i) {
    //     A_inv.push_back(Eigen::MatrixXd::Identity(ctx_dim, ctx_dim));
    //     theta.push_back(Eigen::VectorXd::Zero(ctx_dim));
    // }
}

MAB::~MAB() {}

float MAB::calculate_reward(int original_cube_size, 
    int final_cube_size, int po_frame, int pushed_frame)
{
    int size_reduction = original_cube_size - final_cube_size;
    float size_reduction_reward = 0.0;
    if (size_reduction < 0) {
        size_reduction_reward = this->size_reduction_weight * 
            (float) size_reduction * this->mab_growth_penalty_factor; 
    } else {
        size_reduction_reward = this->size_reduction_weight * 
            (float) size_reduction;
    }

    int pushing_power = pushed_frame - po_frame;
    float combined_reward = this->pushing_power_weight * (float) pushing_power + 
        size_reduction_reward;
    
    // Add CSV logging if verbose > 1
    if (verbose > 1) {
        bool write_header = false;
        std::ifstream check_file("mab_reward.csv");
        if (!check_file.good() || check_file.peek() == std::ifstream::traits_type::eof()) {
            write_header = true;
        }
        check_file.close();
        std::ofstream csv_file("mab_reward.csv", std::ios::app);
        if (csv_file.is_open()) {
            if (write_header) {
                csv_file << "original_cube_size,final_cube_size,po_frame,pushed_frame,size_reduction,size_reduction_reward,pushing_power,combined_reward" << std::endl;
            }
            csv_file << original_cube_size << ","
                     << final_cube_size << ","
                     << po_frame << ","
                     << pushed_frame << ","
                     << size_reduction << ","
                     << size_reduction_reward << ","
                     << pushing_power << ","
                     << combined_reward << std::endl;
            csv_file.close();
        }
    }
    return combined_reward;
}

int MAB::select_arm_ucb(const Eigen::VectorXd& context)
{
    int best_arm = 0;
    double max_score = -std::numeric_limits<double>::infinity();

    for (int i = 0; i < n_arms; ++i) {
        const auto& a_inv = A_inv[i];
        const auto& th = theta[i];
        double p_a = th.dot(context);
        double uncertainty = context.transpose() * a_inv * context;
        double ucb_score = p_a + alpha * std::sqrt(std::max(0.0, uncertainty));
        if (ucb_score > max_score) {
            max_score = ucb_score;
            best_arm = i;
        }
    }
    counts[best_arm] += 1;

    // CSV logging for arm selection if verbose > 1
    if (verbose > 1) {
        bool write_header = false;
        std::ifstream check_file("mab_arm_select.csv");
        if (!check_file.good() || check_file.peek() == std::ifstream::traits_type::eof()) {
            write_header = true;
        }
        check_file.close();
        std::ofstream csv_file("mab_arm_select.csv", std::ios::app);
        if (csv_file.is_open()) {
            if (write_header) {
                csv_file << "# context_0=frame, context_1=lemma_len, context_2=depth, context_3=activity, context_4=bias" << std::endl;
                for (int i = 0; i < context.size(); ++i) {
                    csv_file << "context_" << i << ",";
                }
                csv_file << "best_arm" << std::endl;
            }
            for (int i = 0; i < context.size(); ++i) {
                csv_file << context[i] << ",";
            }
            csv_file << best_arm << std::endl;
            csv_file.close();
        }
    }
    return best_arm;
}

void MAB::update(int arm, float reward, const Eigen::VectorXd& context)
{
    // std::vector<Eigen::MatrixXd> A;   // A matrix for each arm
    // std::vector<Eigen::VectorXd> b;   // b vector for each arm
    // double lambda;                    // 正则化参数

    // 1. update matrix A: A = A + x x^T
    A[arm] += context * context.transpose();

    // 2. update vec b: b = b + r * x
    b[arm] += context * reward;

    // 3. Regularize A with lambda*I before inversion
    Eigen::MatrixXd A_reg = A[arm] + lambda * 
        Eigen::MatrixXd::Identity(context.size(), context.size());

    // 4. Recompute A_inv and theta for the updated arm
    Eigen::FullPivLU<Eigen::MatrixXd> lu_decomp(A_reg);
    if (lu_decomp.isInvertible()) {
        A_inv[arm] = A_reg.inverse();
        theta[arm] = A_inv[arm] * b[arm];
    } else {
        // Handle error - matrix became non-invertible
        A[arm] = Eigen::MatrixXd::Identity(context.size(), context.size());
        A_inv[arm] = Eigen::MatrixXd::Identity(context.size(), context.size());
        b[arm] = Eigen::VectorXd::Zero(context.size());
        theta[arm] = Eigen::VectorXd::Zero(context.size());
        // output a warning or error message
        if (verbose > 0)
            std::cerr << "Warning: Matrix A for arm " << arm << " became non-invertible, resetting to identity." << std::endl;
    }

    if (verbose > 1) {
        // CSV logging for arm update
        bool write_header = false;
        std::ifstream check_file("mab_arm_update.csv");
        if (!check_file.good() || check_file.peek() == std::ifstream::traits_type::eof()) {
            write_header = true;
        }
        check_file.close();
        std::ofstream csv_file("mab_arm_update.csv", std::ios::app);
        if (csv_file.is_open()) {
            if (write_header) {
                csv_file << "# context_0=frame, context_1=lemma_len, context_2=depth, context_3=activity, context_4=bias" << std::endl;
                for (int i = 0; i < context.size(); ++i) {
                    csv_file << "context_" << i << ",";
                }
                csv_file << "arm,reward" << std::endl;
            }
            for (int i = 0; i < context.size(); ++i) {
                csv_file << context[i] << ",";
            }
            csv_file << arm << "," << reward << std::endl;
            csv_file.close();
        }
        std::cout << "Updated arm " << arm 
                  << ": counts = " << counts[arm]
                  << ", average reward = " << values[arm] 
                  << ", current reward = " << reward 
                  << std::endl;
    }
}