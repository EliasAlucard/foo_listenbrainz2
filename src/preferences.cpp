#include "stdafx.h"

namespace prefs
{
	static constexpr bool default_console = true;
	static constexpr bool default_enabled = false;
	static constexpr bool default_library = false;
	cfg_bool console(g_guid_console, default_console);
	cfg_bool enabled(g_guid_enabled, default_enabled);
	cfg_bool library(g_guid_library, default_library);
	cfg_string user_token(g_guid_user_token, "");
}

class my_preferences_page_instance : public CDialogImpl<my_preferences_page_instance>, public preferences_page_instance
{
public:
	my_preferences_page_instance(preferences_page_callback::ptr p_callback) : m_callback(p_callback) {}

	BEGIN_MSG_MAP(CPreferencesDialog)
		MSG_WM_INITDIALOG(OnInitDialog)
		COMMAND_RANGE_HANDLER_EX(IDC_ENABLED, IDC_LIBRARY, OnChanged)
	END_MSG_MAP()

	enum { IDD = IDD_PREFERENCES };

	BOOL OnInitDialog(CWindow wndFocus, LPARAM lInitParam)
	{
		console_checkbox = GetDlgItem(IDC_CONSOLE);
		console_checkbox.SetCheck(prefs::console.get_value());
		console_checkbox.EnableWindow(prefs::enabled.get_value());

		enabled_checkbox = GetDlgItem(IDC_ENABLED);
		enabled_checkbox.SetCheck(prefs::enabled.get_value());

		library_checkbox = GetDlgItem(IDC_LIBRARY);
		library_checkbox.SetCheck(prefs::library.get_value());
		library_checkbox.EnableWindow(prefs::enabled.get_value());

		user_token_edit = GetDlgItem(IDC_USER_TOKEN);
		user_token_edit.EnableWindow(prefs::enabled.get_value());
		uSetWindowText(user_token_edit, prefs::user_token);

		return 0;
	}

	bool has_changed()
	{
		if (console_checkbox.IsChecked() != prefs::console.get_value()) return true;
		if (enabled_checkbox.IsChecked() != prefs::enabled.get_value()) return true;
		if (library_checkbox.IsChecked() != prefs::library.get_value()) return true;

		pfc::string8 temp;
		uGetWindowText(user_token_edit, temp);
		if (prefs::user_token != temp) return true;

		return false;
	}

	t_uint32 get_state()
	{
		t_uint32 state = preferences_state::resettable;
		if (has_changed()) state |= preferences_state::changed;
		return state;
	}

	void apply()
	{
		prefs::console = console_checkbox.IsChecked();
		prefs::enabled = enabled_checkbox.IsChecked();
		prefs::library = library_checkbox.IsChecked();
		uGetWindowText(user_token_edit, prefs::user_token);
	}

	void on_change()
	{
		console_checkbox.EnableWindow(enabled_checkbox.IsChecked());
		library_checkbox.EnableWindow(enabled_checkbox.IsChecked());
		user_token_edit.EnableWindow(enabled_checkbox.IsChecked());

		m_callback->on_state_changed();
	}

	void reset()
	{
		console_checkbox.SetCheck(prefs::default_console);
		enabled_checkbox.SetCheck(prefs::default_enabled);
		library_checkbox.SetCheck(prefs::default_library);
		uSetWindowText(user_token_edit, "");

		on_change();
	}

	void OnChanged(UINT, int, HWND)
	{
		on_change();
	}


private:
	CCheckBox console_checkbox;
	CCheckBox enabled_checkbox;
	CCheckBox library_checkbox;
	CEdit user_token_edit;
	preferences_page_callback::ptr m_callback;
};

class my_preferences_page_impl : public preferences_page_impl<my_preferences_page_instance> {
public:
	const char* get_name()
	{
		return COMPONENT_TITLE;
	}

	GUID get_guid()
	{
		return g_guid_preferences;
	}

	GUID get_parent_guid()
	{
		return preferences_page::guid_tools;
	}
};

preferences_page_factory_t<my_preferences_page_impl> g_my_preferences_page_impl;
