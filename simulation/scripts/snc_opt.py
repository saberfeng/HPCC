import numpy as np
from scipy.optimize import minimize, LinearConstraint
import matplotlib.pyplot as plt

fig_save_dir = "simulation/scripts"
m_bytes = 1048  # packet size
nic_bw = 100*10**9  # 100 Gbps
smallest_a = m_bytes*8/nic_bw # 1048bytes 83ns
sw_bw = 100*10**9  # 100 Gbps

# ----------------- Stochastic Network Calculus Deduction Verification------------------

def At_dist_verify():
    a, b = smallest_a, smallest_a*2
    avg_inter_arrival = (a+b)/2
    rate_of_arrivals = 1/avg_inter_arrival
    print(f"Average inter-arrival time: {avg_inter_arrival}s, {avg_inter_arrival*1e6}us")
    print(f"Rate of arrivals: {rate_of_arrivals}/s, {rate_of_arrivals/1e6}/us")
    t_s = 800 * 1e-9
    expected_arrivals_by_t = rate_of_arrivals * t_s
    print(f"Expected arrivals by t: {expected_arrivals_by_t}")
    expected_bytes_by_t = expected_arrivals_by_t * m_bytes
    print(f"Expected bytes by t: {expected_bytes_by_t}")


def arrival_curve(t):
    a, b = smallest_a, smallest_a*2
    k = 3 # sigma factor: 2:95.45%, 3:99.73%, 4:99.99366%
    sigma = k * np.sqrt(m_bytes**2 * t * (b-a)**2 / (6 * (a + b)**2))
    C = k*sigma
    return 2*m_bytes*t / (a+b) + C

def arrival_curve_xy(up_to_t_s, t_step_s):
    up_to_t_ns = int(up_to_t_s * 1e9)
    t_step_ns = int(t_step_s * 1e9)
    x = []
    y = []
    for t_ns in range(0, up_to_t_ns, t_step_ns):
        t_s = t_ns / 1e9
        x.append(t_s)
        y.append(arrival_curve(t_s))
    return x, y

def actual_arrival_process(up_to_t):
    a, b = smallest_a, smallest_a*2
    accumulated_arrivals = []
    cur_t = 0
    arrival_times = []
    while cur_t < up_to_t:
        delay = np.random.uniform(a, b)
        cur_t += delay
        arrival_times.append(cur_t)
    for i in range(len(arrival_times)):
        accumulated_arrivals.append((i+1)*m_bytes)
    return arrival_times, accumulated_arrivals

def test_arrival_curve():
    def exceed_bound_statistics(arr_times, accu_arrivals):
        bound_exceeded = 0
        for i in range(len(accu_arrivals)):
            if accu_arrivals[i] > arrival_curve(arr_times[i]):
                bound_exceeded += 1
        print(f"Bound exceeded: {bound_exceeded}/{len(accu_arrivals)}"
              f" = {bound_exceeded/len(accu_arrivals)}")
    up_to_t = 0.000009 # 5us
    t_step = 0.00000001 #  10ns
    x, y = arrival_curve_xy(up_to_t, t_step)
    plt.plot(x, y, label='Arrival curve')
    arrival_times, accumulated_arrivals = actual_arrival_process(up_to_t)
    exceed_bound_statistics(arrival_times, accumulated_arrivals)

    plt.plot(arrival_times, accumulated_arrivals, 
             label='Actual arrival process') # marker='x' for x markers
    plt.xlabel('Time (s)')
    plt.ylabel('Arrival (bytes)')
    plt.legend()
    plt.savefig(f'{fig_save_dir}/arrival_curves.png')
    plt.show()

def test_At_distribution():
    up_to_t_ns = 83 * 16
    up_to_t_s = up_to_t_ns/1e9
    sample_times = 5000
    Ats = []
    At_freq = {}
    for i in range(sample_times):
        _, accumulated_arrivals = actual_arrival_process(up_to_t_s)
        At = accumulated_arrivals[-1]
        Ats.append(At)
        At_freq[At] = At_freq.get(At, 0) + 1
    # plot a bar for each value
    fig, ax = plt.subplots()
    freq_x = list(At_freq.keys())
    freq_y = list(At_freq.values())
    print(f"freq_x: \n{freq_x}\nfreq_y: \n{freq_y}\n")
    print(f"mean: {np.mean(Ats)}, std: {np.std(Ats)}")
    print(f"std: {np.std(Ats)}")
    ax.bar(freq_x, freq_y, width=500)
    # ax.hist(Ats, bins=10)
    ax.set_xlabel('At(Bytes)')
    ax.set_ylabel('Frequency')
    plt.savefig(f'{fig_save_dir}/At_distribution.png')
    # plt.show()

    a, b = smallest_a, smallest_a*2
    At_normal_u = m_bytes * up_to_t_s * 2 / (a + b)
    At_normal_sigma = np.sqrt(m_bytes**2 * up_to_t_s * (b - a)**2 / 
                                (6 * (smallest_a + smallest_a*2)**2))
    
    At_refined_sigma_nume = m_bytes**2 * up_to_t_s * (b - a)**2 / 12
    At_refined_sigma_denom = ((a+b)/2)**3
    At_refined_sigma = np.sqrt( At_refined_sigma_nume/ At_refined_sigma_denom)
    print(f"Normal distribution: u={At_normal_u}, sigma={At_normal_sigma}, refined_sigma={At_refined_sigma}\n"
          f"1sigma: {At_normal_u - At_normal_sigma} - {At_normal_u + At_normal_sigma}\n"
          f"2sigma: {At_normal_u - 2*At_normal_sigma} - {At_normal_u + 2*At_normal_sigma}\n"
          f"3sigma: {At_normal_u - 3*At_normal_sigma} - {At_normal_u + 3*At_normal_sigma}\n")

