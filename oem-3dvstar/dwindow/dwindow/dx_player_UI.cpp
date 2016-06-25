#include "dx_player.h"

#include "..\renderer_prototype\PixelShaders\UI.h"

#define FAIL_RET(x) {hr = x; if (FAILED(hr)) return hr;}

#ifndef VSTAR
my_quad quad[200];
inline void p2pquad(my_quad &quad)
{
	quad.vertexes[0].x -= 0.5;
	quad.vertexes[0].y -= 0.5;
	quad.vertexes[1].x -= 0.5;
	quad.vertexes[1].y -= 0.5;
	quad.vertexes[2].x -= 0.5;
	quad.vertexes[2].y -= 0.5;
	quad.vertexes[3].x -= 0.5;
	quad.vertexes[3].y -= 0.5;
}

class vertex_from_element
{
public:
	vertex_from_element(int width, int height)
	{
		m_width = width;
		m_height = height;
	}
	my_quad tovertex(UI_element_warp in)
	{
		my_quad out = {0};
		int x1 = m_width/2;
		int y1 = m_height/2;
		int x2 = m_width/2;
		int y2 = m_height/2;

		if (in.anchor1 & LEFT)
			x1 = 0;
		else if (in.anchor1 & RIGHT)
			x1 = m_width;

		if (in.anchor1 & TOP)
			y1 = 0;
		else if (in.anchor1 & BOTTOM)
			y1 = m_height;

		if (in.anchor2 & LEFT)
			x2 = 0;
		else if (in.anchor2 & RIGHT)
			x2 = m_width;

		if (in.anchor2 & TOP)
			y2 = 0;
		else if (in.anchor2 & BOTTOM)
			y2 = m_height;

		x1 += in.x1;
		x2 += in.x2;
		y1 += in.y1;
		y2 += in.y2;


		int width = abs(x1 - x2);
		int height = abs(y1 - y2);

		out.vertexes[0].x = (float)min(x1,x2); out.vertexes[0].y = (float)min(y1,y2);
		out.vertexes[0].z = 1.0f, out.vertexes[0].w = 1.0f;
		out.vertexes[0].tu = out.vertexes[0].tv = 0;
		out.vertexes[3] = out.vertexes[2] = out.vertexes[1] = out.vertexes[0];

		out.vertexes[1].x += width;  out.vertexes[1].tu = float(width)/in.tx;
		out.vertexes[2].y += height; out.vertexes[2].tv = float(height)/in.ty;
		out.vertexes[3].x += width;  out.vertexes[3].y += height;
		out.vertexes[3].tu = float(width)/in.tx;
		out.vertexes[3].tv = float(height)/in.ty;

		out.vertexes[0].tu = out.vertexes[0].tv = 0;
		out.vertexes[1].tu = 1.0f; out.vertexes[1].tv = 0;
		out.vertexes[2].tu = 0; out.vertexes[2].tv = 1.0f;
		out.vertexes[3].tu = out.vertexes[3].tv = 1.0f;

		p2pquad(out);
		return out;
	}

	my_quad tovertex(UI_element_fixed in)
	{
		if (!in.width)
		{
			in.width = in.cx;
			in.height = in.cy;
		}


		my_quad out = {0};
		int x = m_width/2;
		int y = m_height/2;

		if (in.anchor & LEFT)
			x = 0;
		else if (in.anchor & RIGHT)
			x = m_width;

		if (in.anchor & TOP)
			y = 0;
		else if (in.anchor & BOTTOM)
			y = m_height;

		out.vertexes[0].x = (float)x, out.vertexes[0].y = (float)y;
		out.vertexes[0].z = 1.0f, out.vertexes[0].w = 1.0f;
		out.vertexes[3] = out.vertexes[2] = out.vertexes[1] = out.vertexes[0];
		if (in.anchor & LEFT)
		{
			out.vertexes[1].x += in.width;
			out.vertexes[3].x += in.width;
		}
		else if (in.anchor & RIGHT)
		{
			out.vertexes[0].x -= in.width;
			out.vertexes[2].x -= in.width;
		}
		else
		{
			out.vertexes[0].x -= in.width/2;
			out.vertexes[1].x += in.width/2;
			out.vertexes[2].x -= in.width/2;
			out.vertexes[3].x += in.width/2;
		}

		if (in.anchor & TOP)
		{
			out.vertexes[2].y += in.height;
			out.vertexes[3].y += in.height;
		}
		else if (in.anchor & BOTTOM)
		{
			out.vertexes[0].y -= in.height;
			out.vertexes[1].y -= in.height;
		}
		else
		{
			out.vertexes[0].y -= in.height/2;
			out.vertexes[1].y -= in.height/2;
			out.vertexes[2].y += in.height/2;
			out.vertexes[3].y += in.height/2;
		}

		MyVertex *bmp = out.vertexes;
		bmp[0].tu = 0; bmp[0].tv = 0;
		bmp[1].tu = (float)(in.cx)/in.tx; bmp[1].tv = 0;
		bmp[2].tu = 0; bmp[2].tv = (float)(in.cy)/in.ty;
		bmp[3].tu = (float)(in.cx)/in.tx; bmp[3].tv = (float)(in.cy)/in.ty;
		for(int i=0; i<4; i++)
		{
			bmp[i].tu += (float)in.sx / in.tx;
			bmp[i].tv += (float)in.sy / in.ty;
			bmp[i].x += in.x;
			bmp[i].y += in.y;
		}

		p2pquad(out);
		return out;
	}

protected:
	int m_width, m_height;
};


