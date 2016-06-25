#include "dx_player.h"

#ifdef VSTAR

#define safe_decommit(x) if(x)x->decommit()
#define safe_commit(x) if(x)x->commit()

HRESULT dx_player::init_gpu(int width, int height, IDirect3DDevice9 *device)
{
	safe_commit(m_toolbar_background);
	safe_commit(m_UI_logo);
	for(int i=0; i<9; i++)
		safe_commit(m_buttons[i]);
	for(int i=0; i<11; i++)
		safe_commit(m_numbers[i]);
	for(int i=0; i<6; i++)
		safe_commit(m_progress[i]);
	safe_commit(m_volume_base);
	safe_commit(m_volume_button);

	m_Device = device;
	m_width = width;
	m_height = height;


	return S_OK;
}

HRESULT dx_player::init_cpu(IDirect3DDevice9 *device)
{
	m_Device = device;

	if (m_UI_logo == NULL) m_renderer1->loadBitmap(&m_UI_logo, L"skin\\logo2.jpg");
	if (m_toolbar_background == NULL) m_renderer1->loadBitmap(&m_toolbar_background, L"skin\\toolbar_background.png");
	if (m_volume_base == NULL) m_renderer1->loadBitmap(&m_volume_base, L"skin\\volume_base.png");
	if (m_volume_button == NULL) m_renderer1->loadBitmap(&m_volume_button, L"skin\\volume_button.png");

	wchar_t buttons_pic[9][MAX_PATH] = 
	{
		L"skin\\fullscreen.png",
		L"skin\\volume.png",
		L"skin\\next.png",
		L"skin\\play.png",
		L"skin\\previous.png",
		L"skin\\stop.png",
		L"skin\\3d.png",
		L"skin\\pause.png",
		L"skin\\2d.png",
	};
	for(int i=0; i<9; i++)
		if (m_buttons[i] == NULL) m_renderer1->loadBitmap(&m_buttons[i], buttons_pic[i]);
	for(int i=0; i<11; i++)
	{
		wchar_t tmp[100];
		wsprintfW(tmp, L"skin\\%d.png", i);
		if (m_numbers[i] == NULL) m_renderer1->loadBitmap(&m_numbers[i], tmp);
	}


	wchar_t progress_pics[6][MAX_PATH] = 
	{
		L"skin\\progress_left_base.png",
		L"skin\\progress_center_base.png",
		L"skin\\progress_right_base.png",
		L"skin\\progress_left_top.png",
		L"skin\\progress_center_top.png",
		L"skin\\progress_right_top.png",
	};
	for(int i=0; i<6; i++)
		if (m_progress[i] == NULL) m_renderer1->loadBitmap(&m_progress[i], progress_pics[i]);

	return S_OK;
}

HRESULT dx_player::invalidate_gpu()
{
	safe_decommit(m_toolbar_background);
	safe_decommit(m_UI_logo);
	for(int i=0; i<9; i++)
		safe_decommit(m_buttons[i]);
	for(int i=0; i<11; i++)
		safe_decommit(m_numbers[i]);
	for(int i=0; i<6; i++)
		safe_decommit(m_progress[i]);
	safe_decommit(m_volume_base);
	safe_decommit(m_volume_button);

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

	safe_delete(m_toolbar_background);
	safe_delete(m_UI_logo);
	for(int i=0; i<9; i++)
		safe_delete(m_buttons[i]);
	for(int i=0; i<11; i++)
		safe_delete(m_numbers[i]);
	for(int i=0; i<6; i++)
		safe_delete(m_progress[i]);
	safe_delete(m_volume_base);
	safe_delete(m_volume_button);

	return S_OK;
}

