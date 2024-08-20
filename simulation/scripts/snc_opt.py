import numpy as np
from scipy.optimize import minimize, LinearConstraint
import matplotlib.pyplot as plt

fig_save_dir = "simulation/scripts"
m_bytes = 1048  # packet size
nic_bw = 100*10**9  # 100 Gbps
smallest_a = m_bytes*8/nic_bw

def incast_1f_chernoff_at_t(t):
    initial_guess = [smallest_a, smallest_a*2]

    def backlog_bound(params):
        a, b = params
        theta = 1.5  # example value
        k = 3 # sigma factor: 2:95.45%, 3:99.73%, 4:99.99366% 
        x_bytes = 32*10**6  # buffer size
        return np.exp(theta * (k * np.sqrt(m_bytes**2 * t * (b - a)**2 / (6 * (a + b)**2)) - x_bytes))
    
    # Objective function to minimize (we want this to be <= 0.01 for 99% assurance)
    def objective(params):
        return backlog_bound(params) - 0.01

    con_a_lt_b = LinearConstraint([[1, -1]], lb=[-np.inf], ub=[0]) # a < b
    con_a_gt_smallest_a = LinearConstraint([[1, 0]], lb=[smallest_a], ub=[np.inf]) # a > smallest_a
    constraints = [con_a_lt_b, con_a_gt_smallest_a]
    result = minimize(objective, initial_guess, constraints=constraints)

    # Check results
    if result.success:
        optimized_a, optimized_b = result.x
        print(f"Optimized a: {optimized_a}, Optimized b: {optimized_b}, bound_prob: {backlog_bound(result.x)}")
        return optimized_a, optimized_b
    else:
        print("Optimization failed:", result.message)


def incast_1f_chernoff():
    t_step = 0.001 # 1ms
    opt_a, opt_b = np.array([]), np.array([])
    t_range = np.arange(0.001, 0.1, t_step)
    for t in t_range:
        a, b = incast_1f_chernoff_at_t(t)
        opt_a = np.append(opt_a, a)
        opt_b = np.append(opt_b, b)
    # plot a, b vs t
    plt.plot(t_range, opt_a, label='a')
    plt.plot(t_range, opt_b, label='b')
    plt.xlabel('Time (s)')
    plt.ylabel('a, b')
    plt.legend()
    plt.savefig(f'{fig_save_dir}/incast_1f_chernoff.png')
    plt.show()

def incast_2f_chernoff_at_t(t):
    initial_guess = [smallest_a, smallest_a*2, smallest_a, smallest_a*2]

    def backlog_bound(params):
        a1, b1, a2, b2 = params
        theta = 1  # example value
        k1, k2 = 3, 3 # sigma factor: 2:95.45%, 3:99.73%, 4:99.99366% 
        x_bytes = 32*10**6  # buffer size
        bound_comp1 = k1 * np.sqrt(m_bytes**2 * t * (b1 - a1)**2 / (6 * (a1 + b1)**2))
        bound_comp2 = k2 * np.sqrt(m_bytes**2 * t * (b2 - a2)**2 / (6 * (a2 + b2)**2))
        return np.exp(theta * (bound_comp1 + bound_comp2 - x_bytes))
    
    # Objective function to minimize (we want this to be <= 0.01 for 99% assurance)
    def objective(params):
        return backlog_bound(params) - 0.01

    con_a1_lt_b1 = LinearConstraint([[1, -1, 0, 0]], lb=[-np.inf], ub=[0]) # a1 < b1
    con_a1_gt_smallest_a = LinearConstraint([[1, 0, 0, 0]], lb=[smallest_a], ub=[np.inf]) # a1 > smallest_a
    con_a2_lt_b2 = LinearConstraint([[0, 0, 1, -1]], lb=[-np.inf], ub=[0]) # a2 < b2
    con_a2_gt_smallest_a = LinearConstraint([[0, 0, 1, 0]], lb=[smallest_a], ub=[np.inf]) # a1 > smallest_a
    constraints = [con_a1_lt_b1, con_a1_gt_smallest_a, con_a2_lt_b2, con_a2_gt_smallest_a]
    result = minimize(objective, initial_guess, constraints=constraints)

    # Check results
    if result.success:
        opt_a1, opt_b1, opt_a2, opt_b2 = result.x
        print(f"Optimized a1: {opt_a1}, Optimized b1: {opt_b1},"
              f"Optimized a2: {opt_a2}, Optimized b2: {opt_b2},"
              f"bound_prob: {backlog_bound(result.x)}")
        return result.x
    else:
        print("Optimization failed:", result.message)

