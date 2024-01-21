#!/usr/bin/env python3
# -*- coding: utf-8 -*-
import time
#from scipy.optimize import curve_fit
#import matplotlib.pyplot as plt
import numpy as np
import sys
__all__=[ ]

def func(x, w, a, k):
    return w * a ** (k * x)

def curvefit(x, y):
    # plt.plot(xdata, ydata, 'b-')
    cur = time.time()
    # print (int(round(cur * 1000)))
    z1 = np.polyfit(x[0], y[0], 1)
    cur1 = time.time()
    # print (int(round(cur1 * 1000)))
    print(time.time() - cur, x[0], y[0])
    return (z1[0], z1[1])
#if __name__ == '__main__':