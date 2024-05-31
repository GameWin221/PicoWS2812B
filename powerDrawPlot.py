data = [
    0.0, 0.16,
    0.125, 1.18,
    0.25, 2.2,
    0.5, 4.24,
    1.0, 8.32
]

txt = [
    'Off',
    'Eighth',
    'Quarter',
    'Half',
    'Full'
]

import matplotlib.pyplot as plt
import matplotlib.ticker as mticker  
import numpy as np

plt.style.use('_mpl-gallery')

plt.ylabel("Current Draw")
plt.xlabel("Brightness Level")

plt.gca().yaxis.set_major_formatter(mticker.FormatStrFormatter('%.1f A'))

plt.plot(data[0::2], data[1::2], color='red', )

for i in range(0, len(data) // 2):
    plt.annotate(txt[i], (data[i*2 + 0] + 0.015, data[i*2 + 1] + 0.05))
    plt.plot(data[i*2 + 0], data[i*2 + 1], '-ro')

plt.show()