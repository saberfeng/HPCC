import numpy as np
from scipy.optimize import minimize, LinearConstraint
import matplotlib.pyplot as plt

fig_save_dir = "simulation/scripts"

def incast_1f_chernoff_at_t(t):
    m_bytes = 1500  # packet size
    nic_bw = 100*10**9  # 100 Gbps
    smallest_a = m_bytes*8/nic_bw
    print("smallest a:", smallest_a)
    initial_guess = [smallest_a, smallest_a*2]

    def backlog_bound(params):
        a, b = params
        theta = 1  # example value
        k = 3 # sigma factor: 2:95.45%, 3:99.73%, 4:99.99366% 
        x_bytes = 32*10**6  # buffer size
        return np.exp(theta * k * np.sqrt(m_bytes**2 * t * (b - a)**2 / (6 * (a + b)**2)) - x_bytes)
    
    # Objective function to minimize (we want this to be <= 0.01 for 99% assurance)
    def objective(params):
        return backlog_bound(params) - 0.01

       
    # todo: add constraints
    # 1. a<b
    # 2. a>smallest_a
    # 3. A(t)<=D 
    con_a_lt_b = LinearConstraint([[1, -1]], lb=[-np.inf], ub=[0]) # a < b
    con_a_gt_smallest_a = LinearConstraint([[1, 0]], lb=[smallest_a], ub=[np.inf]) # a > smallest_a
    constraints = [con_a_lt_b, con_a_gt_smallest_a]
    result = minimize(objective, initial_guess, constraints=constraints)

    # Check results
    if result.success:
        optimized_a, optimized_b = result.x
        print(f"Optimized a: {optimized_a}, Optimized b: {optimized_b}")
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
    

def main():
    incast_1f_chernoff()
    pass


if __name__ == "__main__":
    main()