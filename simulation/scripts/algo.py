import numpy as np
from math import factorial

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