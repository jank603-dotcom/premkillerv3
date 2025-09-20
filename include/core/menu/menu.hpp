#ifndef MENU_HPP
#define MENU_HPP

class menu_c
{
public:
	void run( );
	bool is_running = true;
	bool is_visible = true;
	void apply_stream_proof( HWND hwnd );
	void remove_stream_proof( HWND hwnd );
	void show_notification(const std::string& message);
	enum class notify_type { info, success, warning, error, damage };
	void show_notification(const std::string& message, notify_type type, float duration_sec = 3.0f);
	const char* get_key_name(int key);
	void handle_hotkeys();

private:
	struct notification_t {
		std::string text;
		std::chrono::steady_clock::time_point time;
		float alpha = 1.0f; 
		notify_type type = notify_type::info;
		float duration = 3.0f; 
	};

	void style( ) const;
	void render( );
	void render_notifications();

	void render_aim( );
	void render_visuals( );
	void render_exploits( );
	void render_misc( );
	void render_themes( );
	void apply_theme( int theme_id );

private:
	bool is_styled = false;
	int current_tab = 0;
	bool theme_changed = false;
	bool stream_proof_active = false;
	std::deque<notification_t> notifications;
	static constexpr int max_notifications = 6;
	static constexpr float notification_duration = 3.0f; 
	static constexpr float notification_spacing = 36.0f;
	static constexpr int notification_corner = 0;
};

#endif // !MENU_HPP