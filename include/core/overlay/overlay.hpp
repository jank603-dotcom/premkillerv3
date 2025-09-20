#ifndef OVERLAY_HPP
#define OVERLAY_HPP

class overlay_c
{
public:
	bool initialize( );

	void loop( );

private:
	bool setup_d3d11( );
	bool setup_window( );

private:
	HWND hwnd{ nullptr };
	ID3D11Device* device{ nullptr };
	ID3D11DeviceContext* device_context{ nullptr };
	ID3D11RenderTargetView* render_target_view{ nullptr };
	IDXGISwapChain* swap_chain{ nullptr };

private:
	bool find_ime( );
	bool align_to_game( HWND hwnd_game ) const;
	void set_attributes( ) const;
};

#endif // !OVERLAY_HPP