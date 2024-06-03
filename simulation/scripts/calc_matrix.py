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
    parser.add_argument('-o', '--output', dest='output', default='', 
                        help='output matrix file')
    parser.add_argument('-q', '--queue_size_txt', dest='queue_size_txt', default='',
                        help='queue size file')
    args = parser.parse_args()

    if args.routing_matrix_txt is None or args.input_rate_txt is None or\
        args.service_rate_txt is None or args.size is None or\
        args.output is None:
        print('Error: input and output file must be specified')
        return

    routing_matrix = np.loadtxt(args.routing_matrix_txt, dtype=float)
    input_rate = np.loadtxt(args.input_rate_txt, dtype=float)
    service_rate = np.loadtxt(args.service_rate_txt, dtype=float)
    queue_size = np.loadtxt(args.queue_size_txt, dtype=int)
    flow_num = args.flow_num
    output_file = args.output

    row_num, col_num = routing_matrix.shape
    lambda_ = np.zeros(col_num)
    for flow_i in range(flow_num):
        # routing matrices and input rate verticies are vertically stacked
        R_t = routing_matrix[flow_i*col_num:(flow_i+1)*col_num, :] # flow routing matrix
        gamma_t = input_rate[flow_i, :]
        lambda_t = np.dot(gamma_t, np.linalg.inv(np.eye(col_num)-R_t))
        lambda_ += lambda_t
    
    rho = lambda_ / service_rate
    

    node_drop_prob = np.zeros(col_num)
    queue_size = 100
    for flow_i in range(queue_size):
        node_drop_prob += (1-rho) * rho**flow_i
    
    mat = np.loadtxt(input_file, dtype=int)
    fmt_1_9e = '%1.9e'
    np.savetxt(output_file, mat, fmt=fmt_1_9e)    

# sum of power of each element in target_vec
# target_vec**1 + target_vec**2 + ... + target_vec**power
def sum_powers_v1(target_vec:np.array, power:np.array):
    res_2d = target_vec[:, np.newaxis]
    result_elements = target_vec**power
    print(result_elements)
    result_sum = np.sum(result_elements)
    print(result_sum)
    return result_sum 

def sum_powers(arr:np.array, power:np.array):


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
    print(sum_powers(a, [1,2,1,2]))
    # print(sum_powers(a, 3))

if __name__ == '__main__':
    #main()
    # test()
    # test2()
    # test3()
    test_sum_powers()
