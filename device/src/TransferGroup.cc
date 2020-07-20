/*
 * TransferGroup.cc
 *
 *  Created on: Feb 11, 2016
 *      Author: Martin Hierholzer
 */

#include "TransferGroup.h"
#include "CopyRegisterDecorator.h"
#include "Exception.h"
#include "NDRegisterAccessorAbstractor.h"
#include "NDRegisterAccessorDecorator.h"
#include "TransferElement.h"
#include "TransferElementAbstractor.h"
#include <iostream>

namespace ChimeraTK {

  /*********************************************************************************************************************/

  std::exception_ptr TransferGroup::runPostReads(
      std::set<boost::shared_ptr<TransferElement>>& elements, std::exception_ptr firstDetectedRuntimeError) {
    std::exception_ptr badNumericCast{nullptr}; // first detected bad_numeric_cast
    for(auto& elem : elements) {
      std::exception_ptr lowLevelElementException{nullptr};
      // check for exceptions on any of the element's low level elements
      for(auto& lowLevelElem : elem->getHardwareAccessingElements()) {
        if(lowLevelElem->_activeException) {
          if(lowLevelElementException) {
            std::cout << "Warning: more than one low level exception in  " << elem->getName()
                      << ". You might lose an exception type!" << std::endl;
          }
          else {
            lowLevelElementException = lowLevelElem->_activeException;
          }
        }
      }

      elem->_activeException =
          lowLevelElementException; // copy the runtime error from low level element into the high level element so it is processed in  post-read
      try {
        // call with updateDataBuffer = false if there has been any ChimeraTK::runtime_error in the transfer phase, true otherwise
        elem->postRead(TransferType::read, firstDetectedRuntimeError == nullptr);
      }
      catch(ChimeraTK::runtime_error&) {
        // just discard the runtime error. It is only a re-thrown error. We already know about it.
      }
      catch(boost::numeric::bad_numeric_cast&) {
        if(badNumericCast == nullptr) { // only store first detected exception
          badNumericCast = std::current_exception();
        }
      }
      catch(...) {
        std::cout << "BUG: Wrong exception type thrown in doPostRead() or doPostWrite()!" << std::endl;
        std::terminate();
      }
    }
    return badNumericCast;
  }

  template<typename Callable>
  TransferGroup::ExceptionHandlingResult TransferGroup::handlePostExceptions(Callable function) {
    try {
      function();
    }
    catch(ChimeraTK::runtime_error& ex) {
      return ExceptionHandlingResult(true, ex.what());
    }
    catch(ChimeraTK::logic_error& ex) {
      return ExceptionHandlingResult(true, ex.what());
    }
    catch(boost::numeric::bad_numeric_cast& ex) {
      return ExceptionHandlingResult(true, ex.what());
    }
    catch(boost::thread_interrupted&) {
      return ExceptionHandlingResult(true, {}, true); // report that we have seen a thread_interrupted exception
    }
    catch(...) {
      std::cout << "BUG: Wrong exception type thrown in doPostRead() or doPostWrite()!" << std::endl;
      std::terminate();
    }
    return ExceptionHandlingResult(); // result without exceptions
  }

  void TransferGroup::read() {
    // reset exception flags
    for(auto& it : _lowLevelElementsAndExceptionFlags) {
      it.second = false;
    }

    // check pre-conditions so preRead() does not throw logic errors
    for(auto& backend : _exceptionBackends) {
      if(!backend->isOpen()) {
        throw ChimeraTK::logic_error("DeviceBackend " + backend->readDeviceInfo() + "is not opened!");
      }
    }
    for(auto& elem : _highLevelElements) {
      // FIXME: cache this information until the next runtime error to avoid virtual function calls
      if(!elem->isReadable()) {
        throw ChimeraTK::logic_error(elem->getName() + "is not readable!");
      }
    }

    for(auto& elem : _highLevelElements) {
      elem->preReadAndHandleExceptions(TransferType::read);
      assert(elem->_activeException == nullptr);
    }

    for(auto& elem : _copyDecorators) {
      elem->preReadAndHandleExceptions(TransferType::read);
      assert(elem->_activeException == nullptr);
    }

    std::exception_ptr firstDetectedRuntimeError{nullptr};

    for(auto& it : _lowLevelElementsAndExceptionFlags) {
      auto& elem = it.first;
      elem->handleTransferException([&] { elem->readTransfer(); });
      if((elem->_activeException != nullptr) && (firstDetectedRuntimeError == nullptr)) {
        firstDetectedRuntimeError = elem->_activeException;
      }
    }

    auto badNumericCast = runPostReads(_copyDecorators, firstDetectedRuntimeError);
    auto badNumericCast2 = runPostReads(_highLevelElements, firstDetectedRuntimeError);

    // re-throw exceptions in the order of occurence
    if(firstDetectedRuntimeError != nullptr) {
      std::rethrow_exception(firstDetectedRuntimeError);
    }
    if(badNumericCast != nullptr) {
      std::rethrow_exception(badNumericCast);
    }
    if(badNumericCast2 != nullptr) {
      std::rethrow_exception(badNumericCast2);
    }
  } // namespace ChimeraTK

  /*********************************************************************************************************************/