const int PLAYBUTTON = 100;
const int PAUSEBUTTON = 101;
const int FULLBUTTON = 102;
const int BACKGROUND = 103;
const int PROGRESSBAR = 104;
const int VOLUME = 105;
const int TESTBUTTON = 106;
const int TESTBUTTON2 = 107;

DWORD color_GDI2ARGB(DWORD in);


HRESULT dx_player::init_gpu(int width, int height, IDirect3DDevice9 *device)
{
	invalidate_gpu();

	HRESULT hr;
	if (!m_ui_logo_gpu)
	{
		FAIL_RET(m_Device->CreateTexture(512, 512, 1, D3DUSAGE_AUTOGENMIPMAP, D3DFMT_A8R8G8B8, D3DPOOL_DEFAULT, &m_ui_logo_gpu, NULL));
		FAIL_RET(m_ui_logo_cpu->AddDirtyRect(NULL));
		FAIL_RET(m_Device->UpdateTexture(m_ui_logo_cpu, m_ui_logo_gpu));
	}
	if (!m_ui_tex_gpu)
	{
		FAIL_RET(m_Device->CreateTexture(222, 14, 1, NULL, D3DFMT_A8R8G8B8, D3DPOOL_DEFAULT, &m_ui_tex_gpu, NULL));
		FAIL_RET(m_ui_tex_cpu->AddDirtyRect(NULL));
		FAIL_RET(m_Device->UpdateTexture(m_ui_tex_cpu, m_ui_tex_gpu));
	}
	if (!m_ui_background_gpu)
	{
		FAIL_RET(m_Device->CreateTexture(64, 64, 1, NULL, D3DFMT_A8R8G8B8, D3DPOOL_DEFAULT, &m_ui_background_gpu, NULL));
		FAIL_RET(m_ui_background_cpu->AddDirtyRect(NULL));
		FAIL_RET(m_Device->UpdateTexture(m_ui_background_cpu, m_ui_background_gpu));
	}


	m_Device = device;
	m_width = width;
	m_height = height;
	vertex_from_element convert(width, height);

	// back ground
	back_ground.anchor1 = BOTTOMLEFT;
	back_ground.x1 = back_ground.y1 = 0;
	back_ground.anchor2 = BOTTOMRIGHT;
	back_ground.x2 = 0; back_ground.y2 = -64;
	back_ground.tx = 64; back_ground.ty = 64;

	// progress bar
	progressbar.anchor1 = BOTTOMLEFT;
	progressbar.x1 = 113; progressbar.y1 = -8;
	progressbar.anchor2 = BOTTOMRIGHT;
	progressbar.x2 = -161; progressbar.y2 = -22;
	progressbar.tx = 222; progressbar.ty = 14;

	// volume
	volume.anchor1 = BOTTOMRIGHT;
	volume.x1 = -42; volume.y1 = -8;
	volume.anchor2 = BOTTOMRIGHT;
	volume.x2 = -76; volume.y2 = -22;
	volume.tx = 222; volume.ty = 14;

	// play button
	playbutton.anchor = BOTTOMLEFT;
	playbutton.width = playbutton.height = 0;
	playbutton.x = 14; playbutton.y = -8;
	playbutton.sx = 96;
	playbutton.sy = 0;
	playbutton.cx = 14;
	playbutton.cy = 14;
	playbutton.tx = 222;
	playbutton.ty = 14;
	pausebutton = playbutton;
	pausebutton.sx += 14;

	// full button
	fullbutton = playbutton;
	fullbutton.anchor = BOTTOMRIGHT;
	fullbutton.x = -14; fullbutton.y = -8;
	fullbutton.sx = 192;

	// test button
	test_button = playbutton;
	test_button.sx = test_button.sy = 0;
	test_button.cx = test_button.cy = 512;
	test_button.tx = test_button.ty = 512;
	test_button.anchor = CENTER;
	test_button.width = test_button.height = min(512, min(width,height-40) *3/5);
	test_button.x = test_button.width / 40; test_button.y = -20;

	test_button2 = test_button;
	test_button.x = 0;


	int xmap0[4] = {51, 75, -107, -131};
	for(int i = 0; i<4; i++)
	{
		colon[i].width = colon[i].height = 0;
		colon[i].anchor = i<2 ? BOTTOMLEFT : BOTTOMRIGHT;
		colon[i].x = xmap0[i];
		colon[i].y = -8;
		colon[i].sx = 0;
		colon[i].sy = 0;
		colon[i].cx = 6;
		colon[i].cy = 14;
		colon[i].tx = 222;
		colon[i].ty = 14;
	}


	// times
	int xmap1[5] = {42,57,66,81,90};
	int xmap2[5] = {-138,-123,-114,-99,-90};
	for(int i=0; i<5; i++)
		for(int j=0; j<10; j++)
		{
			current_time[i][j].width = current_time[i][j].height = 0;
			current_time[i][j].anchor = BOTTOMLEFT;
			current_time[i][j].sx = 6+9*j;
			current_time[i][j].sy = 0;
			current_time[i][j].cx = 9;
			current_time[i][j].cy = 14;
			current_time[i][j].tx = 222;
			current_time[i][j].ty = 14;

			current_time[i][j].y = -8;
			current_time[i][j].x = xmap1[i];

			total_time[i][j] = current_time[i][j];
			total_time[i][j].anchor = BOTTOMRIGHT;
			total_time[i][j].x = xmap2[i];
		}


		m_vertex = NULL;
		for(int i=0; i<50; i++)
		{
			quad[i] = convert.tovertex(current_time[i/10][i%10]);
			quad[i+50] = convert.tovertex(total_time[i/10][i%10]);
		}
		quad[PLAYBUTTON] = convert.tovertex(playbutton);
		quad[PAUSEBUTTON] = convert.tovertex(pausebutton);
		quad[FULLBUTTON] = convert.tovertex(fullbutton);
		quad[BACKGROUND] = convert.tovertex(back_ground);
		quad[PROGRESSBAR] = convert.tovertex(progressbar);
		quad[VOLUME] = convert.tovertex(volume);
		quad[TESTBUTTON] = convert.tovertex(test_button);
		quad[TESTBUTTON2] = convert.tovertex(test_button2);
		for (int i=0; i<4; i++)
			quad[150+i] = convert.tovertex(colon[i]);

		m_Device->CreateVertexBuffer( sizeof(quad), D3DUSAGE_DYNAMIC | D3DUSAGE_WRITEONLY, FVF_Flags, D3DPOOL_DEFAULT, &m_vertex, NULL );
		m_Device->CreatePixelShader((DWORD*)g_code_UI, &m_ps_UI);

		void *pVertices = NULL;
		m_vertex->Lock( 0, sizeof(quad), (void**)&pVertices, D3DLOCK_DISCARD );
		memcpy( pVertices, quad, sizeof(quad));
		m_vertex->Unlock();

		return S_OK;
}

