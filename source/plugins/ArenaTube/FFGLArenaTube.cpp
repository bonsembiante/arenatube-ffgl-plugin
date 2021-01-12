#include "FFGLArenaTube.h"
#include <math.h>//floor
using namespace ffglex;

enum ParamID
{
	PID_PLAYER_URL,
	PID_PLAYER_URL_EVT,
	PID_PLAYER_STATE,
	PID_PLAYER_SEEK_POS,
	PID_PLAYER_SEEK_EVT,
	PID_PLAYER_LOOP_IN_POS,
	PID_PLAYER_LOOP_OUT_POS,
};

static CFFGLPluginInfo PluginInfo(
	PluginFactory< FFGLArenaTube >,// Create method
	"CTAT1",                       // Plugin unique ID
	"Arena Tube",				   // Plugin name
	2,                             // API major version number
	1,                             // API minor version number
	1,                             // Plugin major version number
	000,                           // Plugin minor version number
	FF_SOURCE,                     // Plugin type
	"Embed YouTube videos directly in Resolume Arena",// Plugin description
	"A ffmpeg decoder implementation in alpha state"        // About
);

static const char vertexShaderCode[] = R"(#version 410 core
layout( location = 0 ) in vec4 vPosition;
layout( location = 1 ) in vec2 vUV;

out vec2 uv;

void main()
{
	gl_Position = vPosition;
	uv = vec2(vUV.s, 1.0 - vUV.t);
}
)";

static const char fragmentShaderCode[] = R"(#version 410 core
uniform sampler2D displayTexture;
uniform int blackout;

in vec2 uv;

out vec4 fragColor;

void main()
{
	if( blackout == 0 ){
		fragColor = texture( displayTexture, uv );
	} else {
		fragColor = vec4(0, 0, 0, 1.0);
	}

}
)";

FFGLArenaTube::FFGLArenaTube() {
	video_player = new VideoPlayer();

	SetMinInputs( 0 );
	SetMaxInputs( 0 );

	SetParamInfo( PID_PLAYER_URL, "Url", FF_TYPE_TEXT, "" );
	SetParamInfof( PID_PLAYER_URL_EVT, "Set url", FF_TYPE_EVENT );
	SetOptionParamInfo( PID_PLAYER_STATE, "Player", 3, VIDEO_PLAYER_STATES::PLAYING );
	SetParamElementInfo( PID_PLAYER_STATE, 0, "Play", VIDEO_PLAYER_STATES::PLAYING );
	SetParamElementInfo( PID_PLAYER_STATE, 1, "Pause", VIDEO_PLAYER_STATES::PAUSED );
	SetParamElementInfo( PID_PLAYER_STATE, 2, "Stop", VIDEO_PLAYER_STATES::STOPPED );
	SetParamInfof( PID_PLAYER_SEEK_POS, "Seek position", FF_TYPE_STANDARD );
	SetParamInfof( PID_PLAYER_SEEK_EVT, "Set seek", FF_TYPE_EVENT );
	SetParamInfof( PID_PLAYER_LOOP_IN_POS, "Loop in", FF_TYPE_STANDARD );
	SetParamInfof( PID_PLAYER_LOOP_OUT_POS, "Loop out", FF_TYPE_STANDARD );

	FFGLLog::LogToHost( "Created FFGLArenaTube" );
}

FFResult FFGLArenaTube::InitGL( const FFGLViewportStruct* vp )
{
	if( !shader.Compile( vertexShaderCode, fragmentShaderCode ) )
	{
		DeInitGL();
		return FF_FAIL;
	}
	if( !quad.Initialise() )
	{
		DeInitGL();
		return FF_FAIL;
	}

	ScopedShaderBinding shaderBinding( shader.GetGLID() );

	glGenTextures( 1, &displayTextureID );
	Scoped2DTextureBinding textureBinding( displayTextureID );

	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );

	return CFFGLPlugin::InitGL( vp );
}
FFResult FFGLArenaTube::ProcessOpenGL( ProcessOpenGLStruct* pGL )
{
	// Check for state update
	if( new_player_url )
	{
		int old_state = video_player->state;

		video_player->stop();
		video_player->open_file( file_url.c_str() );
		if (old_state == VIDEO_PLAYER_STATES::PLAYING) {
			video_player->play();
		}


		new_player_url = false;
	}

	// If no video loaded, return
	if (!video_player->loaded) {
		return FF_SUCCESS;
	}

	ScopedShaderBinding shaderBinding( shader.GetGLID() );
	ScopedSamplerActivation samplerBinding( 0 );
	Scoped2DTextureBinding textureBinding( displayTextureID );

	// Check for state update
	if( new_player_state > -1 )
	{
		switch( (VIDEO_PLAYER_STATES)new_player_state )
		{
		case VIDEO_PLAYER_STATES::PLAYING:
			video_player->play();
			break;
		case VIDEO_PLAYER_STATES::PAUSED:
			video_player->pause();
			break;
		case VIDEO_PLAYER_STATES::STOPPED:
			video_player->stop();
			break;
		}
		new_player_state = -1;
	}

	// Check for seek update
	if( new_player_seek > -1 )
	{
		video_player->seek_video( std::chrono::milliseconds( new_player_seek ) );

		// Show next frame after seek
		if( video_player->get_video_frame( &frame_width, &frame_height, &frame_data, true ) )
		{
			glTexImage2D( GL_TEXTURE_2D, 0, GL_RGB, frame_width, frame_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, frame_data );
		}

		new_player_seek = -1;
	}


	// Normal play
	if( video_player->state == VIDEO_PLAYER_STATES::PLAYING ) {
		if( video_player->get_video_frame( &frame_width, &frame_height, &frame_data ) )
		{
			glTexImage2D( GL_TEXTURE_2D, 0, GL_RGB, frame_width, frame_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, frame_data );
		}
	}

	glUniform1i( shader.FindUniform( "displayTexture" ), 0 );
	glUniform1i( shader.FindUniform( "blackout" ), (int) video_player->state == VIDEO_PLAYER_STATES::STOPPED );

	quad.Draw();

	return FF_SUCCESS;
}

