/*
 * exTcpCtrl.h
 *
 *  Created on: May 29, 2015
 *      Author: adagio
 */

#ifndef EXTCPCTRL_H_
#define EXTCPCTRL_H_

#include <DeviceException.h>
#include <string>

namespace mtca4u {

/// Provides class for exceptions related to RebotDevice
class RebotBackendException : public DeviceBackendException {
public:
  enum {
    EX_OPEN_SOCKET,
    EX_CONNECTION_FAILED,
    EX_CLOSE_SOCKET_FAILED,
    EX_SOCKET_WRITE_FAILED,
    EX_SOCKET_READ_FAILED,
    EX_DEVICE_CLOSED,
    EX_SET_IP_FAILED,
    EX_SET_PORT_FAILED,
    EX_SIZE_INVALID,
    EX_INVALID_PARAMETERS
  };
  RebotBackendException(const std::string &_exMessage, unsigned int _exID);
};

} // namespace mtca4u

#endif /* EXTCPCTRL_H_ */