  void TransferGroup::write(VersionNumber versionNumber) {
    if(isReadOnly()) {
      throw ChimeraTK::logic_error("TransferGroup::write() called, but the TransferGroup is read-only.");
    }
    // check pre-conditions so preRead() does not throw logic errors
    for(auto& backend : _exceptionBackends) {
      if(!backend->isOpen()) {
        throw ChimeraTK::logic_error("DeviceBackend " + backend->readDeviceInfo() + "is not opened!");
      }
    }
    for(auto& elem : _highLevelElements) {
      // FIXME: cache this information until the next runtime error to avoid virtual function calls
      if(!elem->isReadable()) {
        throw ChimeraTK::logic_error(elem->getName() + "is not readable!");
      }
    }

    for(auto& it : _lowLevelElementsAndExceptionFlags) {
      it.second = false;
    }

    for(auto& elem : _highLevelElements) {
      elem->preWriteAndHandleExceptions(TransferType::write, versionNumber);
      assert(elem->_activeException == nullptr);
    }

    std::exception_ptr firstDetectedRuntimeError{nullptr};

    for(auto& it : _lowLevelElementsAndExceptionFlags) {
      auto& elem = it.first;
      elem->handleTransferException([&] { elem->writeTransfer(versionNumber); });
      if((elem->_activeException != nullptr) && (firstDetectedRuntimeError == nullptr)) {
        firstDetectedRuntimeError = elem->_activeException;
      }
    }

    for(auto& elem : _highLevelElements) {
      try {
        elem->postWrite(TransferType::write, versionNumber);
      }
      catch(ChimeraTK::runtime_error&) {
        // Just discard all runtime errors. They are only re-thrown. We already know the firstDetectedException.
      }
    }

    if(firstDetectedRuntimeError != nullptr) {
      std::rethrow_exception(firstDetectedRuntimeError);
    }
  }

  /*********************************************************************************************************************/

  bool TransferGroup::isReadOnly() { return readOnly; }

  /*********************************************************************************************************************/

  void TransferGroup::addAccessor(TransferElementAbstractor& accessor) {
    // check if accessor is already in a transfer group
    if(accessor.getHighLevelImplElement()->_isInTransferGroup) {
      throw ChimeraTK::logic_error("The given accessor is already in a TransferGroup and cannot be added "
                                   "to another.");
    }

    // Only accessors without wait_for_new_data can be used in a transfer group.
    if(accessor.getAccessModeFlags().has(AccessMode::wait_for_new_data)) {
      throw ChimeraTK::logic_error(
          "A TransferGroup can only be used with transfer elements that don't have aAccessMode::wait_for_new_data.");
    }

    // set flag on the accessors that it is now in a transfer group
    accessor.getHighLevelImplElement()->_isInTransferGroup = true;

    _exceptionBackends.insert(accessor.getHighLevelImplElement()->getExceptionBackend());

    auto highLevelElementsWithNewAccessor = _highLevelElements;
    highLevelElementsWithNewAccessor.insert(accessor.getHighLevelImplElement());

    // try replacing all internal elements in all high-level elements
    for(auto& hlElem1 : highLevelElementsWithNewAccessor) {
      auto list = hlElem1->getInternalElements();
      list.push_front(hlElem1);

      for(auto& replacement : list) {
        // try on the abstractor first, to make sure we replace at the highest
        // level if possible
        accessor.replaceTransferElement(replacement);
        // try on all high-level elements already stored in the list
        for(auto& hlElem : highLevelElementsWithNewAccessor) {
          hlElem->replaceTransferElement(replacement); // note: this does nothing, if the replacement cannot
                                                       // be used by the hlElem!
        }
      }
    }

    // store the accessor in the list of high-level elements
    // this must be done only now, since it may have been replaced during the
    // replacement process
    _highLevelElements.insert(accessor.getHighLevelImplElement());

    // update the list of hardware-accessing elements, since we might just have
    // made some of them redundant since we are using a set to store the elements,
    // duplicates are intrinsically avoided.
    _lowLevelElementsAndExceptionFlags.clear();
    for(auto& hlElem : _highLevelElements) {
      for(auto& hwElem : hlElem->getHardwareAccessingElements())
        _lowLevelElementsAndExceptionFlags.insert({hwElem, false});
    }

    // update the list of CopyRegisterDecorators
    _copyDecorators.clear();
    for(auto& hlElem : _highLevelElements) {
      if(boost::dynamic_pointer_cast<ChimeraTK::CopyRegisterDecoratorTrait>(hlElem) != nullptr) {
        _copyDecorators.insert(hlElem);
      }
      for(auto& hwElem : hlElem->getInternalElements()) {
        if(boost::dynamic_pointer_cast<ChimeraTK::CopyRegisterDecoratorTrait>(hwElem) != nullptr) {
          _copyDecorators.insert(hwElem);
        }
      }
    }

    // Update read-only flag
    if(accessor.isReadOnly()) readOnly = true;
  }

  /*********************************************************************************************************************/

  namespace detail {
    /// just used in TransferGroup::addAccessor(const
    /// boost::shared_ptr<TransferElement> &accessor)
    struct TransferGroupTransferElementAbstractor : TransferElementAbstractor {
      TransferGroupTransferElementAbstractor(boost::shared_ptr<TransferElement> impl)
      : TransferElementAbstractor(impl) {}

      void replaceTransferElement(boost::shared_ptr<TransferElement> newElement) {
        _impl->replaceTransferElement(newElement);
      }
    };
  } // namespace detail

  /*********************************************************************************************************************/

  void TransferGroup::addAccessor(const boost::shared_ptr<TransferElement>& accessor) {
    /// @todo implement smarter and more efficient!
    auto x = detail::TransferGroupTransferElementAbstractor(accessor);
    addAccessor(x);
  }

  /*********************************************************************************************************************/

  void TransferGroup::dump() {
    std::cout << "=== Accessors added to this group: " << std::endl;
    for(auto& elem : _highLevelElements) {
      std::cout << " - " << elem->getName() << std::endl;
    }
    std::cout << "=== Low-level transfer elements in this group: " << std::endl;
    for(auto& elem : _lowLevelElementsAndExceptionFlags) {
      std::cout << " - " << elem.first->getName() << std::endl;
    }
    std::cout << "===" << std::endl;
  }

} /* namespace ChimeraTK */