FFResult FFGLArenaTube::DeInitGL()
{
	shader.FreeGLResources();
	quad.Release();

	return FF_SUCCESS;
}

void FFGLArenaTube::fork_set_raw_youtube_url()
{
	url_req_thread = new std::thread( &FFGLArenaTube::set_raw_youtube_url, this );
}

void FFGLArenaTube::set_raw_youtube_url()
{
	file_url = url_get_youtube_mp4( raw_file_url.c_str() );
	new_player_url = true;
}

int FFGLArenaTube::sprintf_duration( char* buffer, float value )
{
	float duration_seconds = value * video_player->get_duration() / 1000;
	int minutes            = duration_seconds / 60;
	if (minutes > 59) {
		int hours   = duration_seconds / 60 / 60;
		minutes     = duration_seconds / 60 - hours * 60; // mod
		int seconds = duration_seconds - minutes * 60 - hours * 60 * 60;// mod
		return sprintf( buffer, "%0*d:%0*d:%0*d", 2, hours, 2, minutes, 2, seconds );
	}
	else
	{
		int seconds = duration_seconds - minutes * 60;// mod
		return sprintf( buffer, "%0*d:%0*d", 2, minutes, 2, seconds );
	}
}

char* FFGLArenaTube::GetParameterDisplay( unsigned int index )
{
	/**
	 * We're not returning ownership over the string we return, so we have to somehow guarantee that
	 * the lifetime of the returned string encompasses the usage of that string by the host. Having this static
	 * buffer here keeps previously returned display string alive until this function is called again.
	 * This happens to be long enough for the hosts we know about.
	 */
	static char displayValueBuffer[ 15 ];
	memset( displayValueBuffer, 0, sizeof( displayValueBuffer ) );

	float value;
	switch( index )
	{
	case PID_PLAYER_SEEK_POS:
		if (!video_player->loaded) {
			return "00:00";
		}
		sprintf_duration( displayValueBuffer, seek_pos );
		return displayValueBuffer;

	case PID_PLAYER_LOOP_IN_POS:
		if( !video_player->loaded )
		{
			return "00:00";
		}
		value = video_player->get_duration() > 0 ? (float)video_player->loop_in / video_player->get_duration() : 0;
		sprintf_duration( displayValueBuffer, value );
		return displayValueBuffer;

	case PID_PLAYER_LOOP_OUT_POS:
		if( !video_player->loaded )
		{
			return "99:99";
		}
		value = video_player->get_duration() > 0 ? (float)video_player->loop_out / video_player->get_duration() : 1;
		sprintf_duration( displayValueBuffer, (float) video_player->loop_out / video_player->get_duration() );
		return displayValueBuffer;

	default:
		return CFFGLPlugin::GetParameterDisplay( index );
	}
}

FFResult FFGLArenaTube::SetTextParameter( unsigned int dwIndex, const char* value )
{
	switch( dwIndex )
	{
		case PID_PLAYER_URL:
			raw_file_url = std::string( value );
			break;
		default:
			return FF_FAIL;
	}

	return FF_SUCCESS;
}

FFResult FFGLArenaTube::SetFloatParameter( unsigned int dwIndex, float value )
{
	switch( dwIndex )
	{
		case PID_PLAYER_URL_EVT:
			if( value )
			{
				fork_set_raw_youtube_url();
			}
			break;
		case PID_PLAYER_STATE:
			if( video_player->state != (VIDEO_PLAYER_STATES)value )
			{
				new_player_state = value;
			}
			break;
		case PID_PLAYER_SEEK_POS:
			seek_pos        = value;
			break;
		case PID_PLAYER_SEEK_EVT:
			if( value )
			{
				new_player_seek = seek_pos * video_player->get_duration();
			}
			break;
		case PID_PLAYER_LOOP_IN_POS:
			video_player->loop_in = value * video_player->get_duration();
			break;
		case PID_PLAYER_LOOP_OUT_POS:
			video_player->loop_out = value * video_player->get_duration();
			break;
		default:
			return FF_FAIL;
	}

	return FF_SUCCESS;
}

float FFGLArenaTube::GetFloatParameter( unsigned int index )
{
	switch( index )
	{
	case PID_PLAYER_STATE:
		return (float)video_player->state;
	case PID_PLAYER_SEEK_POS:
		return seek_pos;
	case PID_PLAYER_LOOP_IN_POS:
		return video_player->get_duration() > 0 ? video_player->loop_in / video_player->get_duration() : 0;
	case PID_PLAYER_LOOP_OUT_POS:
		return video_player->get_duration() > 0 ? video_player->loop_out / video_player->get_duration() : 1;
	default:
		return 0.0f;
	}
}
