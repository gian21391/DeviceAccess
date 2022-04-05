/*
 * RegisterInfo.h
 *
 *  Created on: Mar 1, 2016
 *      Author: Martin Hierholzer
 */

#pragma once

#include "ForwardDeclarations.h"
#include "BackendRegisterInfoBase.h"

#include <iostream>
#include <memory>

namespace ChimeraTK {

  class RegisterInfo {
   public:
    explicit RegisterInfo(std::unique_ptr<BackendRegisterInfoBase>&& impl);

    RegisterInfo(const RegisterInfo& other);
    RegisterInfo(RegisterInfo&& other) = default;

    RegisterInfo& operator=(const RegisterInfo& other);
    RegisterInfo& operator=(RegisterInfo&& other) = default;

    /** Return full path name of the register (including modules) */
    [[nodiscard]] RegisterPath getRegisterName() const;

    /** Return number of elements per channel */
    [[nodiscard]] unsigned int getNumberOfElements() const;

    /** Return number of channels in register */
    [[nodiscard]] unsigned int getNumberOfChannels() const;

    /** Return number of dimensions of this register */
    [[nodiscard]] unsigned int getNumberOfDimensions() const;

    /** Return desciption of the actual payload data for this register. See the
     * description of DataDescriptor for more information. */
    [[nodiscard]] const DataDescriptor& getDataDescriptor() const;

    /** Return whether the register is readable. */
    [[nodiscard]] bool isReadable() const;

    /** Return whether the register is writeable. */
    [[nodiscard]] bool isWriteable() const;

    /** Return all supported AccessModes for this register */
    [[nodiscard]] AccessModeFlags getSupportedAccessModes() const;

    /** Check whether the RegisterPath object is valid (i.e. contains an implementation object) */
    [[nodiscard]] bool isValid() const;

    /**
     * Return a reference to the implementation object. Only for advanced use, e.g. when backend-depending code shall
     * be written.
     */
    [[nodiscard]] BackendRegisterInfoBase& getImpl();

    /**
     * Return a const reference to the implementation object. Only for advanced use, e.g. when backend-depending code
     * shall be written.
     */
    [[nodiscard]] const BackendRegisterInfoBase& getImpl() const;

   protected:
    std::unique_ptr<BackendRegisterInfoBase> _impl;
  };

} /* namespace ChimeraTK */