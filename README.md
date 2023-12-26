# CVid: Console Video Player

This simple program can play videos on the Windows CMD or Powershell. In case VLC is just too complex.

## Compilation
Everything is contained in CVid.cpp. Simply compile with **MSVC**.

To set up the Python environment for running the converter script use:
```
pip install requirements.txt
```

## Usage
The program can only play .wav audio and a custom video format. Therefore the included python script VideoConverter.py should be used to generate those files from an mp4 or other video format.

### Convert Video
To convert a video, you first need to set up the environment, then simply run:
```
python VideoConverter.py video.mp4 -h 100
```
The output video and audio will be in the same folder as your source video.<br>
The possible command line arguments are:
- -h or --height: The height of the output video, max is 126
- -w or --width: The width of the output video, max is 240. This is automatically calculated from height and aspect ratio if no value is given
- -l or --luminance: The luminance at which a pixel will be considered on. A higher value means darker pixels will be considered on. Range is 0-256, default is 128
- -a or --audio: Should .wav files also be generated. True or false, default is true

### Play Video
Once you have converted video and audio files, simply place them both in the same directory and run the program:

```
cvid -v path/to/video
```
The possible command line arguments are:
- -v or --video: The path of the video and audio to play, without a file extention.
- -c or --charset: The custom character set to use for rendering. You need to provide three ASCII characters which will represent an upper, a lower, and both pixels. The default is ▀▄█

## TODO
- [x] Implement RLE compression to video format
- [ ] Add ability to load any type of video without converting, possibly through ffmpeg.
- [ ] Add sound playback for that loaded video, possibly through miniaudio.
- [ ] Add 16 colors
