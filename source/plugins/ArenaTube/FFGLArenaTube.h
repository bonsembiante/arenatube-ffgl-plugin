#pragma once

#include <FFGLSDK.h>
#include <thread>
#include "URLUtils.h"
#include "VideoPlayer.h"

class FFGLArenaTube: public CFFGLPlugin
{
public:
	FFGLArenaTube();

	//CFFGLPlugin
	FFResult InitGL( const FFGLViewportStruct* vp ) override;
	FFResult ProcessOpenGL( ProcessOpenGLStruct* pGL ) override;
	FFResult DeInitGL() override;

	void fork_set_raw_youtube_url();
	void set_raw_youtube_url();

	char* GetParameterDisplay( unsigned int index ) override;

	FFResult SetFloatParameter( unsigned int dwIndex, float value ) override;
	FFResult SetTextParameter( unsigned int dwIndex, const char* value ) override;

	float GetFloatParameter( unsigned int index ) override;

	int sprintf_duration( char* buffer, float value );

private:
	ffglex::FFGLShader shader;
	ffglex::FFGLScreenQuad quad;

	int frame_width, frame_height;
	unsigned char* frame_data;

	GLuint displayTextureID;

	std::thread * url_req_thread;
	std::string raw_file_url = ""; 
	std::string file_url = ""; 
	bool new_player_url = false;

	int new_player_state = -1;

	float seek_pos = -1;
	int new_player_seek = -1;

	VideoPlayer* video_player;
};
