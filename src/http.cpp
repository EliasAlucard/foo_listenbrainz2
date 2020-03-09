#include "stdafx.h"
#include "http.h"

#include <WinInet.h>

void lb_http_client::run()
{
	bool report = prefs::console.get_value() && m_data["listen_type"].get<std::string>() == "single"; // only report server reponse if preference enabled and "single", not "playing_now"
	pfc::string8_fast auth_header = PFC_string_formatter() << "Authorization: token " << prefs::user_token;
	std::string str = m_data.dump();

	HINTERNET internet = InternetOpenA(g_user_agent, INTERNET_OPEN_TYPE_PRECONFIG, nullptr, nullptr, 0);
	HINTERNET connect = InternetConnectA(internet, "api.listenbrainz.org", INTERNET_DEFAULT_HTTPS_PORT, nullptr, nullptr, INTERNET_SERVICE_HTTP, 0, 0);
	HINTERNET request = HttpOpenRequestA(connect, "POST", "/1/submit-listens", "HTTP/1.1", nullptr, nullptr,
		SECURITY_INTERNET_MASK |
		INTERNET_FLAG_HYPERLINK |
		INTERNET_FLAG_NO_AUTH |
		INTERNET_FLAG_NO_CACHE_WRITE |
		INTERNET_FLAG_NO_UI |
		INTERNET_FLAG_PRAGMA_NOCACHE |
		INTERNET_FLAG_RELOAD |
		INTERNET_FLAG_SECURE, 0);

	if (HttpSendRequestA(request, auth_header.get_ptr(), auth_header.get_length(), (LPVOID)str.c_str(), str.length()))
	{
		if (report)
		{
			pfc::string8_fast result;
			char szBuffer[1025];
			DWORD bytesToRead = sizeof(szBuffer) - 1;
			DWORD bytesRead = 0;
			while (InternetReadFile(request, szBuffer, bytesToRead, &bytesRead))
			{
				if (bytesRead == 0) break;
				szBuffer[bytesRead] = 0;
				bytesRead = 0;
				result << szBuffer;
			}
			FB2K_console_formatter() << COMPONENT_TITLE << ": " << result;
		}
	}

	InternetCloseHandle(request);
	InternetCloseHandle(connect);
	InternetCloseHandle(internet);
}
