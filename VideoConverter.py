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

VIDEO_PATH = args.source
LUMINANCE_CUT = int(args.luminance)
CONVERTED_NAME = VIDEO_PATH.split(".")[0]
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
        currentBit = 0
        byte = 0
        #For each pixel in the resized image
        for row in range(FRAME_SIZE[1]):
            for col in range(FRAME_SIZE[0]):
                pixel = frame[row, col]

                #Write a 1 if the pixel is white enough
                if pixel > LUMINANCE_CUT:
                    byte |= 1

                currentBit += 1

                #If the current byte has been filled
                if currentBit >= 8:
                    outputFile.write(byte.to_bytes(1, "big"))
                    #print(byte)
                    byte = 0
                    currentBit = 0
                else:
                    byte <<= 1

        #Print and shift the final byte
        byte <<= 8 - currentBit - 1
        outputFile.write(byte.to_bytes(1, "big"))

        print("Processed " + str(currentFrame) + " frames of " + str(int(video.get(cv2.CAP_PROP_FRAME_COUNT))))
    else: 
        break

outputFile.close()
