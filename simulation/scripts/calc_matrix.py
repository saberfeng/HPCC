import numpy as np
import argparse

def main():
    parser = argparse.ArgumentParser(description='run simulation')
    parser.add_argument('-r', '--routing_matrix_txt', dest='routing_matrix_txt', 
                        default='', help='input routing matrix file')
    parser.add_argument('-i', '--input_rate_txt', dest='input_rate_txt',
                        default='', help='input flow rate matrix file')
    parser.add_argument('-s', '--service_rate_txt', dest='service_rate_txt',
                        default='', help='input service rate file')
    parser.add_argument('-f', '--flow_num', dest='flow_num', type=int, 
                        default=0, help='number of flows')
    parser.add_argument('-o1', '--output_rho', dest='output_rho', default='', 
                        help='output rho file')
    parser.add_argument('-o2', '--output_nodrop_prob', dest='output_nodrop_prob', default='', 
                        help='output node drop probability file')
    parser.add_argument('-q', '--queue_size_txt', dest='queue_size_txt', default='',
                        help='queue size file')
    args = parser.parse_args()

    if args.routing_matrix_txt is None or args.input_rate_txt is None or\
        args.service_rate_txt is None or args.queue_size_txt is None or\
        args.output_rho is None or args.output_nodrop_prob is None:
        print('Error: input and output file must be specified')
        return

    routing_matrix = np.loadtxt(args.routing_matrix_txt, dtype=float)
    input_rate_Bps = np.loadtxt(args.input_rate_txt, dtype=float)
    input_rate_MBps = input_rate_Bps/10**6
    service_rate_Bps = np.loadtxt(args.service_rate_txt, dtype=float)
    service_rate_MBps = service_rate_Bps/10**6
    queue_size_B = np.loadtxt(args.queue_size_txt, dtype=float)
    queue_size_MB = queue_size_B.astype(int) / 10**6
    flow_num = args.flow_num

    row_num, col_num = routing_matrix.shape
    lambda_ = np.zeros(col_num)
    for flow_i in range(flow_num):
        # routing matrices and input rate verticies are vertically stacked
        R_t = routing_matrix[flow_i*col_num:(flow_i+1)*col_num, :] # flow routing matrix
        if len(input_rate_MBps.shape) == 1:
            gamma_t = input_rate_MBps
        else:
            gamma_t = input_rate_MBps[flow_i, :]
        lambda_t = np.dot(gamma_t, np.linalg.inv(np.eye(col_num)-R_t))
        lambda_ += lambda_t
    
    rho = lambda_ / service_rate_MBps
    
    node_nodrop_prob = np.zeros(col_num)
    rho_power_sum = sum_powers_lowmem_highcomp(rho, queue_size_MB)
    node_nodrop_prob += (1-rho) * rho_power_sum

    np.savetxt(args.output_rho,rho, fmt='%1.9e')
    np.savetxt(args.output_nodrop_prob, node_nodrop_prob, fmt='%1.9e')    

# input:   [1, 1, 2, 3] 
# output: [[0, 0, 0, 0],
#          [1, 1, 1, 1], 
#          [0, 0, 2, 2], 
#          [0, 0, 0, 3]]
def column_range_expansion(v:np.array):
    # Determine the number of rows: maximum number in V
    num_rows = np.max(v)
    M = np.zeros((num_rows+1, len(v)), dtype=int)
    for idx, num in enumerate(v):
        M[:num+1, idx] = np.arange(0, num + 1)
    return M

# input: arr: [2, 2, 2, 2]
#        power: [1, 2, 3, 4]
# output: [ 2  6 14 30] = 
#         [2^0+2^1, 2^0+2^1+2^2, 
#           2^0+2^1+2^2+2^3, 2^0+2^1+2^2+2^3+2^4]
def sum_powers(arr:np.array, power:np.array):
    power_expansion = column_range_expansion(power)
    raw_sum = np.sum(arr**power_expansion, axis=0)
    rows_num = power_expansion.shape[0]
    paddings = rows_num - power - 1 # the number of padding zeros
    return raw_sum - paddings 

def sum_powers_lowmem_highcomp(arr:np.array, power:np.array):
    res = np.zeros(arr.shape[0]) 
    while True:
        has_nonneg_power = False
        for i in range(power.shape[0]):
            if power[i] < 0:
                continue
            else:
                has_nonneg_power = True
                res[i] += arr[i]**power[i]
                power[i] -= 1
        if not has_nonneg_power:
            break
    return res



def test():
    a = np.array([1,2,3,4])

    # save txt
    np.savetxt('test1.txt', a, fmt='%d')
    b = np.loadtxt('test1.txt', dtype=int)
    print(a==b)

    # save dat
    a.tofile('test2.dat')
    c = np.fromfile('test2.dat', dtype=int)
    print(a==c)

    # save npy
    np.save('test3.npy', a)
    d = np.load('test3.npy')
    print(a==d)

    m = np.array([
        [1,2,3.23,.2344],
        [5,6,7,8],
        [9,10,11,12234]])
    # np.savetxt('mt.txt',m,fmt='%1.9e')
    m1 = np.loadtxt('mt.txt', dtype=float)
    print(m1==m)

def test2():
    m2 = np.loadtxt('mt.txt', dtype=float)
    print(m2)
    print(m2[1:3, :])
    print(m2[0,:])

def test3():
    R1 = np.array([
        [0, 1, 0],
        [0, 0, 0.02],
        [0, 0, 0]]) # routing matrix for type 1 customers
    R2 = np.array([
        [0, 0, 1],
        [0, 0, 0,],
        [0, 0.01, 0]])

    # outside input rate for type 1,2 customers
    gamma_1 = np.array([19.25, 0, 0,])
    gamma_2 = np.array([15.75, 0, 0,])

    # service rate
    mu = np.array([120, 10, 3])

    # print(np.eye(3)-R1)
    # print(np.linalg.inv(np.eye(3)-R1))
    lambda_1 = np.dot(gamma_1, np.linalg.inv(np.eye(3)-R1))
    lambda_2 = np.dot(gamma_2, np.linalg.inv(np.eye(3)-R2))
    print(lambda_1)
    print(lambda_2)
    lambda_ = lambda_1 + lambda_2
    print(lambda_)

    rho = lambda_ / mu
    print(rho)

    print(1-rho)
    print(rho**2)

def test_sum_powers():
    a = np.array([2,2,2,2])
    print(sum_powers(a, np.array([1,2,1,2])))
    print(sum_powers(a, np.array([2,2,4,4])))
    print(sum_powers_lowmem_highcomp(a, np.array([1,2,1,2])))
    print(sum_powers_lowmem_highcomp(a, np.array([2,2,4,4])))


if __name__ == '__main__':
    #main()
    # test()
    # test2()
    # test3()
    # test_sum_powers()
    main()
