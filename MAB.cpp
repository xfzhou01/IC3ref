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
    int final_cube_size, int po_frame, int pushed_frame, int ic3_frame_index)
{
    // 1. the calculation of size reduction and pushing power
    float size_reduction = (float)(original_cube_size - final_cube_size);
    float size_reduction_ratio = size_reduction / original_cube_size;
    float push_distance = (float)(pushed_frame - po_frame);
    float max_possible_push = (float)(ic3_frame_index - po_frame + 1);
    float push_ratio = push_distance / max_possible_push;

    // 2. generalization quality calculation
    // 2.1 effective push quality
    bool effective_push = push_distance > 0;
    float push_quality = effective_push ? push_ratio : -0.1; // no push gives negative quality

    // 2.2 Generalization bouns / penalty:
    float generalization_quality = 0;
    if (size_reduction_ratio > 0.5 && push_ratio > 0.3) {
        // both good reduction and push
        generalization_quality = 0.3;
    } else if (size_reduction_ratio > 0.7 && push_ratio < 0.1) {
        // over generalization：reduction +++ but poor push
        generalization_quality = -0.2;
    }

    // 3. bounus calculation is special cases
    float special_bonus = 0;
    // 3.1 extra reward for pushing to the near boundary
    if (ic3_frame_index - push_distance < 2) {
        special_bonus += 0.4;
    }
    // 3.2 extra reward for final cube size being 1
    //    meaning a complete generalization
    if (final_cube_size == 1) {
        special_bonus += 0.2;
    }
    // 3.3 push at high frames (near the boundary of IC3)
    if (po_frame > 0.7 * ic3_frame_index && push_distance > 0) {
        special_bonus += 0.2;
    }

    // 4. reward calculation
    float reward =
        size_reduction_ratio * 0.35 +    // generalization strength
        push_quality * 0.45 +            // push_quality
        generalization_quality +         // bouns/penalty on push and reduction balance
        special_bonus;                   // special bounus
    
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
                csv_file << "original_cube_size,final_cube_size,po_frame,pushed_frame,ic3_frame_index,size_reduction,size_reduction_ratio,push_distance,max_possible_push,push_ratio,push_quality,generalization_quality,special_bonus,reward" << std::endl;
            }
            csv_file << original_cube_size << ","
                     << final_cube_size << ","
                     << po_frame << ","
                     << pushed_frame << ","
                     << ic3_frame_index << ","
                     << size_reduction << ","
                     << size_reduction_ratio << ","
                     << push_distance << ","
                     << max_possible_push << ","
                     << push_ratio << ","
                     << push_quality << ","
                     << generalization_quality << ","
                     << special_bonus << ","
                     << reward << std::endl;
            csv_file.close();
        }
    }
    // 5. tailor into a range of [-0.5, 2.0]
    return std::max(-0.5f, std::min(2.0f, reward));
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
        if (!check_file.good() || check_file.peek() == 
            std::ifstream::traits_type::eof()) {
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
        if (verbose > 0) {
            std::cerr << "Warning: Matrix A for arm " << arm << " became non-invertible, resetting to identity." << std::endl;
            std::cerr << "Context vector: [";
            for (int i = 0; i < context.size(); ++i) {
                std::cerr << context[i];
                if (i != context.size() - 1) std::cerr << ", ";
            }
            std::cerr << "]" << std::endl;
        }
    }

    if (verbose > 1) {
        // CSV logging for arm update
        bool write_header = false;
        std::ifstream check_file("mab_arm_update.csv");
        if (!check_file.good() || check_file.peek() == 
        std::ifstream::traits_type::eof()) {
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