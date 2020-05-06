import math
import serial
import time

formatterNotes = lambda x: "%04d#" % x
formatterDura = lambda x: "%d*" % x

songCount = 1
titles = ["Twinkle,2 Littl@", "CDEFG@"]
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
for j in range(0, songCount):
	
	for i in range(0, len(notes[j])):
		s.write(bytes(formatterNotes(notes[j][i]), 'UTF-8'))
		time.sleep(waitTime)
	s.write(bytes("!", 'UTF-8'))
	time.sleep(waitTime)

	for i in range(0, len(dura[j])):
		s.write(bytes(formatterDura(dura[j][i]), 'UTF-8'))
		time.sleep(waitTime)
	s.write(bytes("!", 'UTF-8'))
	time.sleep(waitTime)

	for i in range(0, math.ceil(len(titles[j]) / 5)):
		s.write(bytes(titles[j][i*5:i*5+5], 'UTF-8'))
		time.sleep(waitTime)

s.close()
print("Signal sent")
