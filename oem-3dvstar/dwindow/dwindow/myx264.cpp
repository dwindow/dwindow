#include "myx264.h"
#include <windows.h>
#pragma comment(lib, "libx264.a")
#pragma comment(lib, "libgcc.a")
#pragma comment(lib, "libmingwex.a")

x264::x264()
:i_pts(0)
,encoder(NULL)
{

}

x264::~x264()
{
	x264_encoder_close(encoder);	
}

HRESULT x264::init(int width, int height, int bitrate)
{
	this->width = width;
	this->height = height;

	x264_param_t param;
	x264_param_default_preset(&param, "medium", "zerolatency");
// 	x264_param_apply_profile(&param, "baseline");
	param.i_frame_reference = 1;
	param.i_width = width;
	param.i_height = height;
	param.i_fps_num = 24000;
	param.i_fps_den = 1001;
	param.i_csp = X264_CSP_I420;

	param.i_keyint_max = 25;
	//param.b_intra_refresh = 1;
	param.b_cabac = 1;
	param.b_annexb = 1;
	param.rc.i_rc_method = X264_RC_ABR;
	param.rc.i_bitrate = bitrate;

	encoder = x264_encoder_open(&param);

	// init picture
	x264_picture_alloc(&pic_in, X264_CSP_I420, width, height);

	last_encode_time = timeGetTime();

	return S_OK;
}
int x264::encode_a_frame(void *data, void*nal_out, bool *IDR)
{
	if (!encoder)
		return E_INVALIDARG;

	pic_in.img.plane[0] = (uint8_t*)data;
	pic_in.img.plane[1] = pic_in.img.plane[0] + width * height;
	pic_in.img.plane[2] = pic_in.img.plane[1] + width * height / 4;


	x264_nal_t *nals;
	int nnal;
	int frame_size = 0;

	pic_in.i_pts = i_pts++;
	pic_in.i_type = IDR && *IDR ? X264_TYPE_IDR : X264_TYPE_AUTO;
	x264_encoder_encode(encoder, &nals, &nnal, &pic_in, &pic_out);

	BYTE *p = ((BYTE*)nal_out) + 4;
	for (x264_nal_t *nal = nals; nal < nals + nnal; nal++) {
		memcpy(p, nal->p_payload, nal->i_payload);
		p += nal->i_payload;
		frame_size += nal->i_payload;

		if (nal->i_type == 5 && IDR)
			*IDR = true;
	}
	last_encode_time = timeGetTime();
	return frame_size;
}
