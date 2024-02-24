#include "TableStorageEntry.h"
#include "TableStorageRequestHandler.h"
#include <rtc/rtc.hpp>
#include "CameraDataChannelHandler.h"
#include "Signal.hpp"
#include "MovementHandler.h"
#include <jetgpio.h>

#include <chrono>
#include <iostream>
#include <memory>
#include <thread>
#include <cstring>
#include <rtc/rtc.hpp>
#include "PCA9685.h"
#include "ServoHandler.h"
#include "LidarHandler.h"

using std::shared_ptr;
using std::weak_ptr;
template <class T> weak_ptr<T> make_weak_ptr(shared_ptr<T> ptr) { return ptr; }

int ConfigurePeer(TableStorageRequestHandler& tableStorageRequestHandler, CameraDataChannelHandler& cameraHandler, 
	MovementHandler& movementHandler, ServoHandler& servoHandler, LidarHandler& lidarHandler,
	TableStorageEntry& answerTableEntry, std::shared_ptr<rtc::Track>& track,
	std::shared_ptr<rtc::PeerConnection>& pc, std::shared_ptr<rtc::DataChannel>& dc) {

	answerTableEntry = TableStorageEntry("answerer");
	answerTableEntry.status = "standby";
	HttpObject result = tableStorageRequestHandler.SendRequest(tableStorageRequestHandler.PUT, answerTableEntry);
	result = tableStorageRequestHandler.SendRequest(tableStorageRequestHandler.PUT, TableStorageEntry("caller", "", "", ""));

	pc->setLocalDescription();

	pc->onTrack([&](std::shared_ptr<rtc::Track> t) {
		std::this_thread::sleep_for(std::chrono::milliseconds(500));
	track = t;
	std::cout << "track opened" << std::endl;

	rtc::Description::Media trackDesc = track->description();
	auto trackDescAtt = trackDesc.attributes();
	pc->addTrack(trackDesc);



	track->onOpen([&] {
		std::this_thread::sleep_for(std::chrono::milliseconds(500));
	std::cout << "track opened" << std::endl; ;
	cameraHandler.StartCamera(track);
		});
	track->onClosed([&] {
				std::cout << "track closed" << std::endl; ;
				//cameraHandler.Disconnect();
			});
		});

	pc->onLocalDescription([&](rtc::Description description) {
		answerTableEntry.description = std::string(description);
		});

	pc->onLocalCandidate([&](rtc::Candidate candidate) {
		std::cout << "New Candidate: " << std::string(candidate) << "]" << std::endl;
	if (answerTableEntry.candidate.length() == 0) {
		answerTableEntry.candidate = std::string(candidate);
	}
	else {
		answerTableEntry.candidate += '\n' + std::string(candidate);
	}
		});

	pc->onStateChange([](rtc::PeerConnection::State state) {
		std::cout << "[State: " << state << "]" << std::endl;
	});
	pc->onGatheringStateChange([](rtc::PeerConnection::GatheringState state) {
		std::cout << "[Gathering State: " << state << "]" << std::endl;
	});

	pc->onDataChannel([&](shared_ptr<rtc::DataChannel> _dc) {
		movementHandler.start();
		servoHandler.start();
		answerTableEntry = TableStorageEntry("answerer");
		answerTableEntry.status = "standby";
		HttpObject result = tableStorageRequestHandler.SendRequest(tableStorageRequestHandler.PUT, answerTableEntry);
		
		dc = _dc;
		lidarHandler.start(dc);

		std::cout << "[Got a DataChannel with label: " << _dc->label() << "]" << std::endl;



		dc->onClosed([&]() {
			std::cout << "[DataChannel closed: " << dc->label() << "]" << std::endl;
			if (cameraHandler.IsRunning()) {
				//cameraHandler.StopCamera();
				//std::this_thread::sleep_for(std::chrono::milliseconds(1000));
				//cameraHandler.Disconnect();
				//std::this_thread::sleep_for(std::chrono::milliseconds(1000));
				//cameraHandler.~CameraDataChannelHandler();
			}
			servoHandler.~ServoHandler();
			lidarHandler.~LidarHandler();
		});


		dc->onMessage([&](auto data) {
							
			std::uint16_t test = -1;
			if (std::holds_alternative<std::vector<std::byte>>(data)) {
				std::memcpy(&test, ((std::vector<std::byte>)std::get<std::vector<std::byte>>(data)).data(), sizeof(std::uint16_t));
			}
			switch (test) {
			case Signal::Movement:
				std::memcpy(&movementHandler.ms, ((std::vector<std::byte>)std::get<std::vector<std::byte>>(data)).data(), sizeof(MovementSignal));
				movementHandler.read = true;
				break;
			case Signal::CameraLook:
				std::memcpy(&servoHandler.ss, ((std::vector<std::byte>)std::get<std::vector<std::byte>>(data)).data(), sizeof(ServoSignal));
				servoHandler.read = true;
				break;
			case -1:
				std::cout << "Signal Error" << std::endl;
			}
		});

	});
}


