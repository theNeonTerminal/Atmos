# BASIC FACE DETECTION USING OPENCV AND MEDIAPIPE
# Refer to requirments.txt for version details

import cv2 as cv
import mediapipe as mp
import time as time

class FaceDetect():
    def __init__(self, minDetectionConf=0.7):
        self.minDetectionConf = minDetectionConf
        self.mpFace = mp.solutions.face_detection
        self.face = self.mpFace.FaceDetection(minDetectionConf)  
    def findFaces (self, img, draw=True):
        imgRGB = cv.cvtColor(img, cv.COLOR_BGR2RGB)
        self.results = self.face.process(imgRGB)
        bBoxes = []
        if self.results.detections:
            for id, detection in enumerate(self.results.detections):
                height, width, _ = img.shape
                bBoxC = detection.location_data.relative_bounding_box
                bBox = int(bBoxC.xmin * width), int(bBoxC.ymin * height), \
                    int(bBoxC.width * width), int(bBoxC.height * height)
                score = round(detection.score[0] * 100, 2)
                bBoxes.append([id, bBox, score])
                
                if draw:
                    img = self.fancyDraw(img, bBox)
                    cv.putText(img, f'{str(score)}%', (bBox[0], bBox[1]-20), cv.FONT_HERSHEY_DUPLEX, 1, (0,0,0), 2)
        return img, bBoxes
    def fancyDraw(slef, img, bBox, l=30, t =5, rt = 2, color=(0,0,0), scaleFactor = 15):
        x, y, w, h = bBox         
        x-=scaleFactor 
        y-=scaleFactor
        w+=scaleFactor
        h+=scaleFactor
        
        x1, y1 = w+x+scaleFactor, y+h+scaleFactor 
        
        cv.rectangle(img, bBox, color, rt) 
        cv.line(img, (x, y), (x+l, y), color, t)
        cv.line(img, (x, y), (x, y+l), color, t)
        
        cv.line(img, (x1, y), (x1-l, y), color, t)
        cv.line(img, (x1, y), (x1, y+l), color, t)
        
        cv.line(img, (x, y1), (x+l, y1), color, t)
        cv.line(img, (x, y1), (x, y1-l), color, t)
        
        cv.line(img, (x1, y1), (x1-l, y1), color, t)
        cv.line(img, (x1, y1), (x1, y1-l), color, t)
        
        return img
    
def main():
    cTime, pTime = 0,0
    cap = cv.VideoCapture(0) #? Put Your ESP32 Cam URL HERE
    # 0 means capture first webcam connected to your PC
    detector = FaceDetect()

    while True:
        _, img = cap.read()
        w,h = 1000, 700
        img = cv.resize(img, (w, h))
        img, boxList = detector.findFaces(img)
        if boxList: 
            for _, bBox, _ in boxList: 
                img = detector.fancyDraw(img, bBox, color=(0,255,0))
        
        cTime = time.time()
        fps = 1/(cTime - pTime)
        pTime = cTime
        
        cv.putText(img, f'FPS: {str(int(fps))}', (10, 70), cv.FONT_HERSHEY_DUPLEX, 1.5, (255, 255, 0), 2)
        cv.imshow("VIDEO", img)
        if cv.waitKey(1) & 0xFF == 27:
            break
    
if __name__ == "__main__":
    main()
