/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/JSON/document.h>
#include <AzCore/JSON/pointer.h>
#include <AzCore/IO/Path/Path_fwd.h>
#include <AzCore/Interface/Interface.h>
#include <AzCore/Serialization/Json/JsonSerialization.h>
#include <AzCore/Settings/SettingsRegistry.h>
#include <AzCore/std/containers/fixed_vector.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/parallel/mutex.h>

// Using a define instead of a static string to avoid the need for temporary buffers to composite the full paths.
#define AZ_SETTINGS_REGISTRY_HISTORY_KEY "/Amazon/AzCore/Runtime/Registry/FileHistory"

namespace AZ
{
    class StackedString;

    class SettingsRegistryImpl final
        : public SettingsRegistryInterface
    {
    public:
        AZ_CLASS_ALLOCATOR(SettingsRegistryImpl, AZ::OSAllocator, 0);
        AZ_RTTI(AZ::SettingsRegistryImpl, "{E9C34190-F888-48CA-83C9-9F24B4E21D72}", AZ::SettingsRegistryInterface);

        static constexpr size_t MaxRegistryFolderEntries = 128;
        
        SettingsRegistryImpl();
        AZ_DISABLE_COPY_MOVE(SettingsRegistryImpl);
        ~SettingsRegistryImpl() override = default;

        void SetContext(SerializeContext* context);
        void SetContext(JsonRegistrationContext* context);
        
        Type GetType(AZStd::string_view path) const override;
        bool Visit(Visitor& visitor, AZStd::string_view path) const override;
        bool Visit(const VisitorCallback& callback, AZStd::string_view path) const override;
        [[nodiscard]] NotifyEventHandler RegisterNotifier(const NotifyCallback& callback) override;
        [[nodiscard]] NotifyEventHandler RegisterNotifier(NotifyCallback&& callback) override;
        void ClearNotifiers();

        bool Get(bool& result, AZStd::string_view path) const override;
        bool Get(s64& result, AZStd::string_view path) const override;
        bool Get(u64& result, AZStd::string_view path) const override;
        bool Get(double& result, AZStd::string_view path) const override;
        bool Get(AZStd::string& result, AZStd::string_view path) const override;
        bool Get(SettingsRegistryInterface::FixedValueString& result, AZStd::string_view path) const override;
        bool GetObject(void* result, Uuid resultTypeID, AZStd::string_view path) const override;

        bool Set(AZStd::string_view path, bool value) override;
        bool Set(AZStd::string_view path, s64 value) override;
        bool Set(AZStd::string_view path, u64 value) override;
        bool Set(AZStd::string_view path, double value) override;
        bool Set(AZStd::string_view path, AZStd::string_view value) override;
        bool Set(AZStd::string_view path, const char* value) override;
        bool SetObject(AZStd::string_view path, const void* value, Uuid valueTypeID) override;

        bool Remove(AZStd::string_view path) override;

        bool MergeCommandLineArgument(AZStd::string_view argument, AZStd::string_view rootKey,
            const CommandLineArgumentSettings& commandLineSettings) override;
        bool MergeSettings(AZStd::string_view data, Format format) override;
        bool MergeSettingsFile(AZStd::string_view path, Format format, AZStd::string_view rootKey,
            AZStd::vector<char>* scratchBuffer = nullptr) override;
        bool MergeSettingsFolder(AZStd::string_view path, const Specializations& specializations,
            AZStd::string_view platform, AZStd::string_view rootKey = "", AZStd::vector<char>* scratchBuffer = nullptr) override;

        void SetApplyPatchSettings(const AZ::JsonApplyPatchSettings& applyPatchSettings) override;
        void GetApplyPatchSettings(AZ::JsonApplyPatchSettings& applyPatchSettings) override;

    private:
        using TagList = AZStd::fixed_vector<size_t, Specializations::MaxCount + 1>;
        struct RegistryFile
        {
            AZ::IO::FixedMaxPathString m_relativePath;
            TagList m_tags;
            bool m_isPatch{ false };
            bool m_isPlatformFile{ false };
        };
        using RegistryFileList = AZStd::fixed_vector<RegistryFile, MaxRegistryFolderEntries>;

        template<typename T>
        bool SetValueInternal(AZStd::string_view path, T value);
        template<typename T>
        bool GetValueInternal(T& result, AZStd::string_view path) const;
        VisitResponse Visit(Visitor& visitor, StackedString& path, AZStd::string_view valueName,
            const rapidjson::Value& value) const;

        // Compares if lhs is less than rhs in terms of processing order. This can also detect and report conflicts.
        bool IsLessThan(bool& collisionFound, const RegistryFile& lhs, const RegistryFile& rhs, const Specializations& specializations,
            const rapidjson::Pointer& historyPointer, AZStd::string_view folderPath);
        bool ExtractFileDescription(RegistryFile& output, const char* filename, const Specializations& specializations);
        bool MergeSettingsFileInternal(const char* path, Format format, AZStd::string_view rootKey, AZStd::vector<char>& scratchBuffer);

        void SignalNotifier(AZStd::string_view jsonPath, Type type);
        
        mutable AZStd::recursive_mutex m_settingMutex;
        mutable AZStd::recursive_mutex m_notifierMutex;
        NotifyEvent m_notifiers;
        rapidjson::Document m_settings;
        JsonSerializerSettings m_serializationSettings;
        JsonDeserializerSettings m_deserializationSettings;
        JsonApplyPatchSettings m_applyPatchSettings;
    };
} // namespace AZ
