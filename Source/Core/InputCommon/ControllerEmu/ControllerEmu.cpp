// Copyright 2010 Dolphin Emulator Project
// Licensed under GPLv2+
// Refer to the license.txt file included.

#include "InputCommon/ControllerEmu/ControllerEmu.h"

#include <memory>
#include <mutex>
#include <string>

#include "Common/IniFile.h"

#include "InputCommon/ControlReference/ControlReference.h"
#include "InputCommon/ControllerEmu/Control/Control.h"
#include "InputCommon/ControllerEmu/ControlGroup/ControlGroup.h"
#include "InputCommon/ControllerEmu/ControlGroup/Extension.h"

namespace ControllerEmu
{
static std::recursive_mutex s_get_state_mutex;

EmulatedController::~EmulatedController() = default;

// This should be called before calling GetState() or State() on a control reference
// to prevent a race condition.
// This is a recursive mutex because UpdateReferences is recursive.
std::unique_lock<std::recursive_mutex> EmulatedController::GetStateLock()
{
  std::unique_lock<std::recursive_mutex> lock(s_get_state_mutex);
  return lock;
}

void EmulatedController::UpdateReferences(const ControllerInterface& devi)
{
  const auto lock = GetStateLock();
  for (auto& ctrlGroup : groups)
  {
    for (auto& control : ctrlGroup->controls)
      control->control_ref.get()->UpdateReference(devi, default_device);

    // extension
    if (ctrlGroup->type == GroupType::Extension)
    {
      for (auto& attachment : ((Extension*)ctrlGroup.get())->attachments)
        attachment->UpdateReferences(devi);
    }
  }
}

void EmulatedController::UpdateDefaultDevice()
{
  for (auto& ctrlGroup : groups)
  {
    // extension
    if (ctrlGroup->type == GroupType::Extension)
    {
      for (auto& ai : ((Extension*)ctrlGroup.get())->attachments)
      {
        ai->default_device = default_device;
        ai->UpdateDefaultDevice();
      }
    }
  }
}

void EmulatedController::LoadConfig(IniFile::Section* sec, const std::string& base)
{
  std::string defdev = default_device.ToString();
  if (base.empty())
  {
    sec->Get(base + "Device", &defdev, "");
    default_device.FromString(defdev);
  }

  for (auto& cg : groups)
    cg->LoadConfig(sec, defdev, base);
}

void EmulatedController::SaveConfig(IniFile::Section* sec, const std::string& base)
{
  const std::string defdev = default_device.ToString();
  if (base.empty())
    sec->Set(/*std::string(" ") +*/ base + "Device", defdev, "");

  for (auto& ctrlGroup : groups)
    ctrlGroup->SaveConfig(sec, defdev, base);
}

void EmulatedController::LoadDefaults(const ControllerInterface& ciface)
{
  // load an empty inifile section, clears everything
  IniFile::Section sec;
  LoadConfig(&sec);

  const std::string& default_device_string = ciface.GetDefaultDeviceString();
  if (!default_device_string.empty())
  {
    default_device.FromString(default_device_string);
    UpdateDefaultDevice();
  }
}
}  // namespace ControllerEmu
