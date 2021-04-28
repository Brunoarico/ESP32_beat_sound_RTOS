import pydub
import numpy as np
import matplotlib.pyplot as plt

def read(f, normalized=False):
    """MP3 to numpy array"""
    a = pydub.AudioSegment.from_mp3(f)
    y = np.array(a.get_array_of_samples())
    if a.channels == 2:
        y = y.reshape((-1, 2))
    if normalized:
        return a.frame_rate, np.float32(y) / 2**15
    else:
        return a.frame_rate, y

def column(matrix, i):
    return [row[i] for row in matrix]

def translate(value, leftMin, leftMax, rightMin, rightMax):
    # Figure out how 'wide' each range is
    leftSpan = leftMax - leftMin
    rightSpan = rightMax - rightMin

    # Convert the left range into a 0-1 range (float)
    valueScaled = float(value - leftMin) / float(leftSpan)

    # Convert the 0-1 range into a value in the right range.
    return rightMin + (valueScaled * rightSpan)

a = read("testeDD.mp3", normalized=True)[1]

amin = np.min(a)
amax= np.max(a)

print(amax, amin, len(a))
at =[]
for i in a:
    v = int(translate(i,amin, amax,0,255))
    at.append(v)
    print(str(v) +",", end="")

plt.plot(at)
plt.show()