HRESULT dx_player::init_cpu(IDirect3DDevice9 *device)
{
	m_Device = device;

	D3DLOCKED_RECT d3dlr;
	HRESULT hr;
	BYTE *dst;
	FILE *f;
	wchar_t raw[MAX_PATH];
	wchar_t apppath[MAX_PATH];
	GetModuleFileNameW(NULL, apppath, MAX_PATH);
	for(int i=wcslen(apppath)-1; i>0; i--)
	{
		if (apppath[i] == L'\\')
		{
			apppath[i] = NULL;
			break;
		}
	}
	wcscat(apppath, L"\\");

	// creation
	if (!m_ui_logo_cpu)
	{
		m_Device->CreateTexture(512, 512, 1, NULL, D3DFMT_A8R8G8B8, D3DPOOL_SYSTEMMEM, &m_ui_logo_cpu, NULL);
		hr = m_ui_logo_cpu->LockRect(0, &d3dlr, 0, NULL);
		dst = (BYTE*)d3dlr.pBits;
		wcscpy(raw, apppath);
		wcscat(raw, L"logo.raw");
		f = _wfopen(raw, L"rb");
		if(!f)
			return E_FAIL;
		for(int i=0; i<512; i++)
		{
			fread(dst, 1, 512*4, f);
			dst += d3dlr.Pitch;		
		}
		fclose(f);

		m_ui_logo_cpu->UnlockRect(0);	
	}
	if (!m_ui_tex_cpu)
	{
		m_Device->CreateTexture(222, 14, 1, NULL, D3DFMT_A8R8G8B8, D3DPOOL_SYSTEMMEM, &m_ui_tex_cpu, NULL);
		hr = m_ui_tex_cpu->LockRect(0, &d3dlr, 0, NULL);
		dst = (BYTE*)d3dlr.pBits;

		wcscpy(raw, apppath);
		wcscat(raw, L"alpha.raw");
		f = _wfopen(raw, L"rb");
		if(!f)
			return E_FAIL;
		for(int i=0; i<14; i++)
		{
			fread(dst, 1, 222*4, f);
			dst += d3dlr.Pitch;		
		}
		fclose(f);
		m_ui_tex_cpu->UnlockRect(0);
	}
	if (!m_ui_background_cpu)
	{
		m_Device->CreateTexture(64, 64, 1, NULL, D3DFMT_A8R8G8B8, D3DPOOL_SYSTEMMEM, &m_ui_background_cpu, NULL);
		HRESULT hr = m_ui_background_cpu->LockRect(0, &d3dlr, 0, NULL);
		for(int i=0; i<64; i++)
		{
			DWORD line_color = i>40?D3DCOLOR_ARGB(180,0,0,0) : D3DCOLOR_ARGB(max(0,i-24)*180/16, 0, 0, 0);
			DWORD* dst = (DWORD*)((BYTE*)d3dlr.pBits + d3dlr.Pitch * i);
			for(int i=0; i<64; i++)
			{
				dst[i] = line_color;
			}
		}
		m_ui_background_cpu->UnlockRect(0);
	}

	m_ui_logo_cpu->AddDirtyRect(NULL);
	m_ui_tex_cpu->AddDirtyRect(NULL);
	m_ui_background_cpu->AddDirtyRect(NULL);

	return S_OK;
}

