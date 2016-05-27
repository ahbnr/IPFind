/*
 *   Copyright (C) 2016 Anton Haubner <anton.haubner@outlook.de>
 *
 *   This file is part of the IPFind library.
 *
 *   This software is provided 'as-is', without any express or implied
 *   warranty.  In no event will the authors be held liable for any damages
 *   arising from the use of this software.
 *
 *   Permission is granted to anyone to use this software for any purpose,
 *   including commercial applications, and to alter it and redistribute it
 *   freely, subject to the following restrictions:
 *
 *   1. The origin of this software must not be misrepresented; you must not
 *   claim that you wrote the original software. If you use this software
 *   in a product, an acknowledgment in the product documentation would be
 *   appreciated but is not required.
 *
 *   2. Altered source versions must be plainly marked as such, and must not be
 *   misrepresented as being the original software.
 *
 *   3. This notice may not be removed or altered from any source distribution.
 *
 *   4. Source versions may not be "relicensed" under a different license
 *   without my explicitly written permission.
 *
*/

#include <IPFind/Server.hpp>

#include <stdexcept>
#include <thread>
#include <memory>
#include <iostream>

using namespace boost::asio::ip;
using namespace std;

namespace IPFind
{
	Server::Server(Utils::Port port)
		:	m_IOService(),
			m_Socket(m_IOService, udp::endpoint(udp::v4(), port)),
			m_Running(false)
	{

	}

	Server::~Server()
	{
		if (m_Running)
		{
			stop();
		}

		m_Running = false;
		m_Socket.close();
		m_IOService.stop();
	}

	void Server::listen()
	{
		if (m_Running)
		{
			throw runtime_error("Listen called twice!");
		}
		std::unique_lock<mutex> Lock(m_RunningMutex);
		m_Running = true;

		unique_ptr<atomic_bool, void(*)(atomic_bool *)> Guard(&m_Running, [](atomic_bool * Ptr)
		{
			*Ptr = false;
		});

		while (m_Running)
		{
			udp::endpoint SenderEnd;
			Utils::BroadcastPackage Buffer;
			m_Socket.async_receive_from(boost::asio::buffer(Buffer), SenderEnd,
										[this, &Buffer, &SenderEnd](boost::system::error_code const & error, std::size_t received)
			{
				if (!error && m_Running && received == Buffer.size())
				{
					//Etwas empfangen -> Antworten
					m_Socket.send_to(boost::asio::buffer(Buffer), SenderEnd);
				}

				else if (error)
				{
					cerr << "ERROR: " << error.message() << endl;
				}
			});

			m_IOService.run();
			m_IOService.reset();
		}
	}

	void Server::listen_async()
	{
		std::thread Thread([this]()
		{
			listen();
		});

		Thread.detach();
	}

	void Server::stop()
	{
		m_Running = false;
		m_Socket.cancel();
		m_IOService.stop();

		std::unique_lock<mutex> Lock(m_RunningMutex);
		m_IOService.reset();
	}
}

