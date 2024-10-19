import numpy as np
from math import factorial
from enum import Enum, auto

# Define an enumeration class for algorithm names
BEST_SLOT_NUM_MULTI_FACTOR = 'bstSltNumMltFctr'
BEST_SLOT_NUM_LINESPD_SLOT = 'bstSltNumLnspdSlt'


# Function to compute Stirling numbers of the second kind
def stirling_second_kind(n, k):
    if k == 0 or k > n:
        return 0
    if k == n or k == 1:
        return 1
    return k * stirling_second_kind(n - 1, k) + stirling_second_kind(n - 1, k - 1)

def probability_empty_drawers(m, n, x):
    # Number of non-empty drawers
    non_empty_drawers = m - x
    # If there are more empty drawers than drawers or if non-empty drawers are negative
    if x < 0 or x > m or non_empty_drawers < 0:
        return 0
    # Total ways to assign balls to drawers
    total_assignments = m ** n
    # Ways to choose non-empty drawers and assign balls to them
    ways_to_choose = factorial(m) // (factorial(non_empty_drawers) * factorial(m - non_empty_drawers))
    ways_to_assign = factorial(non_empty_drawers) * stirling_second_kind(n, non_empty_drawers)
    # Calculate the probability
    probability = (ways_to_choose * ways_to_assign) / total_assignments
    return probability

def probability_atleast_one_empty_drawer(m, n):
    probability = 0
    for x in range(1, m): # Number of empty drawers from 1 to m - 1
        probability += probability_empty_drawers(m, n, x)
    return probability

def find_best_num_drawers(threshold_prob, num_balls):
    num_drawers = 2
    prob = probability_atleast_one_empty_drawer(num_drawers, num_balls)
    while prob > threshold_prob:
        print(f"Number of drawers: {num_drawers}, atleast one emtpy Probability: {prob:.6f}")
        num_drawers += 1
        prob = probability_atleast_one_empty_drawer(num_drawers, num_balls)
    return num_drawers


class NonEmptySlotCalculator:
    def __init__(self):
        pass

    # the combination of r items from the available n items
    # n! / (r! * (n-r)!)
    def comb(self, r, n):
        return factorial(n) / (factorial(r) * factorial(n-r))

    def prob_any_slot_empty(self, slot_num:int, item_num:int):
        prob = 0
        for i in range(slot_num):
            r = i + 1
            n = slot_num
            positivity = (-1)**i
            prob += positivity * self.comb(r, n) * ((n - r) / n )**item_num 
        return prob

    def prob_all_slot_used(self, slot_num:int, item_num:int):
        return 1 - self.prob_any_slot_empty(slot_num, item_num)

    def find_best_slot_num(self, item_num:int, threshold_prob:float):
        slot_num = 2

def try_frequency(slot_num:int, item_num:int):
    times = 1000
    any_empty = 0
    for i in range(times):
        slots = [0]*slot_num
        for j in range(item_num):
            # pick a random slot
            slots[np.random.randint(0, slot_num)] += 1
        if 0 in slots:
            any_empty += 1
    prob_any_empty = any_empty / times 
    prob_all_used = 1 - prob_any_empty
    print(f"frequency/trials all used: {prob_all_used}")


def find_best_slot_num_for_flows(flow_num:int, thresh_allused_prob:float=0.9):
    slot_num = flow_num
    nonempty_calc = NonEmptySlotCalculator()
    prob = nonempty_calc.prob_all_slot_used(slot_num=slot_num, item_num=flow_num)
    while prob < thresh_allused_prob:
        # print(f"Number of slots: {slot_num}, all slot used Probability: {prob:.6f}")
        slot_num -= 1
        prob = nonempty_calc.prob_all_slot_used(slot_num=slot_num, item_num=flow_num)
    # print(f"Number of slots: {slot_num}, all slot used Probability: {prob:.6f}")
    return slot_num

def test_NonEmptySlotCalculator():
    nonempty_calc = NonEmptySlotCalculator()
    all_slots_used_5_10 = nonempty_calc.prob_all_slot_used(slot_num=5, item_num=20)
    print(f"all_slots_used_5_10: {all_slots_used_5_10}")
    try_frequency(5, 20)
    
    
def test1():
    # Example usage
    m = 10  # Number of drawers
    num_ball = 16 # Number of balls
    x = 1  # Number of empty drawers

    # prob = probability_empty_drawers(m, num_ball, x)
    # print(f"The probability of having exactly {x} empty drawers is: {prob:.6f}")

    # prob = probability_atleast_one_empty_drawer(m, num_ball)
    # print(f"The probability of having at least one empty drawer is: {prob:.6f}")

    threshold_prob = 0.1
    find_best_num_drawers(threshold_prob, num_ball) 

def main():
    # test_NonEmptySlotCalculator()
    find_best_slot_num_for_flows(10)

if __name__ == '__main__':
    main()