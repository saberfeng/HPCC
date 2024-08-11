import numpy as np
from scipy.optimize import minimize


def incast_1f_chernoff():
    m_bytes = 1500  # packet size
    nic_bw = 100*10**9  # 100 Gbps
    smallest_a = m_bytes*8/nic_bw
    initial_guess = [smallest_a, smallest_a*2]

    def backlog_bound(params):
        a, b = params
        theta = 1  # example value
        k = 3 # sigma factor: 2:95.45%, 3:99.73%, 4:99.99366% 
        t = 1  # example time
        x_bytes = 32*10**6  # buffer size
        return np.exp(theta * k * np.sqrt(m_bytes**2 * t * (b - a)**2 / (6 * (a + b)**2)) - x_bytes)
    
    # Objective function to minimize (we want this to be <= 0.01 for 99% assurance)
    def objective(params):
        return backlog_bound(params) - 0.01

       
    # todo: add constraints
    # 1. a<b
    # 2. a>smallest_a
    # 3. A(t)<D 
    result = minimize(objective, initial_guess, bounds=((0, None), (0, None)))

    # Check results
    if result.success:
        optimized_a, optimized_b = result.x
        print(f"Optimized a: {optimized_a}, Optimized b: {optimized_b}")
    else:
        print("Optimization failed:", result.message)


def main():
    incast_1f_chernoff()
    pass


if __name__ == "__main__":
    main()