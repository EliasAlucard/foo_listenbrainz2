#include "stdafx.h"
#include "http.h"

#include <regex>
using std::regex;

class my_initquit : public initquit
{
	void on_quit() override
	{
		simple_thread_pool::instance().exit();
	}
};

class my_play_callback_static : public play_callback_static
{
protected:
	my_play_callback_static() : m_target(SIZE_MAX), m_counter(0) {}
	~my_play_callback_static() {}

	void on_playback_new_track(metadb_handle_ptr p_track) override
	{
		double length = p_track->get_length();
		m_target = length <= 0 ? SIZE_MAX : min(static_cast<t_size>(length) >> 1, 240);
		m_counter = 0;
	}

	void on_playback_time(double p_time) override
	{
		m_counter++;
		if (m_counter == 2 && m_target > 10) // don't send "playing_now" notifications for really short tracks
		{
			submit(playing_now);
		}
		else if (m_counter == m_target)
		{
			submit(single);
		}
	}

	void on_playback_pause(bool p_state) override {}
	void on_playback_stop(play_control::t_stop_reason p_reason) override {}
	void on_playback_starting(play_control::t_track_command p_command, bool p_paused) override { }
	void on_playback_seek(double p_time) override {}
	void on_playback_edited(metadb_handle_ptr p_track) override {}
	void on_playback_dynamic_info(const file_info & p_info) override {}
	void on_playback_dynamic_info_track(const file_info & p_info) override {}
	void on_volume_change(float p_new_val) override {}

	unsigned int get_flags() override
	{
		return flag_on_playback_new_track | flag_on_playback_time;
	}

private:
	enum listen_type
	{
		playing_now,
		single
	};

	bool is_uuid(const char* mbid)
	{
		if (mbid == nullptr) return false;
		regex rx("^[0-9a-f]{8}-[0-9a-f]{4}-[0-9a-f]{4}-[0-9a-f]{4}-[0-9a-f]{12}$");
		return regex_search(mbid, rx);
	}

	void submit(listen_type type)
	{
		if (!prefs::enabled.get_value()) return;

		if (!is_uuid(prefs::user_token))
		{
			FB2K_console_formatter() << COMPONENT_TITLE << ": Not submitting. Token not set.";
			return;
		}

		metadb_handle_ptr handle;
		playback_control::get()->get_now_playing(handle);

		if (prefs::library.get_value() && !library_manager::get()->is_item_in_library(handle))
		{
			return;
		}

		const file_info_impl info = handle->get_info_ref()->info();

		const char* artist = info.meta_get("ARTIST", 0);
		const char* title = info.meta_get("TITLE", 0);
		if (artist == nullptr || title == nullptr) return;

		json j = json::object();
		j["payload"] = json::array();
		json track_metadata = json::object();

		track_metadata["artist_name"] = artist;
		track_metadata["track_name"] = title;

		if (type == playing_now)
		{
			j["listen_type"] = "playing_now";
		}
		else if (type == single)
		{
			j["listen_type"] = "single";
			j["payload"][0]["listened_at"] = pfc::fileTimeWtoU(pfc::fileTimeNow());

			const char* album = info.meta_get("ALBUM", 0);
			if (album != nullptr)
			{
				track_metadata["release_name"] = album;
			}

			json additional_info = json::object();
			additional_info["listening_from"] = "foo_listenbrainz2 " COMPONENT_VERSION;

			const char *release_mbid = info.meta_get("MUSICBRAINZ_ALBUMID", 0);
			if (!is_uuid(release_mbid)) release_mbid = info.meta_get("MUSICBRAINZ ALBUM ID", 0);
			if (is_uuid(release_mbid))
			{
				additional_info["release_mbid"] = release_mbid;
			}

			const char *release_group_mbid = info.meta_get("MUSICBRAINZ_RELEASEGROUPID", 0);
			if (!is_uuid(release_group_mbid)) release_group_mbid = info.meta_get("MUSICBRAINZ RELEASE GROUP ID", 0);
			if (is_uuid(release_group_mbid))
			{
				additional_info["release_group_mbid"] = release_group_mbid;
			}

			const char *recording_mbid = info.meta_get("MUSICBRAINZ_TRACKID", 0);
			if (!is_uuid(recording_mbid)) recording_mbid = info.meta_get("MUSICBRAINZ TRACK ID", 0);
			if (is_uuid(recording_mbid))
			{
				additional_info["recording_mbid"] = recording_mbid;
			}

			const char *track_mbid = info.meta_get("MUSICBRAINZ_RELEASETRACKID", 0);
			if (!is_uuid(track_mbid)) track_mbid = info.meta_get("MUSICBRAINZ RELEASE TRACK ID", 0);
			if (is_uuid(track_mbid))
			{
				additional_info["track_mbid"] = track_mbid;
			}

			const char *work_mbid = info.meta_get("MUSICBRAINZ_WORKID", 0);
			if (!is_uuid(work_mbid)) work_mbid = info.meta_get("MUSICBRAINZ WORK ID", 0);
			if (is_uuid(work_mbid))
			{
				additional_info["work_mbids"] = json::array();
				additional_info["work_mbids"][0] = work_mbid;
			}

			const char* artist_mbid = info.meta_get("MUSICBRAINZ_ARTISTID", 0);
			if (!is_uuid(artist_mbid)) artist_mbid = info.meta_get("MUSICBRAINZ ARTIST ID", 0);
			if (is_uuid(artist_mbid))
			{
				additional_info["artist_mbids"] = json::array();
				additional_info["artist_mbids"][0] = artist_mbid;
			}

			const char *track_number = info.meta_get("TRACKNUMBER", 0);
			if (track_number != nullptr)
			{
				additional_info["tracknumber"] = track_number;
			}

			const char *isrc = info.meta_get("ISRC", 0);
			if (isrc != nullptr)
			{
				additional_info["isrc"] = isrc;
			}

			track_metadata["additional_info"] = additional_info;
		}

		j["payload"][0]["track_metadata"] = track_metadata;

		auto task = new lb_http_client(j);
		simple_thread_pool::instance().enqueue(task);
	}

	t_size m_counter, m_target;
};

static initquit_factory_t<my_initquit> g_my_initquit;
static play_callback_static_factory_t<my_play_callback_static> g_my_play_callback_static;
