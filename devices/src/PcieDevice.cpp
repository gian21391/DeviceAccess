#include <iostream>
#include <string.h>
#include <errno.h>
#include <stdio.h>
#include <sys/ioctl.h>
#include <sstream>
#include <unistd.h>

#include <boost/bind.hpp>
#include <boost/shared_ptr.hpp>

//#include <boost/lambda.hpp>

// the io constants and struct for the driver
// FIXME: they should come from the installed driver
#include <pciedev_io.h>
#include <pcieuni_io_compat.h>
#include <llrfdrv_io_compat.h>
#include "PcieDevice.h"
#include "PcieDeviceException.h"
namespace mtca4u {

PcieDevice::PcieDevice()
	:_deviceID(0),
	_ioctlPhysicalSlot(0),
	_ioctlDriverVersion(0),
	_ioctlDMA(0)	{}

PcieDevice::PcieDevice(std::string host, std::string instance, std::list<std::string> parameters)
: BaseDeviceImpl(host,instance,parameters)
, _deviceID(0),
_ioctlPhysicalSlot(0),
_ioctlDriverVersion(0),
_ioctlDMA(0)
{
	//temp
	_instance = "/dev/"+_instance;
#ifdef _DEBUG
	std::cout<<"pci is connected"<<std::endl;
#endif
}

PcieDevice::~PcieDevice() {close(); }

void PcieDevice::open() {
#ifdef _DEBUG
	std::cout << "open pcie dev" << std::endl;
#endif
	open(_instance);
}

void PcieDevice::open(const std::string& devName, int perm,
		DeviceConfigBase* /*pConfig*/) {
	if (_opened) {
		throw PcieDeviceException("Device already has been _Opened",
				PcieDeviceException::EX_DEVICE_OPENED);
	}
	_instance =  devName; //Todo cleanup
	_deviceID = ::open(devName.c_str(), perm);
	if (_deviceID < 0) {
		throw PcieDeviceException(createErrorStringWithErrnoText("Cannot open device: "),
				PcieDeviceException::EX_CANNOT_OPEN_DEVICE);
	}

	determineDriverAndConfigureIoctl();

	_opened = true;
}

void PcieDevice::determineDriverAndConfigureIoctl() {
	// determine the driver by trying the physical slot ioctl
	device_ioctrl_data ioctlData = { 0, 0, 0, 0 };

	if (ioctl(_deviceID, PCIEDEV_PHYSICAL_SLOT, &ioctlData) >= 0) {
		// it's the pciedev driver
		_ioctlPhysicalSlot = PCIEDEV_PHYSICAL_SLOT;
		_ioctlDriverVersion = PCIEDEV_DRIVER_VERSION;
		_ioctlDMA = PCIEDEV_READ_DMA;
		_readDMAFunction =
				boost::bind(&PcieDevice::readDMAViaIoctl, this, _1, _2, _3, _4);
		//_writeFunction =
			//	boost::bind(&PcieDevice::writeWithStruct, this, _1, _2, _3);
		_writeFunction =
				boost::bind(&PcieDevice::writeWithStruct, this, _1, _2, _3, _4);
		//_readFunction = boost::bind(&PcieDevice::readWithStruct, this, _1, _2, _3);
		_readFunction =
				boost::bind(&PcieDevice::readWithStruct, this, _1, _2, _3, _4);
		return;
	}

	if (ioctl(_deviceID, LLRFDRV_PHYSICAL_SLOT, &ioctlData) >= 0) {
		// it's the llrf driver
		_ioctlPhysicalSlot = LLRFDRV_PHYSICAL_SLOT;
		_ioctlDriverVersion = LLRFDRV_DRIVER_VERSION;
		_ioctlDMA = 0;
		_readDMAFunction =
				boost::bind(&PcieDevice::readDMAViaStruct, this, _1, _2, _3, _4);
		_writeFunction =
				boost::bind(&PcieDevice::writeWithStruct, this, _1, _2, _3, _4);
		//_readFunction = boost::bind(&PcieDevice::readWithStruct, this, _1, _2, _3);
		_readFunction =
				boost::bind(&PcieDevice::readWithStruct, this, _1, _2, _3, _4);
		return;
	}

	if (ioctl(_deviceID, PCIEUNI_PHYSICAL_SLOT, &ioctlData) >= 0) {
		// it's the pcieuni
		_ioctlPhysicalSlot = PCIEUNI_PHYSICAL_SLOT;
		_ioctlDriverVersion = PCIEUNI_DRIVER_VERSION;
		_ioctlDMA = PCIEUNI_READ_DMA;
		_readDMAFunction =
				boost::bind(&PcieDevice::readDMAViaIoctl, this, _1, _2, _3, _4);
		//_writeFunction = boost::bind(&PcieDevice::directWrite, this, _1, _2, _3,
		//		sizeof(int32_t));
		_writeFunction =
				boost::bind(&PcieDevice::directWrite, this, _1, _2, _3, _4);
		_readFunction =
				boost::bind(&PcieDevice::directRead, this, _1, _2, _3, sizeof(int32_t));
		_readFunction =
				boost::bind(&PcieDevice::directRead, this, _1, _2, _3, _4);
		return;
	}

	// No working driver. Close the device and throw an exception.
	std::cerr << "Unsupported driver. "
			<< createErrorStringWithErrnoText("Error is ") << std::endl;
	;
	::close(_deviceID);
	throw PcieDeviceException("Unsupported driver in device" + _instance,
			PcieDeviceException::EX_UNSUPPORTED_DRIVER);
}

void PcieDevice::close() {
	if (_opened) {
		::close(_deviceID);
	}
	_opened = false;
}

void PcieDevice::readInternal(uint8_t bar, uint32_t address, int32_t* data) {
	device_rw l_RW;
	if (_opened == false) {
		throw PcieDeviceException("Device closed", PcieDeviceException::EX_DEVICE_CLOSED);
	}
	l_RW.barx_rw = bar;
	l_RW.mode_rw = RW_D32;
	l_RW.offset_rw = address;
	l_RW.size_rw =
			0; // does not overwrite the struct but writes one word back to data
	l_RW.data_rw = -1;
	l_RW.rsrvd_rw = 0;

	if (::read(_deviceID, &l_RW, sizeof(device_rw)) != sizeof(device_rw)) {
		throw PcieDeviceException(
				createErrorStringWithErrnoText("Cannot read data from device: "),
				PcieDeviceException::EX_READ_ERROR);
	}
	*data = l_RW.data_rw;
}

void PcieDevice::directRead(uint8_t bar, uint32_t address, int32_t* data,  size_t sizeInBytes) {
	if (_opened == false) {
		throw PcieDeviceException("Device closed", PcieDeviceException::EX_DEVICE_CLOSED);
	}
	if (bar > 5) {
		std::stringstream errorMessage;
		errorMessage << "Invalid bar number: " << bar << std::endl;
		throw PcieDeviceException(errorMessage.str(), PcieDeviceException::EX_READ_ERROR);
	}
	loff_t virtualOffset = PCIEUNI_BAR_OFFSETS[bar] + address;

	if (pread(_deviceID, data, sizeInBytes, virtualOffset) !=
			static_cast<int>(sizeInBytes)) {
		throw PcieDeviceException(
				createErrorStringWithErrnoText("Cannot read data from device: "),
				PcieDeviceException::EX_READ_ERROR);
	}
}


void PcieDevice::writeInternal(uint8_t bar, uint32_t address, int32_t const* data) {
	device_rw l_RW;
	if (_opened == false) {
		throw PcieDeviceException("Device closed", PcieDeviceException::EX_DEVICE_CLOSED);
	}
	l_RW.barx_rw = bar;
	l_RW.mode_rw = RW_D32;
	l_RW.offset_rw = address;
	l_RW.data_rw = *data;
	l_RW.rsrvd_rw = 0;
	l_RW.size_rw = 0;

	if (::write(_deviceID, &l_RW, sizeof(device_rw)) != sizeof(device_rw)) {
		throw PcieDeviceException(
				createErrorStringWithErrnoText("Cannot write data to device: "),
				PcieDeviceException::EX_WRITE_ERROR);
	}
}

// direct write allows to read areas directly, without a loop in user space
void PcieDevice::directWrite(uint8_t bar, uint32_t address, int32_t const* data,  size_t sizeInBytes) {
	if (_opened == false) {
		throw PcieDeviceException("Device closed", PcieDeviceException::EX_DEVICE_CLOSED);
	}
	if (bar > 5) {
		std::stringstream errorMessage;
		errorMessage << "Invalid bar number: " << bar << std::endl;
		throw PcieDeviceException(errorMessage.str(), PcieDeviceException::EX_WRITE_ERROR);
	}
	loff_t virtualOffset = PCIEUNI_BAR_OFFSETS[bar] + address;

	if (pwrite(_deviceID, data, sizeInBytes, virtualOffset) !=
			static_cast<int>(sizeInBytes)) {
		throw PcieDeviceException(
				createErrorStringWithErrnoText("Cannot write data to device: "),
				PcieDeviceException::EX_WRITE_ERROR);
	}
}

void PcieDevice::readWithStruct(uint8_t bar, uint32_t address, int32_t* data,  size_t sizeInBytes) {
	if (sizeInBytes % 4) {
		throw PcieDeviceException("Wrong data size - must be dividable by 4",
				PcieDeviceException::EX_READ_ERROR);
	}

	for (uint32_t i = 0; i < sizeInBytes / 4; i++) {
		readInternal(bar, address + i * 4, data + i);
	}
}

void PcieDevice::read(uint8_t bar, uint32_t address, int32_t* data,  size_t sizeInBytes)
{
	// Yes I know, the order of bar and size is reversed. The writeArea interface
	// got it wrong and I wanted to break it to keep the internal functions nice.
	_readFunction(bar, address, data, sizeInBytes);
	//read(bar, address, data);
}

void PcieDevice::writeWithStruct(uint8_t bar, uint32_t address, int32_t const* data,  size_t sizeInBytes) {
	if (_opened == false) {
		throw PcieDeviceException("Device closed", PcieDeviceException::EX_DEVICE_CLOSED);
	}
	if (sizeInBytes % 4) {
		throw PcieDeviceException("Wrong data size - must be dividable by 4",
				PcieDeviceException::EX_WRITE_ERROR);
	}
	for (uint32_t i = 0; i < sizeInBytes / 4; i++) {
		writeInternal(bar, address + i * 4, (data + i));
	}
}

void PcieDevice::write(uint8_t bar, uint32_t address, int32_t const* data,  size_t sizeInBytes) {
	_writeFunction(bar, address, data, sizeInBytes);
}

void PcieDevice::readDMA(uint8_t bar, uint32_t address, int32_t* data,  size_t sizeInBytes) {
	_readDMAFunction(bar, address, data, sizeInBytes );
}

void PcieDevice::readDMAViaStruct(uint8_t /*bar*/, uint32_t address, int32_t* data,  size_t sizeInBytes) {
	ssize_t ret;
	device_rw l_RW;
	device_rw* pl_RW;

	if (_opened == false) {
		throw PcieDeviceException("Device closed", PcieDeviceException::EX_DEVICE_CLOSED);
	}
	if (sizeInBytes < sizeof(device_rw)) {
		pl_RW = &l_RW;
	} else {
		pl_RW = (device_rw*)data;
	}

	pl_RW->data_rw = 0;
	pl_RW->barx_rw = 0;
	pl_RW->size_rw = sizeInBytes;
	pl_RW->mode_rw = RW_DMA;
	pl_RW->offset_rw = address;
	pl_RW->rsrvd_rw = 0;

	ret = ::read(_deviceID, pl_RW, sizeof(device_rw));
	if (ret != (ssize_t)sizeInBytes) {
		throw PcieDeviceException(
				createErrorStringWithErrnoText("Cannot read data from device: "),
				PcieDeviceException::EX_DMA_READ_ERROR);
	}
	if (sizeInBytes < sizeof(device_rw)) {
		memcpy(data, pl_RW, sizeInBytes);
	}
}

void PcieDevice::readDMAViaIoctl(uint8_t /*bar*/, uint32_t address, int32_t* data,  size_t sizeInBytes) {
	if (_opened == false) {
		throw PcieDeviceException("Device closed", PcieDeviceException::EX_DEVICE_CLOSED);
	}

	// safety check: the requested dma size (size of the data buffer) has to be at
	// least
	// the size of the dma struct, because the latter has to be copied into the
	// data buffer.
	if (sizeInBytes < sizeof(device_ioctrl_dma)) {
		throw PcieDeviceException("Reqested dma size is too small",
				PcieDeviceException::EX_DMA_READ_ERROR);
	}

	// prepare the struct
	device_ioctrl_dma DMA_RW;
	DMA_RW.dma_cmd = 0;     // FIXME: Why is it 0? => read driver code
	DMA_RW.dma_pattern = 0; // FIXME: Why is it 0? => read driver code
	DMA_RW.dma_size = sizeInBytes;
	DMA_RW.dma_offset = address;
	DMA_RW.dma_reserved1 = 0; // FIXME: is this a correct value?
	DMA_RW.dma_reserved2 = 0; // FIXME: is this a correct value?

	// the ioctrl_dma struct is copied to the beginning of the data buffer,
	// so the information about size and offset are passed to the driver.
	memcpy((void*)data, &DMA_RW, sizeof(device_ioctrl_dma));
	int ret = ioctl(_deviceID, _ioctlDMA, (void*)data);
	if (ret) {
		throw PcieDeviceException(
				createErrorStringWithErrnoText("Cannot read data from device "),
				PcieDeviceException::EX_DMA_READ_ERROR);
	}
}

void PcieDevice::writeDMA(uint8_t /*bar*/, uint32_t /*address*/, int32_t const* /*data*/,  size_t /*sizeInBytes*/) {
	throw PcieDeviceException("Operation not supported yet", PcieDeviceException::EX_DMA_WRITE_ERROR);
}

std::string PcieDevice::readDeviceInfo() {
	std::ostringstream os;
	device_ioctrl_data ioctlData = { 0, 0, 0, 0 };
	if (ioctl(_deviceID, _ioctlPhysicalSlot, &ioctlData) < 0) {
		throw PcieDeviceException(createErrorStringWithErrnoText("Cannot read device info: "),
				PcieDeviceException::EX_INFO_READ_ERROR);
	}
	os << "SLOT: " << ioctlData.data;
	if (ioctl(_deviceID, _ioctlDriverVersion, &ioctlData) < 0) {
		throw PcieDeviceException(createErrorStringWithErrnoText("Cannot read device info: "),
				PcieDeviceException::EX_INFO_READ_ERROR);
	}
	os << " DRV VER: " << (float)(ioctlData.offset / 10.0) +
			(float)ioctlData.data;
	return os.str();
}

std::string PcieDevice::createErrorStringWithErrnoText(
		std::string const& startText) {
	char errorBuffer[255];
	return startText + _instance + ": " +
			strerror_r(errno, errorBuffer, sizeof(errorBuffer));
}


boost::shared_ptr<BaseDevice> PcieDevice::createInstance(std::string host, std::string instance, std::list<std::string> parameters) {
	return boost::shared_ptr<BaseDevice> (new PcieDevice(host,instance,parameters));
}

} // namespace mtca4u
