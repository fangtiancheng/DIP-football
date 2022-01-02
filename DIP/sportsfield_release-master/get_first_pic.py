import cv2
import os

video_path = "../inputfile/demo.mp4"
if os.path.exists(video_path) == False:
    print("error")

cap = cv2.VideoCapture(video_path)

frame = None
while True:
    _, frame = cap.read()
    if frame is not None:
        break

cv2.imwrite("../trans_out/test_frame.jpg", frame)
