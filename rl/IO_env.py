import gym
from gym import spaces
import numpy as np
from stable_baselines3 import PPO
import random
from collections import Counter

# Parameters:
# The cost of waiting for the next I/O process during forwarding
Time_wait_cost = 0.001

# Maximum number of I/O processes per node
Num_process_per_nodes = 10

# number of Nodes
Num_nodes = 20

# size of each variable (GB)
Var_size = 4

# Max num of vars
Max_vars = 50

# Mapping between the number of processes and the forwarding speed(GB)
forwarding_speed = {
    1: 3.1,
    2: 6,
    4: 11.7,
    8: 20.4,
    16: 23.8,
    32: 24.8
}

# Mapping between the number of processes and the I/O speed(GB)
out_put_speed = {
    1: 0.9,
    2: 1.5,
    4: 3.1,
    8: 5.9,
    16: 8.4,
    32: 9.1
}

# For each additional variable output at the same time, the speed needs to have a discount
discount = 0.95


class IOEnv(gym.Env):
    def __init__(self):
        super(IOEnv, self).__init__()

        self.IO_process_each_nodes = [0] * Num_nodes
        self.Total_GB = 0
        self.Total_IO_process = 0
        self.Num_process_each_var = []
        self.Forwarding_times = 0
        self.Forwarding_vars = 0

        self.action_space = spaces.Discrete(
            2*len(forwarding_speed))
        self.observation_space = spaces.MultiDiscrete(
            [Num_process_per_nodes*Num_nodes, Max_vars])
        self.state = np.array([0, 0])

    def reset(self):

        self.IO_process_each_nodes = [0] * Num_nodes
        self.Total_GB = 0
        self.Total_IO_process = 0
        self.Num_process_each_var = []
        self.Forwarding_times = 0
        self.Forwarding_vars = 0

        self.state = np.array([0, 0])
        return self.state

    def step(self, action):

        a = action % 2
        b_index = action // 2
        b = list(forwarding_speed.keys())[b_index]
        if a == 0:  # forwarding
            if self.Total_IO_process + b >= Num_process_per_nodes*Num_nodes:
                reward = 0
                done = True
                self.state = np.array(
                    [self.Total_IO_process, self.Forwarding_vars])
                return self.state, reward, done, {}

            if self.Forwarding_vars + 1 > Max_vars-1:
                reward = 0
                done = True
                self.state = np.array(
                    [self.Total_IO_process, self.Forwarding_vars])
                return self.state, reward, done, {}

            self.Forwarding_vars = self.Forwarding_vars+1
            self.Num_process_each_var.append(b)
            self.Total_IO_process = self.Total_IO_process + b

            self.Forwarding_times = self.Forwarding_times + \
                Var_size/forwarding_speed[b]

            self.Total_GB = self.Total_GB + Var_size

            count_nodes = 0
            iteration_counts = []
            while count_nodes < b:
                x = random.randint(0, Num_nodes - 1)
                if self.IO_process_each_nodes[x] < Num_process_per_nodes:
                    self.IO_process_each_nodes[x] += 1
                    count_nodes += 1
                    iteration_counts.append(x)
            extra_wait_time = sum(iteration_counts) * Time_wait_cost

            reward = -extra_wait_time
            self.state = np.array(
                [self.Total_IO_process, self.Forwarding_vars])
            done = False
            return self.state, reward, done, {}

        if a == 1:
            if self.Forwarding_vars == 0:
                self.state = np.array(
                    [self.Total_IO_process, self.Forwarding_vars])
                done = True
                reward = -1
                return self.state, reward, done, {}

            # print(self.Num_process_each_var)
            minimum_value = min(self.Num_process_each_var)
            speed = out_put_speed[minimum_value] * \
                (discount ** self.Forwarding_vars)
            output_time = Var_size/speed
            through_put = self.Total_GB/(self.Forwarding_times+output_time)
            reward = through_put
            done = True
            self.state = np.array(
                [self.Total_IO_process, self.Forwarding_vars])
            return self.state, reward, done, {}

    def render(self, action=None, reward=None):
        return self.Num_process_each_var
        # print(f"Current Num_process_each_var: {self.Num_process_each_var}")


env = IOEnv()

model = PPO("MlpPolicy", env,  verbose=1)

model.learn(total_timesteps=50000)

print("#"*50)

state = env.reset()

sets = []
for _ in range(100):
    action, _ = model.predict(state)
    state, reward, done, _ = env.step(action)
    if done:
        sets.append(env.render(action=action, reward=reward))
# print(sets)

sets_as_tuples = [tuple(s) for s in sets]
counter = Counter(sets_as_tuples)
most_common_set = counter.most_common(1)[0]
print(
    f"The best strategy is: {most_common_set[0]} .")

