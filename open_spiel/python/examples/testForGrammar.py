# -*- coding: utf-8 -*-
# @Time    : 2020/5/15 下午7:09
# @Author  : zhanghao
# @Email   : zhanghao1903@qq.com
# @File    : testForGrammar.py
# @Software: PyCharm

import collections

import tensorflow as tf
print(tf.test.is_gpu_available())
print(tf.test.is_built_with_cuda())

StepOutput = collections.namedtuple("step_output", ["action", "probs"])

ans = StepOutput(action=2, probs=[0.5, 0.5])
print(type(ans))
print(ans)
print(ans.action)
print(ans.probs)
print([ans])

li = [idx for idx in range(2)]
print(li[:])