#!/usr/bin/env python
import matplotlib.pyplot as plt
from scipy.optimize import curve_fit
import numpy as np
import textwrap

####################################################################################

# Defining the function form here.                                                 #

# Note that, by default, we are using 3 Gaussians to fit the data.                 #

# To get rid of any of them, it is simple enough to set some of the 'a's to 0.     #

# Typically, no need to add more Gaussian functions, that will lead to overfitting.#

# Also, when 'n's are set below 2, you are not allowed to have x=0 in your data.   #

# If you need to have x=0, you need to set all the 'n's as integers that beyond 2. #

# However, I recommend to use a very small number like 0.00000001 to replace the 0 #

# and include n = 2 term.                                                          #

####################################################################################

def g(x,a1,a2,a3,b1,b2,b3,n1,n2,n3):

    g1 = a1*np.exp(-b1*x**2)*x**(n1-2)

    g2 = a2*np.exp(-b2*x**2)*x**(n2-2)

    g3 = a3*np.exp(-b3*x**2)*x**(n3-2)

    g_sum = g1+g2+g3
    return g_sum


####################################################################################

# This is an example where we only use 2 Gaussians to fit the given data.          #

####################################################################################

if __name__ == '__main__':

    x = np.linspace(0.000001,0.0005,6) # Here, we need to use a very small number to avoid 0^0 #
    y = [6,4,2.4,1.8,1.4,1.2]
    n1 = 2 # You can also set it to other integers
    n2 = 3 # You can also set it to other integers
    a3 = 0 # This is mandatory because we want 2 Gaussians, therefore the third coefficient is set to 0
    b3 = 0 # This is arbitrary
    n3 = 0 # This is arbitrary

    popt,pcov=curve_fit(lambda x,a1,a2,b1,b2: g(x,a1,a2,a3,b1,b2,b3,n1,n2,n3) ,x,y)

    print(popt)

