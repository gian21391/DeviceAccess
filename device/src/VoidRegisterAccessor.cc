#include "VoidRegisterAccessor.h"

namespace ChimeraTK {

  VoidRegisterAccessor::VoidRegisterAccessor(boost::shared_ptr<NDRegisterAccessor<Void>> impl)
  : NDRegisterAccessorAbstractor(impl) {
    if(!impl->getAccessModeFlags().has(AccessMode::wait_for_new_data) && !impl->isWriteable()) {
      throw ChimeraTK::logic_error(
          "A VoidRegisterAccessor without wait_for_new_data does not make sense for non-writeable register " +
          impl->getName());
    }
  }

  bool VoidRegisterAccessor::isReadOnly() const {
    // synchronous void accessors are never readable, hence they are never read-only
    if(!_impl->getAccessModeFlags().has(AccessMode::wait_for_new_data)) {
      return false;
    }
    return _impl->isReadOnly();
  }

  bool VoidRegisterAccessor::isReadable() const {
    // synchronous void accessors are never readable
    if(!_impl->getAccessModeFlags().has(AccessMode::wait_for_new_data)) {
      return false;
    }
    return _impl->isReadable();
  }

  void VoidRegisterAccessor::read() {
    if(!_impl->getAccessModeFlags().has(AccessMode::wait_for_new_data)) {
      throw ChimeraTK::logic_error("A VoidRegisterAccessor without wait_for_new_data is not readable.");
    }
    _impl->read();
  }

  bool VoidRegisterAccessor::readNonBlocking() {
    if(!_impl->getAccessModeFlags().has(AccessMode::wait_for_new_data)) {
      throw ChimeraTK::logic_error("A VoidRegisterAccessor without wait_for_new_data is not readable.");
    }
    return _impl->readNonBlocking();
  }

  bool VoidRegisterAccessor::readLatest() {
    if(!_impl->getAccessModeFlags().has(AccessMode::wait_for_new_data)) {
      throw ChimeraTK::logic_error("A VoidRegisterAccessor without wait_for_new_data is not readable.");
    }
    return _impl->readLatest();
  }

} // namespace ChimeraTK