HRESULT dx_player::invalidate_gpu()
{
	m_ps_UI = NULL;
	m_vertex = NULL;
	m_ui_logo_gpu = NULL;
	m_ui_tex_gpu = NULL;
	m_ui_background_gpu = NULL;
	return S_OK;
}


HRESULT dx_player::invalidate_cpu()
{
	invalidate_gpu();
	m_ui_logo_cpu = NULL;
	m_ui_tex_cpu = NULL;
	m_ui_background_cpu = NULL;
	return S_OK;
}
HRESULT dx_player::draw_ui(IDirect3DSurface9 * surface, int view)
{
	if (!m_file_loaded)
		draw_nonmovie_bg(surface, view);

	int total = 0;
	this->total(&total);
	int current = m_current_time;

	bool showui = m_show_ui && m_theater_owner == NULL;

	float delta_alpha = 1-(float)(timeGetTime()-m_ui_visible_last_change_time)/fade_in_out_time;
	delta_alpha = max(0, delta_alpha);
	delta_alpha = min(1, delta_alpha);
	float alpha = showui ? 1.0f : 0.0f;
	alpha -= showui ? delta_alpha : -delta_alpha;

	if (alpha <=0)
		return S_FALSE;

	if (!surface)
		return E_POINTER;

	HRESULT hr = E_FAIL;

	hr = m_Device->SetPixelShader(NULL);
	m_Device->SetRenderTarget(0, surface);
	m_Device->SetRenderState( D3DRS_ALPHABLENDENABLE, TRUE );
	m_Device->SetRenderState( D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
	m_Device->SetRenderState( D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);
	m_Device->SetRenderState( D3DRS_TEXTUREFACTOR, 0xffffff | (BYTE)(alpha * 255) << 24 );
	m_Device->SetTextureStageState(0, D3DTSS_ALPHAOP, D3DTOP_MODULATE);
	m_Device->SetTextureStageState(0, D3DTSS_ALPHAARG2, D3DTA_TFACTOR);
	m_Device->SetSamplerState(0, D3DSAMP_ADDRESSU, D3DTADDRESS_WRAP);
	m_Device->SetSamplerState(0, D3DSAMP_ADDRESSV, D3DTADDRESS_WRAP);



	m_Device->SetTextureStageState( 0, D3DTSS_COLOROP,   D3DTOP_SELECTARG1 );
	m_Device->SetTextureStageState( 0, D3DTSS_COLORARG1, D3DTA_TEXTURE );
	m_Device->SetTextureStageState( 1, D3DTSS_COLOROP,   D3DTOP_DISABLE );
	hr = m_Device->SetTexture( 0, m_ui_background_gpu );
	hr = m_Device->SetStreamSource( 0, m_vertex, 0, sizeof(MyVertex) );
	hr = m_Device->SetFVF( FVF_Flags );
// 	hr = m_Device->DrawPrimitive( D3DPT_TRIANGLESTRIP, BACKGROUND*4, 2 );
	//m_Device->SetRenderState( D3DRS_ALPHABLENDENABLE, FALSE );

	hr = m_Device->SetTexture( 0, m_ui_tex_gpu );

	for(int i=0; i<2; i++)
	{
		int ms = i?(int)(total) : (int)(current);
		int s = ms / 1000;
		int hour = (s / 3600) % 10;
		int minute1 = (s/60 /10) % 6;
		int minute2 = (s/60) % 10;
		int s1 = (s/10) % 6;
		int s2 = s%10;

		hr = m_Device->DrawPrimitive( D3DPT_TRIANGLESTRIP, i*200+0+4*hour, 2 );
		hr = m_Device->DrawPrimitive( D3DPT_TRIANGLESTRIP, i*200+40+4*minute1, 2 );
		hr = m_Device->DrawPrimitive( D3DPT_TRIANGLESTRIP, i*200+80+4*minute2, 2 );
		hr = m_Device->DrawPrimitive( D3DPT_TRIANGLESTRIP, i*200+120+4*s1, 2 );
		hr = m_Device->DrawPrimitive( D3DPT_TRIANGLESTRIP, i*200+160+4*s2, 2 );
	}

	if(!is_playing())
		hr = m_Device->DrawPrimitive( D3DPT_TRIANGLESTRIP, PLAYBUTTON*4, 2 );
	else
		hr = m_Device->DrawPrimitive( D3DPT_TRIANGLESTRIP, PAUSEBUTTON*4, 2 );
	hr = m_Device->DrawPrimitive( D3DPT_TRIANGLESTRIP, FULLBUTTON*4, 2 );

	hr = m_Device->DrawPrimitive( D3DPT_TRIANGLESTRIP, 600, 2 );
	hr = m_Device->DrawPrimitive( D3DPT_TRIANGLESTRIP, 604, 2 );
	hr = m_Device->DrawPrimitive( D3DPT_TRIANGLESTRIP, 608, 2 );
	hr = m_Device->DrawPrimitive( D3DPT_TRIANGLESTRIP, 612, 2 );


	float ps_const[12] = {	222.0f,  14.0f,  124.0f, 0.0f,
		34.0f, 14.0f, 1.0f, 1.0f, 
		(double)m_volume, alpha, 0.0f, 0.0f};
	// texture width, texture height, top_item_start_x, top_item_start_y
	// item_width, item_height, repeat x, repeat y
	// value1, alpha, reserved..
	hr = m_Device->SetPixelShader(m_ps_UI);
	hr = m_Device->SetPixelShaderConstantF(0, ps_const, 3);
	hr = m_Device->DrawPrimitive( D3DPT_TRIANGLESTRIP, VOLUME*4, 2 );
	hr = m_Device->SetPixelShader(NULL);

	float progress_volue = abs(total) > 1 ? (float)((double)(current) / total) : 0;
	float ps_const2[12] = {	222.0f,  14.0f,  208.5f, 0.0f,
		8.0f, 14.0f, 0.1f, 1.0f, 
		progress_volue, alpha, 0.0f, 0.0f};
	hr = m_Device->SetPixelShader(m_ps_UI);
	hr = m_Device->SetPixelShaderConstantF(0, ps_const2, 3);
	hr = m_Device->DrawPrimitive( D3DPT_TRIANGLESTRIP, PROGRESSBAR*4, 2 );
	hr = m_Device->SetPixelShader(NULL);

	// test button

	// restore alpha op
	m_Device->SetTextureStageState(0, D3DTSS_ALPHAOP, D3DTOP_SELECTARG1);
	return S_OK;
}

HRESULT dx_player::draw_nonmovie_bg(IDirect3DSurface9 *surface, int view)
{
	HRESULT hr = m_Device->SetRenderTarget(0, surface);
	m_Device->SetTextureStageState( 0, D3DTSS_COLOROP,   D3DTOP_SELECTARG1 );
	m_Device->SetTextureStageState( 0, D3DTSS_COLORARG1, D3DTA_TEXTURE );
	m_Device->SetTextureStageState( 1, D3DTSS_COLOROP,   D3DTOP_DISABLE );
	m_Device->SetTextureStageState(0, D3DTSS_ALPHAOP, D3DTOP_SELECTARG1);
	m_Device->SetRenderState( D3DRS_ALPHABLENDENABLE, TRUE );
	m_Device->SetRenderState( D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
	m_Device->SetRenderState( D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);
	hr = m_Device->SetTexture(0, m_ui_logo_gpu);
	hr = m_Device->SetStreamSource( 0, m_vertex, 0, sizeof(MyVertex) );
	hr = m_Device->SetFVF( FVF_Flags );
	//hr = m_Device->DrawPrimitive( D3DPT_TRIANGLESTRIP, (!left_eye ? TESTBUTTON : TESTBUTTON2)*4, 2 );
	hr = m_Device->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2, quad+ (view == 1 ? TESTBUTTON : TESTBUTTON2), sizeof(MyVertex));
	return S_OK;
}

#define rt(x) {*out=(x);return S_OK;}
HRESULT dx_player::hittest(int x, int y, int *out, double *out_value /* = NULL */)
{
	int logo_x = m_width/2;
	int logo_y = m_height/2-20;
	if ((logo_x-x)*(logo_x-x) + (logo_y-y)*(logo_y-y) < test_button2.width/2 * test_button2.height/2)
		rt(hit_logo);

	if (out_value)
		*out_value = 0;

	if (((m_width - 100 <= x && x < m_width)  ||
		(0 <= x && x < 100))
		&& y < m_height - 30
		)
	{
		if (out_value)
		{
			*out_value = (double)(m_height-y) / (m_height-30);
			if(*out_value>1) *out_value = 1;
			if(*out_value<0) *out_value = 0;
		}
		rt(x < 100 ? hit_brightness : hit_volume2);
	}

	y = y - m_height + 30;

	if (y<0 || y>=30 || x<0 || x>m_width)			// 按钮只要大致
		rt(hit_out);

	if (7 <= x && x < 35)
		rt(hit_play);										// play/pause button

	if (m_width - 32 <= x && x < m_width - 7)	// full button
		rt(hit_full);

	if (y<8 || y>=22)									// 滚动条和音量要更加精确
		rt(hit_bg);

	if (m_width - 79 <= x && x < m_width - 39)	// volume  本应该是-76到-42,宽松3个像素来方便0%和100%
	{
		if (out_value)
		{
			*out_value = (double) (x-(m_width-76)) / 33.0;
			if(*out_value>1) *out_value = 1;
			if(*out_value<0) *out_value = 0;
		}
		rt(hit_volume);
	}

	if (110 <= x && x < 113+(m_width-274))	// progress bar 本应该是113到113+width，左边宽松3像素方便0%
	{
		if (out_value)
		{
			*out_value = (double) (x-113) / (m_width-274);
			if(*out_value>1) *out_value = 1;
			if(*out_value<0) *out_value = 0;
		}
		rt(hit_progress);
	}


	*out = 0;
	return S_OK;
}
#endif