int main(int argc, char** argv) {
	int pi = gpioInitialise();
	PCA9685 pca9685 = PCA9685(pi,1, 0x40,0);
	pca9685.SetDutyCyclePercent(0, 0);
	std::this_thread::sleep_for(std::chrono::milliseconds(1 * 1000/2));
	pca9685.SetDutyCyclePercent(0, .5);
	std::this_thread::sleep_for(std::chrono::milliseconds(1 * 1000/2));
	pca9685.SetDutyCyclePercent(0, 1);
	std::this_thread::sleep_for(std::chrono::milliseconds(1 * 1000/2));
	pca9685.SetDutyCyclePercent(0, .5);
	std::this_thread::sleep_for(std::chrono::milliseconds(1 * 1000/4));

	TableStorageEntry answerTableEntry = TableStorageEntry("answerer");
	answerTableEntry.status = "standby";

	rtc::InitLogger(rtc::LogLevel::Warning);
	rtc::Configuration config;
	config.iceServers.emplace_back("stun.l.google.com:19302");
	std::shared_ptr<rtc::DataChannel> dc;
	std::shared_ptr<rtc::PeerConnection> pc;
	std::shared_ptr<rtc::Track> track;
	
	TableStorageRequestHandler tableStorageRequestHandler;
	ServoHandler servoHandler = ServoHandler(pi, 0, pca9685);
	MovementHandler movementHandler = MovementHandler(pi);
	CameraDataChannelHandler cameraHandler;
	LidarHandler lidarHandler = LidarHandler();

	bool exit = false;
	int i = 0;
	while (!exit) {
		bool getPeer = true;
		

		pc = std::make_shared<rtc::PeerConnection>(config);
		ConfigurePeer(tableStorageRequestHandler, cameraHandler, movementHandler, servoHandler, lidarHandler, answerTableEntry, track, pc, dc);

		std::cout << std::endl
				  << "**********************************************************************************"
				  << std::endl
				  << "System Config Complete"
				  << std::endl;

		while (getPeer) {
			//Check if there is an offer available
			HttpObject result = tableStorageRequestHandler.SendRequest(tableStorageRequestHandler.GET, TableStorageEntry("caller"));

			if (result.entry.status.compare("calling") == 0) {
				HttpObject result2 = tableStorageRequestHandler.SendRequest(tableStorageRequestHandler.PUT, TableStorageEntry("caller", "", "", ""));
				
				getPeer = false;
				std::cout << "Caller Description found" << std::endl;
				std::string temp = result.entry.description;
				pc->setRemoteDescription(temp);
				std::cout << "Caller Candidate found" << std::endl;

				temp = result.entry.candidate;
				int start = 0;
				for (int end = start + 1; end < temp.size(); end++) {
					if (temp[end] == '\n' || temp[end] == '\0') {
						pc->addRemoteCandidate(temp.substr(start, end - start));
						start = end + 1;
					}
				}

				std::this_thread::sleep_for(std::chrono::milliseconds(3000));
				answerTableEntry.status = "answering";
				result = tableStorageRequestHandler.SendRequest(tableStorageRequestHandler.PUT, answerTableEntry);
				
				std::this_thread::sleep_for(std::chrono::milliseconds(30 * 1000));
				while (dc->isOpen()) {
					std::this_thread::sleep_for(std::chrono::milliseconds(5000));
				}
				exit = true;
				//new (&servoHandler) ServoHandler(pi, 0, pca9685);
				//new (&lidarHandler) LidarHandler();
				//new (&cameraHandler) CameraDataChannelHandler();
			}
			else {
				std::cout << "No caller, waiting - " << i++ << std::endl;
				std::this_thread::sleep_for(std::chrono::milliseconds(1000));
			}

		}
	}

	if (dc)
		dc->close();

	if (pc)
		pc->close();
}


