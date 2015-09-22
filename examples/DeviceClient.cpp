#include <iostream>
#include "plugin/ExampleDevice.h"
#include "MtcaMappedDevice/BackendFactory.h"
using namespace mtca4u;
int main() {
	//ExampleDeviceRegisterer::init();
	BackendFactory FactoryInstance = BackendFactory::getInstance();
	boost::shared_ptr<DeviceBackend> _pcieDeviceInstance;
	_pcieDeviceInstance = FactoryInstance.createDevice("PCIE0");
	if (_pcieDeviceInstance == 0)
	{
		std::cout<<"Device Failed"<<std::endl;
		return 1;
	}
	if (_pcieDeviceInstance->isConnected() == true )
		std::cout<<"Device status: Connected"<<std::endl;

	if (_pcieDeviceInstance->isOpen() == false )
		std::cout<<"Device status: Closed"<<std::endl;

	boost::shared_ptr<DeviceBackend> exampleDeviceInstance = FactoryInstance.createDevice("example");
	if (exampleDeviceInstance == 0)
	{
		std::cout<<"Device Failed"<<std::endl;
		return 1;
	}
	if (exampleDeviceInstance->isConnected() == true )
		std::cout<<"Device status: Connected"<<std::endl;

	if (exampleDeviceInstance->isOpen() == false )
		std::cout<<"Device status: Closed"<<std::endl;

	exampleDeviceInstance->open();
	if (exampleDeviceInstance->isOpen() == true )
		std::cout<<"Device status: Open"<<std::endl;
	exampleDeviceInstance->close();
	if (exampleDeviceInstance->isOpen() == false )
		std::cout<<"Device status: Closed"<<std::endl;

	return 0;
}

