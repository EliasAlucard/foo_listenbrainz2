#pragma once

class lb_http_client : public simple_thread_task
{
public:
	lb_http_client(json p_data) : m_data(p_data) {}

	void run() override;

private:
	json m_data;
};
