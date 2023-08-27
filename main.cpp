#include "TableStorageEntry.h"
#include "TableStorageRequestHandler.h"
#include <rtc/rtc.hpp>
#include "CameraDataChannelHandler.h"
#include "Signal.hpp"
#include "MovementHandler.h"

#include <chrono>
#include <iostream>
#include <memory>
#include <thread>
#include <cstring>
#include <rtc/rtc.hpp>
#include "PCA9685.h"
#include "ServoHandler.h"

using std::shared_ptr;
using std::weak_ptr;
template <class T> weak_ptr<T> make_weak_ptr(shared_ptr<T> ptr) { return ptr; }

int ConfigurePeer(TableStorageRequestHandler& tableStorageRequestHandler,
	CameraDataChannelHandler& cameraHandler, MovementHandler& movementHandler, ServoHandler& servoHandler,
	TableStorageEntry& answerTableEntry, std::shared_ptr<rtc::Track>& track,
	std::shared_ptr<rtc::PeerConnection>& pc, std::shared_ptr<rtc::DataChannel>& dc) {

	answerTableEntry = TableStorageEntry("answerer");
	answerTableEntry.status = "standby";
	HttpObject result = tableStorageRequestHandler.SendRequest(tableStorageRequestHandler.PUT, answerTableEntry);
	//Setup rtc
	//rtc::InitLogger(rtc::LogLevel::Warning);
	//rtc::Configuration config;
	//config.iceServers.emplace_back("stun.l.google.com:19302");
	//auto pc = std::make_shared<rtc::PeerConnection>(config);
	//shared_ptr<rtc::DataChannel> dc;


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
				cameraHandler.Disconnect();
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

		std::cout << "[Got a DataChannel with label: " << _dc->label() << "]" << std::endl;


		dc = _dc;

		dc->onClosed([&]() {
			std::cout << "[DataChannel closed: " << dc->label() << "]" << std::endl;
			if (cameraHandler.IsRunning()) {
				cameraHandler.StopCamera();
			}
			servoHandler.~ServoHandler();
		});


		dc->onMessage([&](auto data) {
							
			std::uint16_t test = -1;
			if (std::holds_alternative<std::vector<std::byte>>(data)) {
				std::memcpy(&test, ((std::vector<std::byte>)std::get<std::vector<std::byte>>(data)).data(), sizeof(std::uint16_t));
			}
			switch (test) {
			case Signal::Movement:
				//MovementSignal ms;
				//movementHandler.ms
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
	//Setup Azure Comms
	int pi = pigpio_start(NULL, NULL);
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


	bool exit = false;
	while (!exit) {
		bool getPeer = true;
		std::cout
			<< std::endl
			<< "**********************************************************************************"
			<< std::endl
			<< "[Command]: ";

		int command = 1;

		//std::cin >> command;
		//std::cin.ignore();

		switch (command) {
			case 0: {
				exit = true;
				break;
			}
			case 1: {
				pc = std::make_shared<rtc::PeerConnection>(config);
				ConfigurePeer(tableStorageRequestHandler, cameraHandler, movementHandler, servoHandler, answerTableEntry, track, pc, dc);
				while (getPeer) {
					//Check if there is an offer available
					HttpObject result = tableStorageRequestHandler.SendRequest(tableStorageRequestHandler.GET, TableStorageEntry("caller"));

					if (result.entry.status.compare("calling") ==  0) {
						getPeer = false;
						std::cout << "Caller Description found" << std::endl;
						std::string temp = result.entry.description;
						pc->setRemoteDescription(temp);
						std::cout <<  "Caller Candidate found" << std::endl;

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
						HttpObject result = tableStorageRequestHandler.SendRequest(tableStorageRequestHandler.PUT, answerTableEntry);
						break;
					}
					else {
						std::cout << "No caller, waiting" << std::endl;
						std::this_thread::sleep_for(std::chrono::milliseconds(1000));
					}
				}

				std::this_thread::sleep_for(std::chrono::milliseconds(30 * 1000));
				while (dc->isOpen()) {
					std::this_thread::sleep_for(std::chrono::milliseconds(5000));
				}

				break;
			}
			case 2: {
				answerTableEntry.status = "answering";
				HttpObject result = tableStorageRequestHandler.SendRequest(tableStorageRequestHandler.PUT, answerTableEntry);
			}
			case 3: {
				// Send Message
				if (!dc || !dc->isOpen()) {
					std::cout << "** Channel is not Open ** " << std::endl;
					break;
				}
				std::cout << "[Message]: ";
				std::string message;
				getline(std::cin, message);
				dc->send(message);
				break;
			}
			case 4: {
				// Connection Info
				if (!dc || !dc->isOpen()) {
					std::cout << "** Channel is not Open ** " << std::endl;
					break;
				}
				rtc::Candidate local, remote;
				std::optional<std::chrono::milliseconds> rtt = pc->rtt();
				if (pc->getSelectedCandidatePair(&local, &remote)) {
					std::cout << "Local: " << local << std::endl;
					std::cout << "Remote: " << remote << std::endl;
					std::cout << "Bytes Sent:" << pc->bytesSent()
						<< " / Bytes Received:" << pc->bytesReceived() << " / Round-Trip Time:";
					if (rtt.has_value())
						std::cout << rtt.value().count();
					else
						std::cout << "null";
					std::cout << " ms";
				}
				else {
					std::cout << "Could not get Candidate Pair Info" << std::endl;
				}
				break;
			}
			default: {
				std::cout << "** Invalid Command ** " << std::endl;
				break;
			}
		}
	}

	if (dc)
		dc->close();

	if (pc)
		pc->close();
}