def incast_2f_chernoff():
    t_step = 0.0001 # 1ms
    opt_a1, opt_b1, opt_a2, opt_b2 = \
        np.array([]), np.array([]), np.array([]), np.array([])
    t_range = np.arange(0.0001, 0.1, t_step)
    for t in t_range:
        opt = incast_2f_chernoff_at_t(t)
        opt_a1 = np.append(opt_a1, opt[0])
        opt_b1 = np.append(opt_b1, opt[1])
        opt_a2 = np.append(opt_a2, opt[2])
        opt_b2 = np.append(opt_b2, opt[3])

    # plot a, b vs t
    plt.plot(t_range, opt_a1, label='a1')
    plt.plot(t_range, opt_b1, label='b1')
    plt.plot(t_range, opt_a2, label='a2')
    plt.plot(t_range, opt_b2, label='b2')
    plt.xlabel('Time (s)')
    plt.ylabel('a, b')
    plt.legend()
    plt.savefig(f'{fig_save_dir}/incast_2f_chernoff.png')
    plt.show() 


def incast_Nf_chernoff_at_t(t, N):
    init_guess_a = np.array(smallest_a*np.ones(N))
    init_guess_b = np.array(smallest_a*2*np.ones(N))
    init_guess = np.concatenate((init_guess_a, init_guess_b))

    def backlog_bound(params):
        a, b = params[:N], params[N:]
        theta = 1  # example value
        k = np.array([3]*N) # sigma factor: 2:95.45%, 3:99.73%, 4:99.99366%
        x_bytes = 32*10**6  # buffer size
        sum = (k * np.sqrt(m_bytes**2 * t * (b-a)**2 / (6*(a+b)**2))).sum()
        return np.exp(theta * (sum - x_bytes))
    
    # Objective function to minimize (we want this to be <= 0.01 for 99% assurance)
    def objective(params):
        return backlog_bound(params) - 0.01

    # [[ 1.,  0., -1., -0.],
    #  [ 0.,  1., -0., -1.]]
    con_a_lt_b_mat = np.concatenate((np.eye(N), -np.eye(N)), axis=1)
    con_a_lt_b = LinearConstraint(con_a_lt_b_mat, lb=-np.inf, ub=np.zeros(N)) # a < b <-> a - b < 0
    # [[1. 0. 0. 0.]
    #  [0. 1. 0. 0.]]
    con_a1_gt_smallest_a_mat = np.concatenate((np.eye(N), np.zeros((N, N))), axis=1)
    con_a1_gt_smallest_a = LinearConstraint(con_a1_gt_smallest_a_mat, lb=[smallest_a]*N, ub=np.inf) # a > smallest_a
    constraints = [con_a_lt_b, con_a1_gt_smallest_a]
    result = minimize(objective, init_guess, constraints=constraints)

    # Check results
    if result.success:
        a, b = result.x[:N], result.x[N:]
        print(f"Optimized a: {a}, Optimized b: {b},"
              f"bound_prob: {backlog_bound(result.x)}")
        return result.x
    else:
        print("Optimization failed:", result.message)   


def incast_Nf_chernoff(N=2):
    t_step = 0.0001 # 1ms
    opt_a, opt_b = np.array([]), np.array([])
    t_range = np.arange(0.0001, 0.1, t_step)
    for t in t_range:
        opt = incast_Nf_chernoff_at_t(t, N)
        opt_a = np.append(opt_a, opt[0])
        opt_b = np.append(opt_b, opt[1])

    # plot a, b vs t
    # plt.plot(t_range, opt_a1, label='a1')
    # plt.plot(t_range, opt_b1, label='b1')
    # plt.plot(t_range, opt_a2, label='a2')
    # plt.plot(t_range, opt_b2, label='b2')
    # plt.xlabel('Time (s)')
    # plt.ylabel('a, b')
    # plt.legend()
    # plt.savefig(f'{fig_save_dir}/incast_Nf_chernoff.png')
    # plt.show()

def main():
    # incast_1f_chernoff()
    # incast_2f_chernoff()
    incast_Nf_chernoff(1)
    pass


if __name__ == "__main__":
    main()