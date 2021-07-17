import numpy as np
import matplotlib.pyplot as plt

t_max = 300	#The number of days for which the simulation is carried out

data = np.zeros((t_max,4))
data = np.loadtxt('data.dat',dtype='int')

plt.plot(data[:,0],data[:,1],label='$Sus$')
plt.plot(data[:,0],data[:,2],label='$Inf$')
plt.plot(data[:,0],data[:,3],label='$Rec$')

plt.xlabel('$Number of days$')
plt.ylabel('$Number of nodes$')
plt.legend(loc='best')
plt.grid() 

plt.show()
