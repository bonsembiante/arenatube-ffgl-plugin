# ArenaTube - Resolume Arena Plugin
This plugin enables the posiblity of mixing YouTube videos directly from the Resolume Arena, without using an external player and an interface like NDI or Spout.

The implementation is done with C++ using the [libavcodec (ffmpeg libraries)](https://www.ffmpeg.org/) and the tool [youtube-dl](https://youtube-dl.org/). The video is loaded and every frame is displayed as a texture in the OpenGL context exposed by Resolume.

The plugin has some known bugs because is an alpha version. At the moment, this project is only a proof of concept that works on my machine, and that is enough for me. I can mix some videos at the same time without problems. The known bugs are at some specific situations and are not critical.

## Download

Check at Releases for a zip containing a Readme + youtube-dl + dll.
The build was done for Windows 10 x64 and it works fine in Resolume Arena 7. No more tests were performed and I don't know if will work okay on other Windows/Arena Versions. Feel free to make any tests you want. I don't think that I would dedicate any effort on keeping the project alive, because I already reached the goal I wanted. 

## Installing

1. Copy the dll to the Resolume Arena's **Extra Effects** folder. It should be on "Documents" -> "Resolume Arena" -> "Extra Effects".

2. Create dir C:\ArenaTube and copy youtube-dl.exe there (The path is hardcoded, so, it's mandatory).

3. Start or restart Resolume Arena.

## Using

The extension enable an ArenaTube source in the software. For detailed info about the use, watch the YouTube tutorial.

## Known bugs

- When changing a clip video to a different one, the time labels are not updated. This happens by Resolume API's limitations (actually, FFGL SDK limitations).
- At the moment, the video frames display is done it by calculating a delta time from the starting playing time and the current time. As result, in a very high cpu usage condition it may have some buffering problems that show frames at a higher speed that normal, but it only happens some times. Using the seek function or clicking Stop and then Play, resets the video position and solve any problem with buffering.
- When seeking, some times the playing speed gets higher during a few frames. I didn't debug it strongly but I'm pretty sure that is related with the video keyframes. When a seeking is performed, not always the selected time has a keyframe. So, we need to select the first keyframe backwards and pass every frame to the codec until we arrive to the desired frame. Maybe something is not working properly there.

## Thanks to

- Bartholomew @bartjoyce, for his great ffmpeg + opengl videos serie, that was very helpful
- ffmpeg
- youtube-dl
- Stackoverflow, always