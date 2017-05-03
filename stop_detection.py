import numpy as np
import cv2
from picamera.array import PiRGBArray
from picamera import PiCamera
import time
import RPi.GPIO as GPIO

stop_cascade = cv2.CascadeClassifier('stop_cascade_LBP_work2.xml')

#cap = cv2.VideoCapture(0)
# initialize the camera and grab a reference to the raw camera capture
camera = PiCamera()
camera.resolution = (640, 480)
camera.framerate = 32
rawCapture = PiRGBArray(camera, size=(640, 480))

GPIO.setmode(GPIO.BCM)
TRIG = 4

GPIO.setup(TRIG,GPIO.OUT)
# allow the camera to warmup
time.sleep(0.1)

# capture frames from the camera
for frame in camera.capture_continuous(rawCapture, format="bgr", use_video_port=True):
    # grab the raw NumPy array representing the image, then initialize the timestamp
    # and occupied/unoccupied text
    image = frame.array
 
    gray = cv2.cvtColor(image, cv2.COLOR_BGR2GRAY)
    #faces = face_cascade.detectMultiScale(gray, 1.3, 5)
    
    # add this
    # image, reject levels level weights.
    stops = stop_cascade.detectMultiScale(gray)
    
    # add this
    for (x,y,w,h) in stops:
        cv2.rectangle(img,(x,y),(x+w,y+h),(255,255,0),2)
    num = len(stops)
    font = cv2.FONT_HERSHEY_SIMPLEX
    cv2.putText(img,str(num),(20,450), font, 2,(255,255,255),2,cv2.LINE_AA)
    if num > 0:
        GPIO.output(TRIG,GPIO.HIGH)
    else:
        GPIO.output(TRIG,GPIO.LOW)

    cv2.imshow('img',img)
    k = cv2.waitKey(100) & 0xff
    if k == 27:
        break
 
    # clear the stream in preparation for the next frame
    rawCapture.truncate(0)

cap.release()
cv2.destroyAllWindows()