HRESULT dx_player::draw_ui(IDirect3DSurface9 * surface, int view)
{
	if (!m_file_loaded)
		draw_nonmovie_bg(surface, view);

	double UIScale = GET_CONST("UIScale");
	const double button_size = 40 * UIScale * UIScale;
	const double margin_button_right = 32 * UIScale;
	const double margin_button_bottom = 8 * UIScale;
	const double space_of_each_button = 62 * UIScale;
	const double toolbar_height = 65 * UIScale;
	const double width_progress_left = 5 * UIScale;
	const double width_progress_right = 6 * UIScale;
	const double margin_progress_right = 460 * UIScale;
	const double margin_progress_left = 37 * UIScale;
	const double progress_height = 21 * UIScale;
	const double progress_margin_bottom = 27 * UIScale;
	const double volume_base_width = 84 * UIScale;
	const double volume_base_height = 317 * UIScale;
	const double volume_margin_right = (156 - 84) * UIScale;
	const double volume_margin_bottom = (376 - 317) * UIScale;
	const double volume_button_zero_point = 32 * UIScale;
	const double volume_bar_height = 265 * UIScale;
	const double numbers_left_margin = 21 * UIScale;
	const double numbers_right_margin = 455 * UIScale;
	const double numbers_width = 12 * UIScale;
	const double numbers_height = 20 * UIScale;
	const double numbers_bottom_margin = 26;
	const double hidden_progress_width = 72 * UIScale;

	// calculate alpha
	bool showui = m_show_ui && m_theater_owner == NULL;

	float delta_alpha = 1-(float)(timeGetTime()-m_ui_visible_last_change_time)/fade_in_out_time;
	delta_alpha = max(0, delta_alpha);
	delta_alpha = min(1, delta_alpha);
	float alpha = showui ? 1.0f : 0.0f;
	alpha -= showui ? delta_alpha : -delta_alpha;

	// UILayout
	double client_width = m_width;
	double client_height = m_height;

	// toolbar background
	RECTF t = {0, client_height - toolbar_height, client_width, client_height};
	m_renderer1->Draw(surface, m_toolbar_background, NULL, &t, alpha);


	// buttons
	double x = client_width - button_size - margin_button_right;
	double y = client_height - button_size - margin_button_bottom;
	for(int i=0; i<7; i++)
	{
		RECTF rect = {x, y, x+button_size, y+button_size};
		int select = i;
		if (select == 3 && is_playing()) 
			select = 7;
		if (select == 6 && (bool)m_renderer1->m_force2d)
			select = 8;
		m_renderer1->Draw(surface,m_buttons[select], NULL, &rect, alpha);

		x -= space_of_each_button;
	}

	// draw progress
	double x_max = client_width - margin_progress_right;
	double x_min = margin_progress_left;
	double y_min = client_height - progress_margin_bottom;
	int y_max = y_min + progress_height;

	int total_time = 0;
	total(&total_time);

	double value = (double) m_current_time / total_time;

	enum
	{
		left_base,
		center_base,
		right_base,
		left_top,
		center_top,
		right_top,
	};

	// draw progressbar base
	RECTF l = {x_min, y_min, x_min + width_progress_left, y_max};
	m_renderer1->Draw(surface,m_progress[left_base], NULL, &l, alpha);

	RECTF l2 = {x_min + width_progress_left, y_min, x_max-width_progress_right, y_max};
	m_renderer1->Draw(surface,m_progress[center_base], NULL, &l2, alpha);

	RECTF l3 = {x_max-width_progress_right, y_min, x_max, y_max};
	m_renderer1->Draw(surface,m_progress[right_base], NULL, &l3, alpha);

	// draw progressbar top
	double progress_width = x_max - x_min;
	float v = value * progress_width;
	if (v > 1.5)
	{
		RECTF t1 = {x_min, y_min, x_min + min(width_progress_left,v), y_max};
		m_renderer1->Draw(surface,m_progress[left_top], NULL, &t1, alpha);
	}

	if (v > width_progress_left)
	{
		float r = min(x_max-width_progress_right, x_min + v);
		RECTF t2 = {x_min + width_progress_left, y_min, r, y_max};
		m_renderer1->Draw(surface,m_progress[center_top], NULL, &t2, alpha);
	}

	if (v > progress_width - width_progress_right)
	{
		RECTF t3 = {x_max-width_progress_right, y_min, x_max - (progress_width - v), y_max};
		m_renderer1->Draw(surface,m_progress[right_top], NULL, &t3, alpha);
	}


	// draw numbers
	if (m_file_loaded)
	{
		for(int i=0; i<2; i++)
		{
			int ms = i?(int)(total_time) : (int)(m_current_time);
			int s = ms / 1000;
			int hour = (s / 3600) % 100;
			int h1 = hour/10;
			int h2 = hour%10;
			int minute1 = (s/60 /10) % 6;
			int minute2 = (s/60) % 10;
			int s1 = (s/10) % 6;
			int s2 = s%10;

			double left = i ? (m_width - numbers_right_margin - numbers_width * 8)
				: (numbers_left_margin);
			double top = m_height - numbers_bottom_margin - numbers_height;

			// H1H2:M1M2:S1S2
			RECTF r = {left, top, left + numbers_width, top + numbers_height};

			m_renderer1->Draw(surface,m_numbers[h1], NULL, &r, alpha);
			r.left += numbers_width;
			r.right += numbers_width;

			m_renderer1->Draw(surface,m_numbers[h2], NULL, &r, alpha);
			r.left += numbers_width;
			r.right += numbers_width;

			m_renderer1->Draw(surface,m_numbers[10], NULL, &r, alpha);
			r.left += numbers_width;
			r.right += numbers_width;

			m_renderer1->Draw(surface,m_numbers[minute1], NULL, &r, alpha);
			r.left += numbers_width;
			r.right += numbers_width;

			m_renderer1->Draw(surface,m_numbers[minute2], NULL, &r, alpha);
			r.left += numbers_width;
			r.right += numbers_width;

			m_renderer1->Draw(surface,m_numbers[10], NULL, &r, alpha);
			r.left += numbers_width;
			r.right += numbers_width;

			m_renderer1->Draw(surface,m_numbers[s1], NULL, &r, alpha);
			r.left += numbers_width;
			r.right += numbers_width;

			m_renderer1->Draw(surface,m_numbers[s2], NULL, &r, alpha);
			r.left += numbers_width;
			r.right += numbers_width;
		}

	}

	// calculate volume alpha
	delta_alpha = 1-(float)(timeGetTime()-m_volume_visible_last_change_time)/fade_in_out_time;
	delta_alpha = max(0, delta_alpha);
	delta_alpha = min(1, delta_alpha);
	alpha = m_show_volume_bar ? 1.0f : 0.0f;
	alpha -= m_show_volume_bar ? delta_alpha : -delta_alpha;

	// draw volume bottom
	RECTF volume_rect = {client_width - volume_base_width - volume_margin_right, 
		client_height - volume_base_height - volume_margin_bottom };
	volume_rect.right = volume_rect.left + volume_base_width;
	volume_rect.bottom = volume_rect.top + volume_base_height;

	m_renderer1->Draw(surface,m_volume_base, NULL, &volume_rect, alpha);


	// draw volume button
	double ypos = volume_button_zero_point + volume_bar_height * (1-(double)m_volume);
	RECTF button_rect = {volume_rect.left + volume_base_width/2 - button_size/2,  volume_rect.top + ypos - 20};
	button_rect.bottom = button_rect.top + button_size;
	button_rect.right = button_rect.left + button_size;
	m_renderer1->Draw(surface,m_volume_button, NULL, &button_rect, alpha);


	return S_OK;
}

