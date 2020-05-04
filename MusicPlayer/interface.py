import math
import serial
import time

formatterNotes = lambda x: "%04d" % x
formatterDura = lambda x: "%d" % x

titles = ["Twinkle,2 Littl@"]
notes = [[261, 261, 392, 392, 440, 440, 392,
          349, 349, 330, 330, 294, 294, 261,
          392, 392, 349, 349, 330, 330, 294,
          392, 392, 349, 349, 330, 330, 294,
          261, 261, 392, 392, 440, 440, 392,
          349, 349, 330, 330, 294, 294, 261]]
dura = [[1, 1, 1, 1, 1, 1, 2,
         1, 1, 1, 1, 1, 1, 2,
         1, 1, 1, 1, 1, 1, 2,
         1, 1, 1, 1, 1, 1, 2,
         1, 1, 1, 1, 1, 1, 2,
         1, 1, 1, 1, 1, 1, 2]]

waitTime = 0.1
serdev = '/dev/ttyACM0'
s = serial.Serial(serdev)

print("Sending signal ...")
for i in range(0, math.ceil(len(titles[0]) / 5)):
	s.write(bytes(titles[0][i*5:i*5+5], 'UTF-8'))
	time.sleep(waitTime)

s.close()
print("Signal sent")
