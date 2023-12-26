import sys
import math
import argparse
import cv2
from moviepy.editor import VideoFileClip

#Read command line args
parser = argparse.ArgumentParser(add_help=False)
parser.add_argument('source')
parser.add_argument('-h', '--height', required=True)
parser.add_argument('-w', '--width')
parser.add_argument('-l', '--luminance', default=128)
parser.add_argument('-a', '--audio', default="true")
args = parser.parse_args()

#Set the constant from args
VIDEO_PATH = args.source
CONVERTED_NAME = VIDEO_PATH.split(".")[0]
LUMINANCE_CUT = int(args.luminance)
FRAME_SIZE = 0, 0
MAX_FRAME_SIZE = 240, 126


#Read the source video
video = cv2.VideoCapture(VIDEO_PATH) 

#Automatically calculate width based on aspect ratio
if True and not args.width:
    aspect = video.get(cv2.CAP_PROP_FRAME_WIDTH) / video.get(cv2.CAP_PROP_FRAME_HEIGHT)
    width = math.floor(aspect * int(args.height))
    FRAME_SIZE = width, int(args.height)
#Make sure the video is properly sized
if FRAME_SIZE[0] > MAX_FRAME_SIZE[0] or FRAME_SIZE[1] > MAX_FRAME_SIZE[1]:
    print("Video dimensions are too big!")

#Rip the audio into a wav
if(args.audio.lower() == "true"):
    clip = VideoFileClip(VIDEO_PATH)
    clip.audio.write_audiofile(CONVERTED_NAME + ".wav")

#Create the convered binary video file
outputFile = open(CONVERTED_NAME + ".cvid", "wb") 
#Write the width and height as 2-bytes each
outputFile.write(FRAME_SIZE[0].to_bytes(2, "big"))
outputFile.write(FRAME_SIZE[1].to_bytes(2, "big"))
#Write the frame count as 2-bytes
outputFile.write(int(video.get(cv2.CAP_PROP_FRAME_COUNT)).to_bytes(2, "big"))
#Write the framerate as 1-byte
outputFile.write(int(video.get(cv2.CAP_PROP_FPS)).to_bytes(1, "big"))

currentFrame = 0
runLength = 0
#Video will always start as black (False)
dataState = False
#For each frame of the video
while(True): 
    #Get the frame
    ret,frame = video.read() 

    #If the frame exists
    if ret: 
        #Resize
        frame = cv2.resize(frame, FRAME_SIZE, interpolation= cv2.INTER_LINEAR)
        #Convert to gray-scale 
        frame = cv2.cvtColor(frame, cv2.COLOR_BGR2GRAY) 
        
        currentFrame += 1
        #For each pixel in the resized image
        for row in range(0, FRAME_SIZE[1], 2):
            for col in range(FRAME_SIZE[0]):
                #Two rows at a time
                for i in range(0, 2):
                    #Make sure the pixel is in bounds
                    if row + i >= FRAME_SIZE[1]:
                        break

                    pixel = frame[row + i, col]

                    #If we have counted the maximum runLength of 255
                    if runLength >= 255:
                        dataState = not dataState
                        outputFile.write(runLength.to_bytes(1, "big"))
                        runLength = 0

                    #If the pixel is different than the previous one
                    if (pixel > LUMINANCE_CUT) != dataState:
                        dataState = not dataState
                        outputFile.write(runLength.to_bytes(1, "big"))
                        runLength = 1
                    else:
                        runLength += 1

        print("Processed " + str(currentFrame) + " frames of " + str(int(video.get(cv2.CAP_PROP_FRAME_COUNT))))
    else: 
        break

outputFile.write(runLength.to_bytes(1, "big"))
outputFile.close()