HRESULT dx_player::draw_nonmovie_bg(IDirect3DSurface9 *surface, int view)
{
	int client_width = m_width;
	int client_height = m_height;

	RECTF logo_rect = {client_width/2 - 960, client_height/2 - 540,
		client_width/2 + 960, client_height/2 + 540};
 	m_renderer1->Draw(surface, m_UI_logo, NULL, &logo_rect, 1);

	return S_OK;
}

#define rt(x) {if(out)*out=(x);return S_OK;}
HRESULT dx_player::hittest(int x, int y, int *out, double *out_value /* = NULL */)
{
	if (out_value)
		*out_value = 0;

	double UIScale = GET_CONST("UIScale");
	const double button_size = 40 * UIScale * UIScale;
	const double margin_button_right = 32 * UIScale;
	const double margin_button_bottom = 8 * UIScale;
	const double space_of_each_button = 62 * UIScale;
	const double toolbar_height = 65 * UIScale;
	const double width_progress_left = 5 * UIScale;
	const double width_progress_right = 6 * UIScale;
	const double margin_progress_right = 460 * UIScale;
	const double margin_progress_left = 37 * UIScale;
	const double progress_height = 21 * UIScale;
	const double progress_margin_bottom = 27 * UIScale;
	const double volume_base_width = 84 * UIScale;
	const double volume_base_height = 317 * UIScale;
	const double volume_margin_right = (156 - 84) * UIScale;
	const double volume_margin_bottom = (376 - 317) * UIScale;
	const double volume_button_zero_point = 32 * UIScale;
	const double volume_bar_height = 265 * UIScale;
	const double numbers_left_margin = 21 * UIScale;
	const double numbers_right_margin = 455 * UIScale;
	const double numbers_width = 12 * UIScale;
	const double numbers_height = 20 * UIScale;
	const double numbers_bottom_margin = 26;
	const double hidden_progress_width = 72 * UIScale;


	// hidden volume and brightness 
	if (((m_width - hidden_progress_width <= x && x < m_width)  ||
		(0 <= x && x < hidden_progress_width))
		&& y < m_height - toolbar_height
		)
	{
		if (out_value)
		{
			*out_value = (double)(m_height-y) / (m_height-toolbar_height);
			if(*out_value>1) *out_value = 1;
			if(*out_value<0) *out_value = 0;
		}
		rt(x <= hidden_progress_width ? hit_brightness : hit_volume2);
	}

	int button_x = m_width - button_size - margin_button_right;
	int button_outs[7] = {hit_full, hit_volume_button, hit_next, hit_play, hit_previous, hit_stop, hit_3d_swtich};
	double button_top = margin_button_bottom + button_size;

	for(int i=0; i<7; i++)
	{
		if (m_height - button_top < y && y< m_height - margin_button_bottom && button_x < x && x < button_x + button_size)
			rt(button_outs[i]);
		button_x -= space_of_each_button;
	}

	// progress bar
	int progressbar_right = m_width - margin_progress_right;
	int progressbar_left = margin_progress_left;


	if (m_height - button_top < y && y< m_height - margin_button_bottom && progressbar_left - 5 < x && x < progressbar_right + 5)
	{
		if (out_value)
		{
			*out_value = (double)(x-progressbar_left) / (progressbar_right - progressbar_left);
			if(*out_value>1) *out_value = 1;
			if(*out_value<0) *out_value = 0;
		}
		rt(hit_progress);
	}

	// volume bar
	RECTF volume_rect = {m_width - volume_base_width - volume_margin_right, 
		m_height - volume_base_height - volume_margin_bottom };
	volume_rect.right = volume_rect.left + volume_base_width;
	volume_rect.bottom = volume_rect.top + volume_base_height;

	if (volume_rect.left < x && x < volume_rect.right &&
		volume_rect.top < y && y < volume_rect.bottom)
	{
		if (out_value)
		{
			*out_value = 1 - (double)(y - volume_button_zero_point - volume_rect.top) / volume_bar_height;
			if(*out_value>1) *out_value = 1;
			if(*out_value<0) *out_value = 0;
		}
		if (m_show_volume_bar)
			rt(hit_volume);
	}

	// logo : center at (m_width/2, m_height/2), size = 500x213
	if (y > m_height - toolbar_height)
		rt(hit_bg);

	if (m_width/2 - 250 < x && x < m_width/2 + 250
		&& (m_height-toolbar_height)/2 - 106 < y && y < (m_height-toolbar_height)/2 + 106)
		rt(hit_logo);

	rt (hit_out);
}

#endif