#include <windows.h>
#include <iostream>
#include <fstream>
#include <vector>
#include <algorithm>
#include <cmath>
#include <chrono>
#include <thread>

using namespace std;

//Argparser functions
char* GetOption(char** argv, int argc, const std::string& option)
{
	char** itr = std::find(argv, argv + argc, option);
	if (itr != argv + argc && ++itr != argv + argc)
	{
		return *itr;
	}
	return 0;
}
bool OptionExists(char** argv, int argc, const std::string& option)
{
	return std::find(argv, argv + argc, option) != argv + argc;
}

struct VideoProperties
{
	unsigned short width = 0;
	unsigned short height = 0;
	unsigned short frames = 0;
	unsigned char fps = 0;
};

//Load an image or video from a binary file
vector<byte> LoadData(const string& path, VideoProperties* properties)
{
	//Open the file in binary mode and seek to end
	ifstream ifs(path, ios::binary | ios::ate);

	//Make sure the file can be opened
	if (!ifs)
	{
		cout << "Could not load data from " + path;
		throw runtime_error("Error loading data from " + path);
	}

	//Save end point and go back to start
	streampos end = ifs.tellg();
	ifs.seekg(0, ios::beg);

	//Create a byte vector for the entire file data
	size_t size = size_t(end - ifs.tellg());
	vector<byte> data(size);

	//Copy the data from filestream to vector
	ifs.read((char*)data.data(), data.size());

	//Get the 2-byte dimension, frame count, and fps data
	properties->width = data[0] << 8;
	properties->width |= data[1];
	properties->height = data[2] << 8;
	properties->height |= data[3];
	properties->frames = data[4] << 8;
	properties->frames |= data[5];
	properties->fps = data[6];

	//Remove the dimension, frame count, and fps data
	data.erase(data.begin(), data.begin() + 7);

	return data;
}

int main(int argc, char* argv[])
{
	//Get the video name
	string videoName;
	if (OptionExists(argv, argc, "-v"))
	{
		videoName = GetOption(argv, argc, "-v");
	}
	else
	{
		cout << "Enter the name of the video to play: ";
		cin >> videoName;
	}

	//ASCII characters to draw pixels with, each character has an upper and lower pixel
	char* characters = new char[4];
	//Load defaults
	characters[0] = (char)32;  //Empty
	characters[1] = (char)223; //Top
	characters[2] = (char)220; //Bottom
	characters[3] = (char)219; //Both
	//Load from command line if option was given
	if (OptionExists(argv, argc, "-c"))
	{
		//Load from command line args
		char* charset = GetOption(argv, argc, "-c");
		if (strlen(charset) >= 3)
		{
			//Load from command line
			characters[1] = charset[0]; //Top
			characters[2] = charset[1]; //Bottom
			characters[3] = charset[2]; //Both
		}
	}

	//Load the video data from file
	VideoProperties properties;
	vector<byte> videoData = LoadData(videoName + ".cvid", &properties);

	//Get the console handle
	HANDLE console = GetStdHandle(STD_OUTPUT_HANDLE);
	//Enable virtual terminal for future possibility of 16 colors
	DWORD fdwMode = ENABLE_VIRTUAL_TERMINAL_PROCESSING | ENABLE_PROCESSED_OUTPUT;
	SetConsoleMode(console, fdwMode);

	//Hide the cursor
	CONSOLE_CURSOR_INFO cursorInfo;
	cursorInfo.dwSize = 100;
	cursorInfo.bVisible = false;
	SetConsoleCursorInfo(console, &cursorInfo);

	//Make sure the desired dimensions are valid
	COORD maxWindowSize = GetLargestConsoleWindowSize(console);
	if (properties.width > maxWindowSize.X)
	{
		cout << "The video is too wide! Max is 240" << endl;
		throw runtime_error("The video is too wide!");
	}
	if (properties.height > maxWindowSize.Y * 2)
	{
		cout << "The video is too tall! Max is 126" << endl;
		throw runtime_error("The video is too tall!");
	}

	//Resize the console to fit the frame
	SMALL_RECT consoleSize{ 0, 0, properties.width - 1 , (short)ceil((float)properties.height / 2) - 1 };
	SetConsoleWindowInfo(console, true, &consoleSize);
	if (!SetConsoleScreenBufferSize(console, { (short)properties.width, (short)ceil((float)properties.height / 2) }))
	{
		cout << "Error in SetConsoleScreenBufferSize(). Code " << GetLastError() << endl;
		return GetLastError();
	}
	if (!SetConsoleWindowInfo(console, true, &consoleSize))
	{
		cout << "Error in SetConsoleWindowInfo(). Code " << GetLastError() << endl;
		return GetLastError();
	}

	//Play the audio if provided
	PlaySoundA((LPCSTR)(videoName + ".wav").c_str(), NULL, SND_FILENAME | SND_ASYNC);

	//Get the ms to wait between frames
	auto waitTime = chrono::microseconds((int)((1.f / properties.fps) * 1000000));
	unsigned int frameOffset = 0;
	//For each frame
	for (size_t i = 0; i < properties.frames; i++)
	{
		auto frameStart = chrono::high_resolution_clock::now();

		//Set the cursor to 0, 0
		SetConsoleCursorPosition(console, COORD{ (short)0, (short)0 });

		//Draw the image data
		//Keep track of important bit data
		unsigned int currentBit = 0;
		string frameString = "";
		//For each row going two at a time because each character represents two pixel rows
		for (unsigned short y = 0; y < properties.height; y += 2)
		{
			//Move the cursor to the beginning of the next line
			SetConsoleCursorPosition(console, COORD{ (short)0, (short)ceil((float)y / 2) });

			//For each column
			for (unsigned short x = 0; x < properties.width; x++)
			{
				char toDraw = 0b00000000;

				//A bitmask to check the upper of the pixel pair
				byte topMask = 0b10000000 >> currentBit % 8;
				//A bitmask for checking the lower pixel of the pixel pair
				byte bottomMask = 0b10000000 >> (currentBit + properties.width) % 8;

				//Check the higher pixel
				unsigned int dataIndex = floor((float)currentBit / 8) + frameOffset;
				if ((videoData[dataIndex] & topMask) == topMask)
				{
					toDraw |= 0b0000001;
				}

				//Check the lower pixel
				dataIndex = floor(((float)currentBit + (float)properties.width) / 8) + frameOffset;
				if (y + 1 < properties.height)
				{
					if ((videoData[dataIndex] & bottomMask) == bottomMask)
					{
						toDraw |= 0b0000010;
					}
				}

				//Next bit
				currentBit++;

				//Choose the right pixel to draw
				switch (toDraw)
				{
				case 0b00000011:
					//Draw both
					frameString += characters[3];
					break;
				case 0b00000001:
					//Draw Top
					frameString += characters[1];
					break;
				case 0b00000010:
					//Draw Bottom
					frameString += characters[2];
					break;
				default:
					//Draw Empty
					frameString += characters[0];
					break;
				}
			}
			frameString += "\n";
			//Skip a line since two rows are added in one pass
			currentBit += properties.width;
		}

		//Draw the frame
		WriteConsoleA(console, frameString.c_str(), frameString.size(), NULL, NULL);

		//Shift the data index to the next frame start
		frameOffset += floor((float)(properties.width * properties.height) / 8) + 1;

		//Keep a steady FPS regardless of processing time
		while (true)
		{
			if (waitTime < chrono::high_resolution_clock::now() - frameStart)
				break;
		}
	}

	//Re-enable Cursor
	cursorInfo.bVisible = true;
	SetConsoleCursorInfo(console, &cursorInfo);
	return 0;
}