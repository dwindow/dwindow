#pragma once

typedef enum cmdTag
{
	thumbnail_cmd_open_file,
	thumbnail_cmd_close_file,

	thumbnail_cmd_seek,
	thumbnail_cmd_set_out_format,
	thumbnail_cmd_get_frame,

	thumbnail_cmd_teminate,


	o_thumbnail_cmd_OK,
	o_thumbnail_cmd_EOF,
	o_thumbnail_cmd_UNKNOWN_COMMAND,
	o_thumbnail_cmd_FAIL = -1,
}thumbnail_cmd_code;

typedef struct set_out_format_parameterTAG
{
	int width;
	int height;
	int pixel_format;
} set_out_format_parameter;

typedef struct thumbnail_cmd_tag
{
	thumbnail_cmd_code cmd;
	int data_size;
}thumbnail_cmd;

