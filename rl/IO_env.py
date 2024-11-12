import gym
from gym import spaces
import numpy as np
from stable_baselines3 import PPO
import random

# Parameters:
Time_wait_cost = 1
Num_process_per_nodes = 10               # 假设每个节点最大I/O进程数为10
Num_nodes = 20                           # 假设有20个节点
Var_size = 4                             # GB
Max_vars = 50
forwarding_speed = {
    1: 3,   # 1个节点对应的速度
    2: 6,   # 2个节点对应的速度
    4: 12,  # 4个节点对应的速度
    8: 24,  # 8个节点对应的速度
    16: 48  # 16个节点对应的速度
}
out_put_speed = {
    1: 1,   # 1个节点对应的速度
    2: 1.5,   # 2个节点对应的速度
    4: 3,  # 4个节点对应的速度
    8: 5,  # 8个节点对应的速度
    16: 8  # 16个节点对应的速度
}

##############################################################################
# IO_process_each_nodes = [0] * Num_nodes  # 初始化每个节点的IO进程为0
# Total_GB = 0
# Total_IO_process = 0
# Num_process_each_var = []
# Forwarding_times = 0
# Forwarding_vars = 0


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
                x = random.randint(0, Num_nodes - 1)  # 随机选择一个节点
                if self.IO_process_each_nodes[x] < Num_process_per_nodes:
                    self.IO_process_each_nodes[x] += 1
                    count_nodes += 1
                    iteration_counts.append(x)  # 添加当前迭代数字
            extra_wait_time = sum(iteration_counts) * Time_wait_cost
            
            
            forwarding_time = Var_size/out_put_speed[b]
            reward = extra_wait_time * 0.1
            # reward = 0
            self.state = np.array(
                [self.Total_IO_process, self.Forwarding_vars])
            done = False
            return self.state, reward, done, {}

        if a == 1:  # output
            if self.Forwarding_vars == 0:
                self.state = np.array(
                    [self.Total_IO_process, self.Forwarding_vars])
                done = True
                reward = -1
                # print(self.state)
                return self.state, reward, done, {}

            print(self.Num_process_each_var)
            minimum_value = min(self.Num_process_each_var)
            speed = out_put_speed[minimum_value]
            output_time = Var_size/speed
            through_put = self.Total_GB/(self.Forwarding_times+output_time)
            reward = through_put
            done = True
            self.state = np.array(
                [self.Total_IO_process, self.Forwarding_vars])
            # print(self.state)
            return self.state, reward, done, {}

    def render(self, action=None, reward=None):
        """可视化当前状态、动作和奖励"""
        print(f"Current state: {self.state[0]:.2f}")
        if action is not None:
            print(f"Action taken: {action}")
        if reward is not None:
            print(f"Reward received: {reward}")


# 创建自定义环境实例
env = IOEnv()

# 初始化PPO模型
model = PPO("MlpPolicy", env,  verbose=1)

# 开始训练模型
model.learn(total_timesteps=10000000)

# 保存模型
model.save("ppo_custom_env")

# 加载模型进行测试
loaded_model = PPO.load("ppo_custom_env")

# 测试模型
# obs = env.reset()
# for _ in range(1000):
#     action, _states = loaded_model.predict(obs)
#     obs, reward, done, info = env.step(action)
#     env.render(action=action, reward=reward)  # 传递当前动作和奖励
#     if done:
#         obs = env.reset()

# count_nodes = 0
# iteration_counts = []
# while count_nodes < b:
#     x = random.randint(0, Num_nodes - 1)  # 随机选择一个节点
#     if IO_process_each_nodes[x] < Num_process_per_nodes:
#         IO_process_each_nodes[x] += 1
#         count_nodes += 1
#         iteration_counts.append(x)  # 添加当前迭代数字
# Time_cost = sum(iteration_counts) * Time_wait_cost
