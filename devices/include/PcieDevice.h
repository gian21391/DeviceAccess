#ifndef MTCA4U_LIBDEV_STRUCT_H
#define	MTCA4U_LIBDEV_STRUCT_H

#include "BaseDeviceImpl.h"
#include <stdint.h>
#include <stdlib.h>
#include <boost/function.hpp>
#include "BaseDevice.h"

namespace mtca4u{

class PcieDevice : public BaseDeviceImpl
{
private:
    int  _deviceID;
    unsigned long _ioctlPhysicalSlot;
    unsigned long _ioctlDriverVersion;
    unsigned long _ioctlDMA;
    
    /// A function pointer which calls the correct dma read function (via ioctl or via struct)
    boost::function< void (uint8_t bar, uint32_t address, int32_t* data, size_t size)>
      _readDMAFunction;

    /// A function pointer which call the right write function
    //boost::function< void (uint8_t, uint32_t, int32_t const *) > _writeFunction;

    /// For the area we need something with a loop for the struct write.
    /// For the direct write this is the same as writeFunction.
    boost::function< void (uint8_t bar, uint32_t address, int32_t const * data,
			   size_t sizeInBytes ) > _writeFunction;

    boost::function<  void( uint8_t bar, uint32_t address, int32_t * data,
			    size_t sizeInBytes ) > _readFunction;

    void readDMAViaIoctl(uint8_t bar, uint32_t address, int32_t* data, size_t sizeInBytes);
    void readDMAViaStruct(uint8_t bar, uint32_t address, int32_t* data, size_t sizeInBytes);

    std::string createErrorStringWithErrnoText(std::string const & startText);
    void  determineDriverAndConfigureIoctl();
    void writeInternal(uint8_t bar, uint32_t address, int32_t const* data);
    void writeWithStruct(uint8_t bar, uint32_t address, int32_t const* data,  size_t sizeInBytes);
    /** This function is the same for one or multiple words */
    void directWrite(uint8_t bar, uint32_t address, int32_t const* data,  size_t sizeInBytes);

    void readInternal(uint8_t bar, uint32_t address, int32_t* data);
    void readWithStruct(uint8_t bar, uint32_t address, int32_t* data, size_t sizeInBytes);
    /** This function is the same for one or multiple words */
    void directRead(uint8_t bar, uint32_t address, int32_t* data,  size_t sizeInBytes);



    /** constructor called through createInstance to create device object */

public:
    PcieDevice();
    PcieDevice(std::string host, std::string instance, std::list<std::string> parameters);
    virtual ~PcieDevice();

    virtual void open(const std::string &devName, int perm = O_RDWR, DeviceConfigBase* pConfig = NULL);
    virtual void open();
    virtual void close();
    
    virtual void read(uint8_t bar, uint32_t address, int32_t* data,  size_t sizeInBytes);
    virtual void write(uint8_t bar, uint32_t address, int32_t const* data,  size_t sizeInBytes);
    
    virtual void readDMA(uint8_t bar, uint32_t address, int32_t* data,  size_t sizeInBytes);
    virtual void writeDMA(uint8_t bar, uint32_t address, int32_t const* data,  size_t sizeInBytes);

    virtual std::string readDeviceInfo();

    /*Host or parameters (at least for now) are just place holders as pcidevice does not use them*/
    static boost::shared_ptr<BaseDevice> createInstance(std::string host, std::string instance, std::list<std::string> parameters);
};

}//namespace mtca4u

#endif	/* MTCA4U_LIBDEV_STRUCT_H */

