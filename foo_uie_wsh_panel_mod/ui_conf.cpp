#include "stdafx.h"
#include "ui_conf.h"
#include "host.h"
#include "ui_goto.h"
#include "ui_find.h"
#include "ui_replace.h"
#include "helpers.h"


using namespace pfc::stringcvt;

//config_dialog_manager g_config_dlg_mgr;

LRESULT CDialogConf::OnInitDialog(HWND hwndFocus, LPARAM lParam)
{
	//g_config_dlg_mgr.add_window(m_hWnd);

	// Get caption text
	uGetWindowText(m_hWnd, m_caption);

	// Init resize
	DlgResize_Init();

	// Apply window placement
	if (m_parent->get_wndpl().length == 0)
	{
		m_parent->get_wndpl().length = sizeof(WINDOWPLACEMENT);

		if (!GetWindowPlacement(&m_parent->get_wndpl()))
			memset(&m_parent->get_wndpl(), 0, sizeof(WINDOWPLACEMENT));
	}
	else
	{
		SetWindowPlacement(&m_parent->get_wndpl());
	}

	// Script Engine
	HWND combo_script_engine = GetDlgItem(IDC_SCRIPT_ENGINE);

	ComboBox_AddString(combo_script_engine, _T("JScript"));
	ComboBox_AddString(combo_script_engine, _T("VBScript"));

	if (!uComboBox_SelectString(combo_script_engine, m_parent->get_script_name()))
		ComboBox_SetCurSel(combo_script_engine, 0);

	// Edge Style
	HWND combo_edge_style = GetDlgItem(IDC_EDGE_STYLE);

	ComboBox_AddString(combo_edge_style, _T("None"));    // NO_EDGE
	ComboBox_AddString(combo_edge_style, _T("Sunken"));  // SUNKEN_EDGE
	ComboBox_AddString(combo_edge_style, _T("Grey"));    // GREY_EDGE
	ComboBox_SetCurSel(combo_edge_style, m_parent->get_edge_style());

	// Edit box
	pfc::string8_fast text;
	int cursel = ComboBox_GetCurSel(combo_script_engine);

	// Subclassing scintilla
	m_editorctrl.SubclassWindow(GetDlgItem(IDC_EDIT));

	if (uComboBox_GetText(combo_script_engine, cursel, text))
	{
		m_editorctrl.SetLanguage(text);
	}

	// Checkboxs
	uButton_SetCheck(m_hWnd, IDC_CHECK_GRABFOCUS, m_parent->get_grab_focus());
	uButton_SetCheck(m_hWnd, IDC_CHECK_PSEUDO_TRANSPARENT, m_parent->get_pseudo_transparent());

	::EnableWindow(combo_edge_style, !m_parent->get_pseudo_transparent());

	// GUID Text
	pfc::string8 guid_text = "GUID: ";
	guid_text += pfc::print_guid(m_parent->get_config_guid());
	uSetWindowText(GetDlgItem(IDC_STATIC_GUID), guid_text);

	// Script
	m_editorctrl.SetContent(m_parent->get_script_code(), true);

	// Set save point
	m_editorctrl.SetSavePoint();

	return TRUE; // set focus to default control
}

LRESULT CDialogConf::OnCloseCmd(WORD wNotifyCode, WORD wID, HWND hWndCtl)
{
	switch (wID)
	{
	case IDOK:
		Apply();
		EndDialog(IDOK);
		break;

	case IDAPPLY:
		Apply();
		break;

	case IDCANCEL:
		if (m_editorctrl.GetModify())
		{
			// Prompt?
			int ret = uMessageBox(m_hWnd, "Save and apply changes?", m_caption, MB_ICONWARNING | MB_YESNOCANCEL);

			switch (ret)
			{
			case IDYES:
				Apply();
				EndDialog(IDOK);
				break;

			case IDCANCEL:
				return 0;
			}
		}

		EndDialog(IDCANCEL);
	}
	
	return 0;
}

LRESULT CDialogConf::OnScriptEngineCbnSelEndOk(WORD wNotifyCode, WORD wID, HWND hWndCtl)
{
	pfc::string8_fast text;
	HWND edit = GetDlgItem(IDC_EDIT);
	HWND combo = GetDlgItem(IDC_SCRIPT_ENGINE);
	int cursel = ComboBox_GetCurSel(combo);

	if (uComboBox_GetText(combo, cursel, text))
	{
		m_editorctrl.SetLanguage(text);
	}

	return 0;
}

LRESULT CDialogConf::OnResetDefault(WORD wNotifyCode, WORD wID, HWND hWndCtl)
{
	pfc::string8 code;
	HWND combo = GetDlgItem(IDC_SCRIPT_ENGINE);

	wsh_panel_vars::get_default_script_code(code);
	uComboBox_SelectString(combo, "JScript");
	m_editorctrl.SetContent(code);
	return 0;
}

