#pragma  once
#include <d3d9.h>
#include <streams.h>
#include <atlbase.h>
#include "my12doomRendererTypes.h"



typedef enum _anchor_point
{
	TOP = 1,
	BOTTOM = 2,
	LEFT = 4,
	RIGHT = 8,
	CENTER = 0x10,

	TOPLEFT = TOP + LEFT,
	TOPRIGHT = TOP + RIGHT,
	BOTTOMLEFT = BOTTOM + LEFT,
	BOTTOMRIGHT = BOTTOM + RIGHT,
} anchor_point;

typedef enum _hittest_result
{
	hit_out = -1,
	hit_logo = -2,
	hit_volume2 = -3,
	hit_brightness = -4,
	hit_bg = 0,
	hit_button1 = 1,
	hit_button2 = 2,
	hit_button3 = 3,
	hit_button4 = 4,
	hit_play = 10,
	hit_pause = 11,
	hit_full = 12,
	hit_volume = 13,
	hit_progress = 15,
	hit_next = 16,
	hit_previous = 17,
	hit_3d_swtich = 18,
	hit_volume_button = 19,
	hit_stop = 20,
} hittest_result;

typedef struct _UI_element_fixed
{
	void *texture;
	int tx,ty,sx,sy,cx,cy;	// texture source: texture total width, total height, start x, start y, width, height
	int width,height;		// display width	not impl now

	anchor_point anchor;	// position
	int x,y;				// position offset
} UI_element_fixed;

typedef struct _UI_element_wrap
{
	void *texture;
	// float u,v :					// always whole texture
	anchor_point anchor1, anchor2;	// 2 anchor point
	int x1,y1, x2,y2;				// 2 position, only one dimention used per time.
	float tx, ty;
} UI_element_warp;

#define GPU_INITED 1
#define CPU_INITED 2

class ui_drawer_base
{
public:
	ui_drawer_base():m_init_state(0){}
	virtual HRESULT init_gpu(int width, int height,IDirect3DDevice9 *device) PURE;
	virtual HRESULT init_cpu(IDirect3DDevice9 *device) PURE;
	virtual HRESULT invalidate_gpu() PURE;
	virtual HRESULT invalidate_cpu() PURE;
	virtual HRESULT draw_ui(IDirect3DSurface9 *surface, int view) PURE;
	virtual HRESULT draw_nonmovie_bg(IDirect3DSurface9 *surface, int view) PURE;
	virtual HRESULT hittest(int x, int y, int *out, double *outv = NULL) PURE;
	virtual HRESULT pre_render_movie(IDirect3DSurface9 *surface, int view){return S_OK;}

	DWORD m_init_state;
};