# ----------------- Stochastic Network Calculus Optimization ------------------

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
        x_bytes = 16*10**6  # buffer size
        sum = (k * np.sqrt(m_bytes**2 * t * (b-a)**2 / (6*(a+b)**2))).sum()
        return np.exp(theta * (sum - x_bytes))
    
    # Objective function to minimize (we want this to be <= 0.01 for 99% assurance)
    def objective(params):
        return backlog_bound(params) - 0.000001

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
              f"bound_prob: {backlog_bound(result.x)},"
              f"t: {t}")
        return result.x
    else:
        print("Optimization failed:", result.message)   


def incast_Nf_chernoff(N=2):
    t_step = 0.0001 # 0.1ms
    t_start = 0.1  # 0.1s
    t_end = 0.7    # 0.7s
    opt_a, opt_b = np.array([]), np.array([])
    t_range = np.arange(t_start, t_end, t_step)
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


def incast_AtNormal_2f_core(t):
    init_guess = [1, 1, 1, 
                  1, 1, 1, 
                  1, 1, 1, 
                  1, 1, 1,]

    def backlog_bound(params):
        r = sw_bw
        Z = 3 # sigma factor: 2:95.45%, 3:99.73%, 4:99.99366%
        a1, b1, c1, a2, b2, c2, \
            a1_prime, b1_prime, c1_prime, \
            a2_prime, b2_prime, c2_prime = params
        t = (-Z*b2 - Z*b2_prime - b1 - b1_prime + r)/(2*(Z*a2 + Z*a2_prime + a1 + a1_prime))
        mu_1_t = a1 * t**2 + b1 * t + c1
        mu_2_t = a1_prime * t**2 + b1_prime * t + c1_prime
        sigma_1_t = a2 * t**2 + b2 * t + c2
        sigma_2_t = a2_prime * t**2 + b2_prime * t + c2_prime  
        infimum = r * t - mu_1_t - mu_2_t - Z * sigma_1_t - Z * sigma_2_t
        print(f"t: {t}, infimum: {infimum},"
              f"mu_1_t: {mu_1_t}, mu_2_t: {mu_2_t},"
              f"sigma_1_t: {sigma_1_t}, sigma_2_t: {sigma_2_t}")
        theta = 1
        x_bytes = 16*10**6  # buffer size
        bound = np.exp(-theta * (x_bytes + infimum))
        return bound
    
    # Objective function to minimize (we want this to be <= 0.01 for 99% assurance)
    def objective(params):
        return backlog_bound(params) - 0.000001

    ##TODO: add constraints
    # # [[ 1.,  0., -1., -0.],
    # #  [ 0.,  1., -0., -1.]]
    # con_a_lt_b_mat = np.concatenate((np.eye(N), -np.eye(N)), axis=1)
    # con_a_lt_b = LinearConstraint(con_a_lt_b_mat, lb=-np.inf, ub=np.zeros(N)) # a < b <-> a - b < 0
    # # [[1. 0. 0. 0.]
    # #  [0. 1. 0. 0.]]
    # con_a1_gt_smallest_a_mat = np.concatenate((np.eye(N), np.zeros((N, N))), axis=1)
    # con_a1_gt_smallest_a = LinearConstraint(con_a1_gt_smallest_a_mat, lb=[smallest_a]*N, ub=np.inf) # a > smallest_a
    # constraints = [con_a_lt_b, con_a1_gt_smallest_a]
    result = minimize(objective, init_guess)

    # Check results
    if result.success:
        print(f"Optimized params: {result.x},"
              f"bound_prob: {backlog_bound(result.x)},"
              f"t: {t}")
        return result.x
    else:
        print("Optimization failed:", result.message)


def incast_AtNormal_2f():
    t_step = 0.0001 # 0.1ms
    t_start = 0.1  # 0.1s
    t_end = 0.7    # 0.7s
    opt_a, opt_b = np.array([]), np.array([])
    t_range = np.arange(t_start, t_end, t_step)
    for t in t_range:
        opt = incast_AtNormal_2f_core(t)
        opt_a = np.append(opt_a, opt[0])
        opt_b = np.append(opt_b, opt[1])   
        break

def main():
    # incast_1f_chernoff()
    # incast_2f_chernoff()
    # incast_Nf_chernoff(1)
    # test_arrival_curve()
    # test_At_distribution()
    # At_dist_verify()
    incast_AtNormal_2f()
    pass


if __name__ == "__main__":
    main()