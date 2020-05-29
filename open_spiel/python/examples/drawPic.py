#  , -* , - coding: utf , -8  , -* , -
# @Time    : 2020/5/24 上午10:25
# @Author  : zhanghao
# @Email   : zhanghao1903@qq.com
# @File    : drawPic.py
# @Software: PyCharm

import matplotlib.pyplot as plt
import numpy as np
random_random = np.array([
    [0.110, -0.084],
    [0.130, -0.168],
    [0.154, -0.148],
    [0.080, -0.152],
    [0.068, -0.124],
    [0.078, -0.076],
    [0.164, -0.146],
    [0.106, -0.112],
    [0.096, -0.114],
    [0.072, -0.094],
    [0.162, -0.062],
    [0.082, -0.118],
    [0.122, -0.122],
    [0.122, -0.066],
    [0.062, -0.184],
    [0.158, -0.122],
    [0.106, -0.066],
    [0.114, -0.130],
    [0.142, -0.058],
    [0.150, -0.158],
    [0.104, -0.178],
    [0.070, -0.142],
])

step_1000 = np.array([
    [0.024, 0.0],
    [-0.032, -0.022],
    [-0.068, 0.004],
    [ 0.052, -0.034],
    [0.022, 0.024],
    [-0.008,  0.008],
    [0.008, 0.042],
    [-0.052, -0.006],
    [-0.002, -0.038],
    [0.002, -0.026],
    [ 0.014, -0.012],
    [0.024, 0.012],
    [-0.068, -0.016],
    [0.01, 0.01],
    [0.024, 0.012],
    [ 0.034, -0.016],
    [-0.028,  0.04 ],
    [-0.006,  0.048],
    [-0.028,  0.024],
    [0.058, 0.002],
])

axis_x = np.linspace(1, 22, 22)
axis_step = np.linspace(1, 20, 20)
print(axis_x)


random_random_2 = random_random.flatten(order='F').reshape(2, 22)
step_1000_2 = step_1000.flatten(order='F').reshape(2, 20)

# random-random
print("先手和后手的平均数：")
print(np.average(random_random_2, axis=1))
print("先手和后手的最大值：")
print(np.amax(random_random_2, axis=1))
print("先手和后手的最小值：")
print(np.amin(random_random_2, axis=1))
print("先手和后手的中位值：")
print(np.median(random_random_2, axis=1))

# 1000 step
print("1000 step\n")
print("先手和后手的平均数：")
print(np.average(step_1000_2, axis=1))
print("先手和后手的最大值：")
print(np.amax(step_1000_2, axis=1))
print("先手和后手的最小值：")
print(np.amin(step_1000_2, axis=1))
print("先手和后手的中位值：")
print(np.median(step_1000_2, axis=1))

# plt.plot(axis_x, random_random_2[0])
# plt.plot(axis_x, random_random_2[1])

plt.plot(axis_step, step_1000_2[0])
plt.plot(axis_step, step_1000_2[1])
plt.show()