import cv2 as cv
import numpy as np

img = cv.imread('opencvteste.jpeg')
# cv.imshow('cat',img)

def rescaleFrame(frame,scale=0.75):
	width = int(frame.shape[1] * scale)
	height = int(frame.shape[0] * scale)
	dimensions = (width,height)

	return cv.resize(frame, dimensions, interpolation=cv.INTER_AREA)

# Reading videos

# blank = np.zeros((250,250,3),dtype='uint8')


##paint picture

# blank[:] = 0,0,0
# cv.rectangle(blank,(50,100),(200,150),(0,204,204),thickness=2)
# cv.imshow('Blank',blank)

# gray = cv.cvtColor(img,cv.COLOR_BGR2GRAY

#BLUR
# blur = cv.GaussianBlur(img,(7,7),cv.BORDER_DEFAULT)


#EDGE CASCADE
canny = cv.Canny(img,125,175)
cv.imshow('as',img)
cv.imshow('name',canny)

# capture = cv.VideoCapture('Pexels Videos 2103099.mp4')
# # capture = cv.VideoCapture(0) #0 corresponde a webcam

# while True:
# 	isTrue,frame = capture.read()

# 	frame_resized = rescaleFrame(frame,0.5)

# 	cv.imshow('Video',frame_resized)

# 	if cv.waitKey(20) & 0xFF==ord('d'):
# 		break

# capture.release()
# cv.destroyAllWindows()

cv.waitKey(0)