LRESULT CDialogConf::OnResetCurrent(WORD wNotifyCode, WORD wID, HWND hWndCtl)
{
	HWND combo = GetDlgItem(IDC_SCRIPT_ENGINE);

	uComboBox_SelectString(combo, m_parent->get_script_name());			
	m_editorctrl.SetContent(m_parent->get_script_code());
	return 0;
}

LRESULT CDialogConf::OnImport(WORD wNotifyCode, WORD wID, HWND hWndCtl)
{
	pfc::string8 filename;

	if (uGetOpenFileName(m_hWnd, "Text files|*.txt;*.text|JScript files|*.js|All files|*.*", 0, "txt", "Import from", NULL, filename, FALSE))
	{
		// Open file
		pfc::string8_fast text;

		helpers::read_file(filename, text);
		m_editorctrl.SetContent(text);
	}

	return 0;
}

LRESULT CDialogConf::OnExport(WORD wNotifyCode, WORD wID, HWND hWndCtl)
{
	pfc::string8 filename;

	if (uGetOpenFileName(m_hWnd, "Text files|*.txt|All files|*.*", 0, "txt", "Save as", NULL, filename, TRUE))
	{
		int len = m_editorctrl.GetTextLength();
		pfc::string8_fast text;

		m_editorctrl.GetText(text.lock_buffer(len), len + 1);
		text.unlock_buffer();

		helpers::write_file(filename, text);
	}

	return 0;
}

void CDialogConf::Apply()
{
	pfc::string8 name;
	pfc::array_t<char> code;
	int len = 0;

	// Get engine name
	uGetWindowText(GetDlgItem(IDC_SCRIPT_ENGINE), name);
	// Get script text
	len = m_editorctrl.GetTextLength();
	code.set_size(len + 1);
	m_editorctrl.GetText(code.get_ptr(), len + 1);

	m_parent->get_edge_style() = static_cast<t_edge_style>(ComboBox_GetCurSel(GetDlgItem(IDC_EDGE_STYLE)));
	m_parent->get_disabled() = false;
	m_parent->get_grab_focus() = uButton_GetCheck(m_hWnd, IDC_CHECK_GRABFOCUS);
	m_parent->get_pseudo_transparent() = uButton_GetCheck(m_hWnd, IDC_CHECK_PSEUDO_TRANSPARENT);
	m_parent->on_update_script(name, code.get_ptr());

	// Wndow position
	GetWindowPlacement(&m_parent->get_wndpl());

	// Save point
	m_editorctrl.SetSavePoint();
}

LRESULT CDialogConf::OnNotify(int idCtrl, LPNMHDR pnmh)
{
	SCNotification * notification = (SCNotification *)pnmh;

	switch (pnmh->code)
	{
		// dirty
	case SCN_SAVEPOINTLEFT:
		{
			pfc::string8 caption = m_caption;

			caption += " *";
			uSetWindowText(m_hWnd, caption);
		}
		break;

		// not dirty
	case SCN_SAVEPOINTREACHED:
		uSetWindowText(m_hWnd, m_caption);
		break;
	}

	SetMsgHandled(FALSE);
	return 0; 
}

bool CDialogConf::MatchShortcuts(unsigned vk)
{
	int modifiers = 
		(IsKeyPressed(VK_SHIFT) ? SCMOD_SHIFT : 0) |
		(IsKeyPressed(VK_CONTROL) ? SCMOD_CTRL : 0) |
		(IsKeyPressed(VK_MENU) ? SCMOD_ALT : 0);

	// Hotkey
	if (modifiers == SCMOD_CTRL)
	{
		switch (vk)
		{
		case 'F':
			{
				if (!m_dlgfind)
				{
					// Create it on request.
					m_dlgfind = new CDialogFind(GetDlgItem(IDC_EDIT));
					
					if (!m_dlgfind || !m_dlgfind->Create(m_hWnd))
						break;
				}

				m_dlgfind->ShowWindow(SW_SHOW);
				m_dlgfind->SetFocus();
			}
			return true;

		case 'H':
			{
				if (!m_dlgreplace)
				{
					m_dlgreplace = new CDialogReplace(GetDlgItem(IDC_EDIT));
					
					if (!m_dlgreplace || !m_dlgreplace->Create(m_hWnd))
						break;
				}

				m_dlgreplace->ShowWindow(SW_SHOW);
				m_dlgreplace->SetFocus();
			}
			return true;

		case 'G':
			{
				CDialogGoto dlg(GetDlgItem(IDC_EDIT));
				dlg.DoModal(m_hWnd);
			}
			return true;

		case 'S':
			Apply();
			return true;
		}
	}

	return false;
}

LRESULT CDialogConf::OnCheckPseudoTransparentBnClicked(WORD wNotifyCode, WORD wID, HWND hWndCtl)
{
	::EnableWindow(GetDlgItem(IDC_EDGE_STYLE), !uButton_GetCheck(m_hWnd, IDC_CHECK_PSEUDO_TRANSPARENT));
	return 0;
}

LRESULT CDialogConf::OnScnKeyDown(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
	return MatchShortcuts(wParam);
}